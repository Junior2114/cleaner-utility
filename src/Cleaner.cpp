#include "Cleaner.h"
#include "Logger.h"
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <system_error>
#include <chrono>
#include <algorithm>

namespace fs = std::filesystem;

Cleaner::Cleaner(bool dryRun) : dryRun_(dryRun) {}

void Cleaner::addTarget(const std::wstring& dir) {
    targets_.push_back(dir);
}

void Cleaner::addDefaultTargets() {
    // %TEMP% пользователя
    wchar_t buf[MAX_PATH]{};
    DWORD n = GetEnvironmentVariableW(L"TEMP", buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        targets_.emplace_back(buf);
    }

    // C:\Windows\Temp — может потребовать админ-прав
    wchar_t winDir[MAX_PATH]{};
    if (GetWindowsDirectoryW(winDir, MAX_PATH)) {
        targets_.emplace_back(std::wstring(winDir) + L"\\Temp");
    }
}

bool Cleaner::tryDeleteFile(const std::wstring& file, CleanResult& result) {
    std::error_code ec;
    auto size = fs::file_size(file, ec);
    if (ec) size = 0;

    if (dryRun_) {
        ++result.filesDeleted;   // условно "удалён"
        result.bytesFreed += size;
        Logger::instance().log(LogLevel::Info,
            "[DRY] Would delete: " + fs::path(file).string());
        return true;
    }

    if (DeleteFileW(file.c_str())) {
        ++result.filesDeleted;
        result.bytesFreed += size;
        return true;
    }

    // Файл занят или нет прав
    ++result.filesSkipped;
    DWORD err = GetLastError();
    if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND) {
        Logger::instance().log(LogLevel::Warning,
            "Skip (" + std::to_string(err) + "): " + fs::path(file).string());
    }
    return false;
}

void Cleaner::processDirectory(const std::wstring& dir,
                               CleanResult& result,
                               unsigned minAgeMinutes)
{
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
        Logger::instance().log(LogLevel::Warning,
            "Target not found: " + fs::path(dir).string());
        return;
    }

    Logger::instance().log(LogLevel::Info,
        "Scanning: " + fs::path(dir).string());

    // Рекурсивный обход
    for (auto it = fs::recursive_directory_iterator(
             dir, fs::directory_options::skip_permission_denied, ec);
         it != fs::recursive_directory_iterator(); )
    {
        if (ec) { ec.clear(); ++it; continue; }

        const auto& entry = *it;
        std::error_code e2;

        if (entry.is_regular_file(e2)) {
            // Проверка возраста
            auto ftime = fs::last_write_time(entry.path(), e2);
            if (!e2) {
                auto now = fs::file_time_type::clock::now();
                auto age = std::chrono::duration_cast<std::chrono::minutes>(now - ftime).count();
                if (age < static_cast<long long>(minAgeMinutes)) {
                    ++it;
                    continue;
                }
            }
            tryDeleteFile(entry.path().wstring(), result);
            ++it;
        } else if (entry.is_directory(e2)) {
            ++it;
        } else {
            ++it;
        }
    }

    // Удаляем пустые подкаталоги (снизу вверх)
    if (!dryRun_) {
        std::vector<fs::path> dirs;
        for (auto it = fs::recursive_directory_iterator(
                 dir, fs::directory_options::skip_permission_denied, ec);
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
            fs::remove(d, e); // удалит только если пустой
        }
    }
}

CleanResult Cleaner::run(unsigned minAgeMinutes) {
    CleanResult total;

    for (const auto& t : targets_) {
        CleanResult r;
        processDirectory(t, r, minAgeMinutes);
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