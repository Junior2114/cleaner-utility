cmake_minimum_required(VERSION 3.15)

project(TempCleaner
    VERSION 1.1.0
    DESCRIPTION "Lightweight Windows temp file cleaner"
    LANGUAGES CXX
)

# --- Глобальные настройки ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Все бинарники складываются в build/bin/<config>/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# CMake-модули живут в cmake/
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(CompilerWarnings)

# --- Генерация Version.h из шаблона ---
configure_file(
    "${CMAKE_SOURCE_DIR}/include/cleaner/Version.h.in"
    "${CMAKE_BINARY_DIR}/generated/cleaner/Version.h"
    @ONLY
)

# --- Основная библиотека: вся логика ---
add_library(mycleaner_core STATIC
    src/Cleaner.cpp
    src/Logger.cpp
)

target_include_directories(mycleaner_core
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_BINARY_DIR}/generated"
)

target_compile_definitions(mycleaner_core PRIVATE UNICODE _UNICODE)
target_link_libraries(mycleaner_core PRIVATE shell32)

mycleaner_set_warnings(mycleaner_core)

# --- Исполняемый файл ---
add_executable(TempCleaner src/main.cpp)

target_link_libraries(TempCleaner PRIVATE mycleaner_core)

target_compile_definitions(TempCleaner PRIVATE UNICODE _UNICODE)
mycleaner_set_warnings(TempCleaner)

# --- Установка ---
include(GNUInstallDirs)

install(TARGETS TempCleaner
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Скрипт модуля PowerShell генерируется из шаблона
configure_file(
    "${CMAKE_SOURCE_DIR}/scripts/cleaner.ps1.in"
    "${CMAKE_BINARY_DIR}/cleaner.ps1"
    @ONLY
)

install(FILES
        "${CMAKE_BINARY_DIR}/cleaner.ps1"
        "${CMAKE_SOURCE_DIR}/scripts/uninstall.ps1"
        DESTINATION ${CMAKE_INSTALL_BINDIR})

# --- Тесты ---
option(ENABLE_TESTS "Build unit tests" ON)

if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()