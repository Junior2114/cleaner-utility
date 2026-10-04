#pragma once
#include <string>

// Описание одной цели очистки.
struct Target {
    std::wstring path;              // абсолютный путь к папке
    std::wstring label;             // человекочитаемое имя для лога
    unsigned     minAgeMinutes;     // не трогать файлы моложе N минут
    bool         enabled;           // можно выключить из конфига
};