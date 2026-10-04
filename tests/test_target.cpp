#include "cleaner/Target.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Target default fields", "[target]") {
    Target t;
    t.path          = L"C:\\Temp";
    t.label         = L"Test";
    t.minAgeMinutes = 10u;
    t.enabled       = true;

    REQUIRE(t.path == L"C:\\Temp");
    REQUIRE(t.label == L"Test");
    REQUIRE(t.minAgeMinutes == 10u);
    REQUIRE(t.enabled == true);
}

TEST_CASE("Target can be disabled", "[target]") {
    Target t;
    t.enabled = false;
    REQUIRE_FALSE(t.enabled);
}

TEST_CASE("Target supports unicode paths", "[target]") {
    Target t;
    t.path  = L"C:\\Пользователи\\Иван\\AppData\\Local\\Temp";
    t.label = L"Русский путь";
    REQUIRE(t.path == L"C:\\Пользователи\\Иван\\AppData\\Local\\Temp");
    REQUIRE(t.label == L"Русский путь");
}

TEST_CASE("Target supports long paths", "[target]") {
    // Проверяем, что wstring не имеет проблем с длинными строками
    std::wstring longPath(1000, L'a');
    Target t;
    t.path = longPath;
    REQUIRE(t.path.size() == 1000);
    REQUIRE(t.path == longPath);
}

TEST_CASE("Target supports min-age zero", "[target]") {
    Target t;
    t.minAgeMinutes = 0u;
    REQUIRE(t.minAgeMinutes == 0u);
}

TEST_CASE("Target supports large min-age", "[target]") {
    Target t;
    t.minAgeMinutes = 100000u; // ~70 дней
    REQUIRE(t.minAgeMinutes == 100000u);
}