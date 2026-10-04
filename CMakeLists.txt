cmake_minimum_required(VERSION 3.15)
project(TempCleaner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(TempCleaner
    src/main.cpp
    src/Cleaner.cpp
    src/Logger.cpp
)

target_compile_definitions(TempCleaner PRIVATE UNICODE _UNICODE)
target_link_libraries(TempCleaner PRIVATE shell32)

if (MSVC)
    target_compile_options(TempCleaner PRIVATE /W4 /permissive-)
endif()

# --- Установка ---
# cmake --install build --config Release
#     --prefix <InstallDir>
#
# Кладём:
#   <InstallDir>/bin/TempCleaner.exe
#   <InstallDir>/bin/cleaner.ps1        (с подставленной версией)
#   <InstallDir>/install.ps1            (для ручной регистрации в $PROFILE)
#   <InstallDir>/uninstall.ps1

include(GNUInstallDirs)

install(TARGETS TempCleaner
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Генерируем cleaner.ps1 из шаблона — подставляем версию проекта
configure_file(
    ${CMAKE_SOURCE_DIR}/scripts/cleaner.ps1.in
    ${CMAKE_BINARY_DIR}/cleaner.ps1
    @ONLY
)

install(FILES
        ${CMAKE_BINARY_DIR}/cleaner.ps1
        ${CMAKE_SOURCE_DIR}/scripts/uninstall.ps1
        DESTINATION ${CMAKE_INSTALL_BINDIR})

# README и LICENSE — на всякий случай
install(FILES
        ${CMAKE_SOURCE_DIR}/README.md
        DESTINATION ${CMAKE_INSTALL_DATADIR}/my-cleaner
        OPTIONAL)