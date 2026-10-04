#include "cleaner/Cleaner.h"
#include "cleaner/Logger.h"
#include "cleaner/Version.h"     // ← добавили

#include <windows.h>

#include <iostream>
#include <string>
#include <vector>

static void printVersion() {
    std::cout << "my-cleaner " << MYCLEANER_VERSION_STRING << "\n";
}

static void printHelp() {
    std::cout <<
        "TempCleaner\n"
        "Usage:\n"
        "  TempCleaner [--dry] [--verbose] [--no-caches] [--min-age <minutes>] [--path <dir>]...\n"
        "Options:\n"
        "  --dry             Only report, do not delete\n"
        "  --verbose         Show each file (useful with --dry)\n"
        "  --no-caches       Skip app caches (Chrome, Edge, Discord, Spotify)\n"
        "  --only-temp       Alias for --no-caches\n"
        "  --min-age N       Skip files newer than N minutes (default 10)\n"
        "  --path DIR        Add custom directory (can repeat; replaces defaults)\n"
        "  --version, -v     Show version and exit\n"
        "  --help, -h        Show this help\n";
}

int wmain(int argc, wchar_t** argv) {
    Logger::instance().setFile("cleaner.log");

    bool dryRun      = false;
    bool verbose     = false;
    bool cleanCaches = true;
    unsigned minAge  = 10;
    std::vector<std::wstring> customPaths;

    for (int i = 1; i < argc; ++i) {
        std::wstring a = argv[i];
        if (a == L"--dry") {
            dryRun = true;
        } else if (a == L"--verbose") {
            verbose = true;
        } else if (a == L"--no-caches" || a == L"--only-temp") {
            cleanCaches = false;
        } else if (a == L"--version" || a == L"-v") {
            printVersion();
            return 0;
        } else if (a == L"--help" || a == L"-h") {
            printHelp();
            return 0;
        } else if (a == L"--min-age" && i + 1 < argc) {
            minAge = static_cast<unsigned>(_wtoi(argv[++i]));
        } else if (a == L"--path" && i + 1 < argc) {
            customPaths.emplace_back(argv[++i]);
        } else {
            std::wcerr << L"Unknown argument: " << a << L"\n";
            printHelp();
            return 1;
        }
    }

    Cleaner cleaner(dryRun);
    cleaner.setVerbose(verbose);

    if (customPaths.empty()) {
        cleaner.addDefaultTargets();
        if (cleanCaches) {
            cleaner.addAppCaches();
        }
    } else {
        for (auto& p : customPaths) cleaner.addTarget(p);
    }

    Logger::instance().log(LogLevel::Info,
        std::string("Running in ") + (dryRun ? "DRY-RUN" : "DELETE") + " mode, " +
        std::to_string(cleaner.targets().size()) + " targets");

    auto result = cleaner.run(minAge);

    std::cout << "\n===== Result =====\n"
              << "Deleted : " << result.filesDeleted << '\n'
              << "Skipped : " << result.filesSkipped << '\n'
              << "Freed   : " << (result.bytesFreed / (1024.0 * 1024.0)) << " MB\n";

    return 0;
}