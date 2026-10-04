include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(mycleaner_tests
    test_target.cpp
    test_cleaner.cpp
)

target_link_libraries(mycleaner_tests
    PRIVATE
        mycleaner_core
        Catch2::Catch2WithMain
)

mycleaner_set_warnings(mycleaner_tests)

# Регистрируем тесты в CTest
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(mycleaner_tests)