#include "cleaner/Cleaner.h"
#include "cleaner/Logger.h"

#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

// ------------------------------------------------------------
//  UTF-16 → UTF-8 без потери данных (для логов)
// ------------------------------------------------------------
static std::string wstring_to_utf8(const std::wstring& w) {
    if (w.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(),
                                   static_cast<int>(w.size()),
                                   nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string out(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(),
                        static_cast<int>(w.size()),
                        out.data(), size, nullptr, nullptr);
    return out;
}

Cleaner::Cleaner(bool dryRun) : dryRun_(dryRun) {}

// ------------------------------------------------------------
//  Добавление целей
// ------------------------------------------------------------

void Cleaner::addTarget(const std::wstring& dir) {
    Target t;
    t.path          = dir;
    t.label         = fs::path(dir).filename().wstring();
    t.minAgeMinutes = 10u;
    t.enabled       = true;
    targets_.push_back(std::move(t));
}

void Cleaner::addTarget(const Target& target) {
    targets_.push_back(target);
}

// ------------------------------------------------------------
//  Системные temp-папки
// ------------------------------------------------------------

void Cleaner::addDefaultTargets() {
    wchar_t buf[MAX_PATH]{};

    DWORD n = GetEnvironmentVariableW(L"TEMP", buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        Target t;
        t.path          = buf;
        t.label         = L"User Temp";
        t.minAgeMinutes = 10u;
        t.enabled       = true;
        targets_.push_back(std::move(t));
    }

    wchar_t winDir[MAX_PATH]{};
    if (GetWindowsDirectoryW(winDir, MAX_PATH)) {
        Target t;
        t.path          = std::wstring(winDir) + L"\\Temp";
        t.label         = L"Windows Temp";
        t.minAgeMinutes = 10u;
        t.enabled       = true;
        targets_.push_back(std::move(t));
    }
}

// ------------------------------------------------------------
//  Кэши приложений
// ------------------------------------------------------------

void Cleaner::addAppCaches() {
    wchar_t buf[MAX_PATH]{};

    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        std::wstring local = buf;

        const wchar_t* chromeProfiles[] = { L"Default", L"Profile 1" };
        for (const wchar_t* prof : chromeProfiles) {
            {
                Target t;
                t.path = local + L"\\Google\\Chrome\\User Data\\" + prof + L"\\Cache";
                t.label = std::wstring(L"Chrome Cache (") + prof + L")";
                t.minAgeMinutes = 15u;
                t.enabled = true;
                targets_.push_back(std::move(t));
            }
            {
                Target t;
                t.path = local + L"\\Google\\Chrome\\User Data\\" + prof + L"\\Code Cache";
                t.label = std::wstring(L"Chrome Code Cache (") + prof + L")";
                t.minAgeMinutes = 15u;
                t.enabled = true;
                targets_.push_back(std::move(t));
            }
        }

        const wchar_t* edgeProfiles[] = { L"Default", L"Profile 1" };
        for (const wchar_t* prof : edgeProfiles) {
            {
                Target t;
                t.path = local + L"\\Microsoft\\Edge\\User Data\\" + prof + L"\\Cache";
                t.label = std::wstring(L"Edge Cache (") + prof + L")";
                t.minAgeMinutes = 15u;
                t.enabled = true;
                targets_.push_back(std::move(t));
            }
            {
                Target t;
                t.path = local + L"\\Microsoft\\Edge\\User Data\\" + prof + L"\\Code Cache";
                t.label = std::wstring(L"Edge Code Cache (") + prof + L")";
                t.minAgeMinutes = 15u;
                t.enabled = true;
                targets_.push_back(std::move(t));
            }
            {
                Target t;
                t.path = local + L"\\Microsoft\\Edge\\User Data\\" + prof + L"\\GPUCache";
                t.label = std::wstring(L"Edge GPU Cache (") + prof + L")";
                t.minAgeMinutes = 15u;
                t.enabled = true;
                targets_.push_back(std::move(t));
            }
        }

        {
            Target t;
            t.path = local + L"\\Spotify\\Data";
            t.label = L"Spotify Data (cache, not offline tracks)";
            t.minAgeMinutes = 60u;
            t.enabled = true;
            targets_.push_back(std::move(t));
        }
    }

    wchar_t roam[MAX_PATH]{};
    n = GetEnvironmentVariableW(L"APPDATA", roam, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        std::wstring base = std::wstring(roam) + L"\\discord";

        {
            Target t;
            t.path = base + L"\\Cache";
            t.label = L"Discord Cache";
            t.minAgeMinutes = 0u;
            t.enabled = true;
            targets_.push_back(std::move(t));
        }
        {
            Target t;
            t.path = base + L"\\Code Cache";
            t.label = L"Discord Code Cache";
            t.minAgeMinutes = 0u;
            t.enabled = true;
            targets_.push_back(std::move(t));
        }
        {
            Target t;
            t.path = base + L"\\GPUCache";
            t.label = L"Discord GPU Cache";
            t.minAgeMinutes = 0u;
            t.enabled = true;
            targets_.push_back(std::move(t));
        }
        {
            Target t;
            t.path = base + L"\\blob_storage";
            t.label = L"Discord Blob Storage";
            t.minAgeMinutes = 0u;
            t.enabled = true;
            targets_.push_back(std::move(t));
        }
    }
}

