#include "cleaner/Cleaner.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Cleaner starts with no targets", "[cleaner]") {
    Cleaner c(false);
    REQUIRE(c.targets().empty());
}

TEST_CASE("Cleaner in dry-run mode still has no targets", "[cleaner]") {
    Cleaner c(true);
    REQUIRE(c.targets().empty());
}

TEST_CASE("Cleaner::addTarget appends target", "[cleaner]") {
    Cleaner c(false);
    c.addTarget(L"C:\\SomePath");

    REQUIRE(c.targets().size() == 1);
    REQUIRE(c.targets()[0].path == L"C:\\SomePath");
    REQUIRE(c.targets()[0].enabled == true);
    // addTarget(wstring) ставит minAgeMinutes = 10 по умолчанию
    REQUIRE(c.targets()[0].minAgeMinutes == 10u);
}

TEST_CASE("Cleaner::addTarget can be called multiple times", "[cleaner]") {
    Cleaner c(false);
    c.addTarget(L"C:\\Path1");
    c.addTarget(L"C:\\Path2");
    c.addTarget(L"C:\\Path3");

    REQUIRE(c.targets().size() == 3);
    REQUIRE(c.targets()[0].path == L"C:\\Path1");
    REQUIRE(c.targets()[1].path == L"C:\\Path2");
    REQUIRE(c.targets()[2].path == L"C:\\Path3");
}

TEST_CASE("Cleaner::addTarget with Target struct preserves fields", "[cleaner]") {
    Cleaner c(false);
    Target t;
    t.path          = L"C:\\Custom";
    t.label         = L"Custom Label";
    t.minAgeMinutes = 60u;
    t.enabled       = true;
    c.addTarget(t);

    REQUIRE(c.targets().size() == 1);
    REQUIRE(c.targets()[0].path == L"C:\\Custom");
    REQUIRE(c.targets()[0].label == L"Custom Label");
    REQUIRE(c.targets()[0].minAgeMinutes == 60u);
    REQUIRE(c.targets()[0].enabled == true);
}

TEST_CASE("Cleaner::addTarget with disabled target", "[cleaner]") {
    Cleaner c(false);
    Target t;
    t.path          = L"C:\\Disabled";
    t.label         = L"Disabled";
    t.minAgeMinutes = 0u;
    t.enabled       = false;
    c.addTarget(t);

    REQUIRE(c.targets().size() == 1);
    REQUIRE(c.targets()[0].enabled == false);
}

TEST_CASE("Cleaner::addDefaultTargets adds targets", "[cleaner]") {
    Cleaner c(false);
    c.addDefaultTargets();
    // Должно добавить как минимум %TEMP% (всегда есть)
    REQUIRE(c.targets().size() >= 1);
}

TEST_CASE("Cleaner::addAppCaches does not crash", "[cleaner]") {
    Cleaner c(false);
    c.addAppCaches();
    // Может быть 0, если нет LOCALAPPDATA — это нормально.
    // Главное, что не падает и не бросает.
    SUCCEED("addAppCaches completed without throwing");
}

TEST_CASE("Cleaner::setVerbose toggles flag", "[cleaner]") {
    Cleaner c(false);
    c.setVerbose(true);
    c.setVerbose(false);
    SUCCEED("setVerbose works");
}

TEST_CASE("Cleaner::run on empty target list returns zero result", "[cleaner]") {
    Cleaner c(false);
    // Список целей пуст — прогон должен вернуть нули
    CleanResult r = c.run(10u);
    REQUIRE(r.filesDeleted == 0u);
    REQUIRE(r.filesSkipped == 0u);
    REQUIRE(r.bytesFreed == 0u);
}

TEST_CASE("Cleaner::run in dry-run on empty list returns zero result", "[cleaner]") {
    Cleaner c(true);
    CleanResult r = c.run(10u);
    REQUIRE(r.filesDeleted == 0u);
    REQUIRE(r.filesSkipped == 0u);
    REQUIRE(r.bytesFreed == 0u);
}