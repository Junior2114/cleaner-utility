#pragma once
#include "Target.h"

#include <cstdint>
#include <string>
#include <vector>

struct CleanResult {
    uint64_t filesDeleted = 0;
    uint64_t filesSkipped = 0;
    uint64_t bytesFreed   = 0;
};

class Cleaner {
public:
    explicit Cleaner(bool dryRun = false);

    void setVerbose(bool v) { verbose_ = v; }

    void addTarget(const std::wstring& dir);
    void addTarget(const Target& target);
    void addDefaultTargets();
    void addAppCaches();

    CleanResult run(unsigned defaultMinAgeMinutes = 10);

    const std::vector<Target>& targets() const { return targets_; }

private:
    void processDirectory(const Target& t, CleanResult& result);
    bool tryDeleteFile(const std::wstring& file, CleanResult& result);

    std::vector<Target> targets_;
    bool dryRun_;
    bool verbose_ = false;
};