// ------------------------------------------------------------
//  Удаление файла
// ------------------------------------------------------------

bool Cleaner::tryDeleteFile(const std::wstring& file, CleanResult& result) {
    std::error_code ec;
    auto size = fs::file_size(file, ec);
    if (ec) size = 0;

    if (dryRun_) {
        ++result.filesDeleted;
        result.bytesFreed += size;
        if (verbose_) {
            Logger::instance().log(LogLevel::Info,
                "[DRY] Would delete: " + wstring_to_utf8(file));
        }
        return true;
    }

    if (DeleteFileW(file.c_str())) {
        ++result.filesDeleted;
        result.bytesFreed += size;
        return true;
    }

    ++result.filesSkipped;
    DWORD err = GetLastError();
    if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) {
        Logger::instance().log(LogLevel::Warning,
            "Skip (" + std::to_string(err) + "): " + wstring_to_utf8(file));
    }
    return false;
}

// ------------------------------------------------------------
//  Обход директории
// ------------------------------------------------------------

void Cleaner::processDirectory(const Target& t, CleanResult& result) {
    std::error_code ec;
    if (!fs::exists(t.path, ec) || !fs::is_directory(t.path, ec)) {
        if (verbose_) {
            Logger::instance().log(LogLevel::Info,
                "Not found (skip): " + wstring_to_utf8(t.path));
        }
        return;
    }

    if (verbose_) {
        Logger::instance().log(LogLevel::Info,
            "Scanning [" + wstring_to_utf8(t.label) + "]: " + wstring_to_utf8(t.path));
    }

    for (auto it = fs::recursive_directory_iterator(
             t.path, fs::directory_options::skip_permission_denied, ec);
         it != fs::recursive_directory_iterator(); )
    {
        if (ec) { ec.clear(); ++it; continue; }

        const auto& entry = *it;
        std::error_code e2;

        if (entry.is_regular_file(e2)) {
            auto ftime = fs::last_write_time(entry.path(), e2);
            if (!e2) {
                auto now = fs::file_time_type::clock::now();
                auto age = std::chrono::duration_cast<std::chrono::minutes>(
                               now - ftime).count();
                if (age < static_cast<long long>(t.minAgeMinutes)) {
                    ++it;
                    continue;
                }
            }
            tryDeleteFile(entry.path().wstring(), result);
            ++it;
        } else {
            ++it;
        }
    }

    if (!dryRun_) {
        std::vector<fs::path> dirs;
        for (auto it = fs::recursive_directory_iterator(
                 t.path, fs::directory_options::skip_permission_denied, ec);
             it != fs::recursive_directory_iterator(); ++it)
        {
            if (it->is_directory(ec)) dirs.push_back(it->path());
        }
        std::sort(dirs.begin(), dirs.end(),
                  [](const fs::path& a, const fs::path& b){
                      return a.string().size() > b.string().size();
                  });
        for (auto& d : dirs) {
            std::error_code e;
            fs::remove(d, e);
        }
    }

    // Итог по цели — всегда печатаем
    Logger::instance().log(LogLevel::Info,
        "  [" + wstring_to_utf8(t.label) + "] deleted: " +
        std::to_string(result.filesDeleted) +
        ", skipped: " + std::to_string(result.filesSkipped) +
        ", freed: "    + std::to_string(result.bytesFreed / 1024) + " KB");
}

// ------------------------------------------------------------
//  Запуск
// ------------------------------------------------------------

CleanResult Cleaner::run(unsigned /*defaultMinAgeMinutes*/) {
    CleanResult total;

    for (const auto& t : targets_) {
        if (!t.enabled) continue;
        CleanResult r;
        processDirectory(t, r);
        total.filesDeleted += r.filesDeleted;
        total.filesSkipped += r.filesSkipped;
        total.bytesFreed   += r.bytesFreed;
    }

    Logger::instance().log(LogLevel::Info,
        "Done. Deleted: " + std::to_string(total.filesDeleted) +
        ", skipped: " + std::to_string(total.filesSkipped) +
        ", freed: " + std::to_string(total.bytesFreed / (1024 * 1024)) + " MB");

    return total;
}