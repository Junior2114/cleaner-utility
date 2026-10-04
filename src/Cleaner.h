#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct CleanResult {
    uint64_t filesDeleted = 0;
    uint64_t filesSkipped = 0;
    uint64_t bytesFreed   = 0;
};

class Cleaner {
public:
    // dryRun = true: только посчитать, ничего не удалять
    explicit Cleaner(bool dryRun = false);

    // Добавить директорию в список очистки
    void addTarget(const std::wstring& dir);

    // Стандартные цели: %TEMP%, Windows\Temp (если есть права)
    void addDefaultTargets();

    // Выполнить очистку. minAgeMinutes — не удалять файлы моложе N минут
    CleanResult run(unsigned minAgeMinutes = 10);

private:
    void    processDirectory(const std::wstring& dir, CleanResult& result, unsigned minAgeMinutes);
    bool    tryDeleteFile(const std::wstring& file, CleanResult& result);

    std::vector<std::wstring> targets_;
    bool dryRun_;
};