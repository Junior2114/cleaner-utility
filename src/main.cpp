#include "Cleaner.h"
#include "Logger.h"
#include <iostream>
#include <string>

static void printHelp() {
    std::cout <<
        "TempCleaner\n"
        "Usage:\n"
        "  TempCleaner [--dry] [--min-age <minutes>] [--path <dir>]...\n"
        "Options:\n"
        "  --dry             Only report, do not delete\n"
        "  --min-age N       Skip files newer than N minutes (default 10)\n"
        "  --path DIR        Add custom directory (can repeat)\n"
        "  --help            Show this help\n";
}

int wmain(int argc, wchar_t** argv) {
    Logger::instance().setFile("cleaner.log");

    bool dryRun = false;
    unsigned minAge = 10;
    std::vector<std::wstring> customPaths;

    for (int i = 1; i < argc; ++i) {
        std::wstring a = argv[i];
        if (a == L"--dry") {
            dryRun = true;
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
    if (customPaths.empty()) {
        cleaner.addDefaultTargets();
    } else {
        for (auto& p : customPaths) cleaner.addTarget(p);
    }

    Logger::instance().log(LogLevel::Info,
        dryRun ? "Running in DRY-RUN mode" : "Running in DELETE mode");

    auto result = cleaner.run(minAge);

    std::cout << "\n===== Result =====\n"
              << "Deleted : " << result.filesDeleted << '\n'
              << "Skipped : " << result.filesSkipped << '\n'
              << "Freed   : " << (result.bytesFreed / (1024.0 * 1024.0)) << " MB\n";

    return 0;
}