# Cleaner_utility

![CI](https://github.com/Junior2114/cleaner-utility/actions/workflows/ci.yml/badge.svg)

Лёгкий чистильщик временных файлов Windows. Очищает `%TEMP%` и
`C:\Windows\Temp`, умеет работать в режиме предпросмотра и с
пользовательскими директориями.

Написан на C++17, использует WinAPI и `std::filesystem`.
Собирается MSVC + CMake. После установки доступен как PowerShell-команда
`cleaner` с автодополнением.

---

## Содержание

- [Возможности](#возможности)
- [Требования](#требования)
- [Установка](#установка)
  - [Вариант 1. Из готового релиза](#вариант-1-из-готового-релиза)
  - [Вариант 2. Из исходников](#вариант-2-из-исходников)
- [Использование](#использование)
- [Опции exe](#опции-exe)
- [Как это работает](#как-это-работает)
- [Удаление](#удаление)
- [Сборка вручную](#сборка-вручную)
- [Структура проекта](#структура-проекта)
- [Безопасность](#безопасность)
- [Известные ограничения](#известные-ограничения)
- [FAQ](#faq)
- [Лицензия](#лицензия)

---

## Возможности

- Очистка `%TEMP%` (пользовательский temp) и `C:\Windows\Temp`
- Флаг `--dry` — предпросмотр без удаления
- Флаг `--min-age N` — не трогать файлы свежее N минут
- Флаг `--path DIR` — своя директория вместо стандартных (можно повторять)
- Логирование в `cleaner.log`
- PowerShell-команда `cleaner` с автодополнением действий и опций
- Автоматический подъём UAC при запуске очистки
- Аккуратная установка: не затирает `$PROFILE`, использует маркеры
- Корректная работа с UTF-8 и русскими путями

---

## Требования

**Для запуска готового бинарника (Вариант 1):**

- Windows 10 / Server 2016 или новее
- PowerShell 5.1+ (встроен в Windows 10/11)

**Для сборки из исходников (Вариант 2):**

Всё выше плюс:

- [CMake](https://cmake.org/download/) 3.15 или новее
- Visual Studio Build Tools 2022 (тулсет v143) или 2026 (тулсет v145)
  - Скачать: <https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio>
  - В установщике выбрать компонент **«Desktop development with C++»**

> Проверить, что `cmake` доступен, можно командой `cmake --version`.

---

## Установка

Есть два пути. Выбирай тот, который удобнее.

### Вариант 1. Из готового релиза

Подходит, если не хочешь возиться со сборкой.

1. Открой страницу **Releases** репозитория.
2. Скачай архив `my-cleaner-<версия>-win64.zip`.
3. Распакуй в любую папку.
4. Запусти `install.cmd` (двойным кликом) — он сам обойдёт ограничения
   ExecutionPolicy и вызовет `install.ps1`.

   Либо, если предпочитаешь вручную:
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\install.ps1
   ```

5. Перезапусти PowerShell или выполни `. $PROFILE`.

### Вариант 2. Из исходников

Подходит, если хочешь собрать сам или внести изменения.

```powershell
git clone https://github.com/<Junior2114>/my-cleaner
cd my-cleaner
.\scripts\install.cmd
```

`install.cmd` вызовет `install.ps1` с обходом ExecutionPolicy.
Если хочешь вызвать напрямую:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1
```

Скрипт установки:

1. Проверит наличие `cmake` в `PATH`.
2. Сконфигурирует проект (`cmake -S . -B build`).
3. Соберёт Release (`cmake --build build --config Release`).
4. Установит exe и PowerShell-модуль в
   `%LOCALAPPDATA%\Programs\my-cleaner\bin\`.
5. Аккуратно подключит модуль в `$PROFILE.CurrentUserAllHosts`
   между маркерами:
   ```
   # >>> my-cleaner >>>
   . "C:\Users\<ТвоеИмя>\AppData\Local\Programs\my-cleaner\bin\cleaner.ps1"
   # <<< my-cleaner <<<
   ```

**Что НЕ делает `install.ps1`:**

- Не затирает `$PROFILE` — если там что-то есть, оно сохраняется.
- Не требует прав администратора — ставится в пользовательскую папку.
- Не плодит дубликаты при повторном запуске — старый блок удаляется.

### Параметры установщика

```powershell
.\scripts\install.ps1 [-InstallDir <путь>] [-NoBuild] [-NoProfile]
```

| Параметр | Описание |
|---|---|
| `-InstallDir` | Куда ставить. По умолчанию `%LOCALAPPDATA%\Programs\my-cleaner` |
| `-NoBuild`    | Не собирать, использовать существующий `build\Release\TempCleaner.exe` |
| `-NoProfile`  | Не трогать `$PROFILE`, только установить файлы |

Пример: поставить без правки профиля, чтобы подключить вручную:

```powershell
.\scripts\install.ps1 -NoProfile
```

Тогда `install.ps1` в конце подскажет строку, которую надо добавить в
`$PROFILE` самому.

---

## Использование

После установки и перезапуска PowerShell:

```powershell
cleaner run                 # очистка (UAC поднимется автоматически)
cleaner dry                 # предпросмотр — ничего не удалит
cleaner run --min-age 60    # не трогать файлы свежее часа
cleaner run --path "D:\Cache"
cleaner log                 # последние 60 строк cleaner.log
cleaner version             # версия и пути
cleaner help                # справка
```

Все опции после действия (`run`, `dry`) передаются в сам `TempCleaner.exe`.
Их можно комбинировать:

```powershell
cleaner run --min-age 30 --path "C:\Build\obj" --path "D:\Cache"
cleaner dry --min-age 120
```

### Действия команды `cleaner`

| Действие | Описание |
|---|---|
| `cleaner run`     | Запуск очистки. UAC поднимется автоматически |
| `cleaner dry`     | Предпросмотр. Ничего не удаляется, даже с опасными опциями |
| `cleaner log`     | Последние 60 строк `cleaner.log` |
| `cleaner version` | Версия, путь к exe, путь к модулю |
| `cleaner help`    | Справка |

---

## Опции exe

Опции передаются после действия `cleaner run` / `cleaner dry`.

| Опция | Описание | По умолчанию |
|---|---|---|
| `--dry`        | Только показать, что было бы удалено. Ничего не удаляет | выкл. |
| `--min-age N`  | Пропускать файлы, изменённые менее N минут назад | `10` |
| `--path DIR`   | Добавить директорию вместо стандартных. Можно повторять | `%TEMP%` + `C:\Windows\Temp` |
| `--help`, `-h` | Справка самого exe | — |

**Важно про `--path`:** если указана хотя бы одна `--path`, **стандартные цели
не добавляются**. Обрабатываются только те папки, что ты явно перечислил.
Чтобы включить и стандартные, и свою:

```powershell
cleaner run --path "$env:TEMP" --path "C:\Windows\Temp" --path "D:\Cache"
```

---

## Как это работает

`TempCleaner.exe` обходит указанные директории рекурсивно. Для каждого файла:

1. Проверяет возраст (`last_write_time`). Если моложе `--min-age` — пропускает.
2. Пытается удалить через `DeleteFileW`.
3. Если файл занят (`ERROR_SHARING_VIOLATION`, `ERROR_ACCESS_DENIED`) —
   увеличивает счётчик `Skipped` и пишет `WARN` в лог.
4. После обхода удаляет **пустые** подкаталоги (снизу вверх).

Симлинки и junction points **не разыменовываются** — в них не заходим,
их не трогаем.

В конце выводится итог:

```
===== Result =====
Deleted : 4213
Skipped : 19
Freed   : 812 MB
```

---

## Удаление

```powershell
& "$env:LOCALAPPDATA\Programs\my-cleaner\bin\uninstall.ps1"
```

Или, если репозиторий ещё на месте:

```powershell
.\scripts\uninstall.ps1
```

Скрипт:

1. Удалит блок между маркерами из `$PROFILE`.
2. Удалит папку `%LOCALAPPDATA%\Programs\my-cleaner`.

Перезапусти PowerShell — команда `cleaner` исчезнет.

Если хочешь оставить профиль нетронутым:

```powershell
.\scripts\uninstall.ps1 -KeepProfile
```

---

## Сборка вручную

Если не хочешь использовать `install.ps1`, собери и запусти вручную.

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Exe появится в `build\Release\TempCleaner.exe`. Запускать можно оттуда:

```powershell
.\build\Release\TempCleaner.exe --dry
.\build\Release\TempCleaner.exe --min-age 60
```

Установка в произвольную папку:

```powershell
cmake --install build --config Release --prefix C:\Tools\my-cleaner
```

Установить задачу в планировщик (по желанию, вручную):

```powershell
$exe = "$env:LOCALAPPDATA\Programs\my-cleaner\bin\TempCleaner.exe"
$action  = New-ScheduledTaskAction -Execute $exe -Argument '--min-age 1440'
$trigger = New-ScheduledTaskTrigger -Weekly -DaysOfWeek Sunday -At 3am
Register-ScheduledTask -TaskName 'my-cleaner-weekly' -Action $action -Trigger $trigger
```

---

## Структура проекта

```
my-cleaner/
├── src/
│   ├── main.cpp              # точка входа, разбор аргументов
│   ├── Cleaner.h             # класс Cleaner
│   ├── Cleaner.cpp           # обход директорий, удаление
│   ├── Logger.h              # простой логгер с уровнями
│   └── Logger.cpp            # запись в файл + stdout
├── scripts/
│   ├── cleaner.ps1.in        # шаблон PowerShell-модуля (CMake подставит версию)
│   ├── install.ps1           # установщик (сборка + установка + правка профиля)
│   ├── install.cmd           # обход ExecutionPolicy, запуск install.ps1
│   └── uninstall.ps1         # удаление из профиля и с диска
├── CMakeLists.txt
├── README.md
├── LICENSE
└── .gitignore
```

---

## Безопасность

- По умолчанию файлы моложе 10 минут не трогаются — это защита от удаления
  временных файлов **работающих прямо сейчас** приложений (браузеры, IDE,
  инсталляторы).
- `--dry` ничего не удаляет, даже если указаны другие опции.
- Занятые файлы пропускаются и логируются как `WARN`.
- Симлинки и junction points не разыменовываются.
- Пустые подкаталоги удаляются **после** файлов, снизу вверх.
- Установщик не требует прав администратора (ставит в `%LOCALAPPDATA%`).
- UAC поднимается **только при запуске очистки**, потому что для
  `C:\Windows\Temp` нужны права админа.

> **Всегда начинай с `cleaner dry`.** Просмотри список того, что было бы
> удалено. Только после этого запускай `cleaner run`.

---

## Известные ограничения

- **Занятые файлы не удаляются.** Если файл открыт другим процессом
  без `FILE_SHARE_DELETE`, Windows не даст его удалить. Утилита корректно
  пропускает такие файлы и пишет `WARN` в лог.
- **`C:\Windows\Temp` без админа.** Если запустить exe без прав
  администратора, файлы из системного temp будут пропускаться
  с кодом `ERROR_ACCESS_DENIED`. Модуль `cleaner.ps1` поднимает UAC
  автоматически, но если запускать exe напрямую — UAC не поднимется.
- **Нет отложенного удаления.** Файлы, которые нельзя удалить сейчас,
  не помечаются на удаление при перезагрузке. Пропускаются.
- **Нет подписи кода.** Бинарник не подписан сертификатом. SmartScreen
  может показать предупреждение при первом запуске — это нормально для
  опенсорсных утилит. Нажми «Подробнее» → «Выполнить в любом случае».

---

## FAQ

**`.\install.ps1` не запускается, красная ошибка про ExecutionPolicy.**

Windows по умолчанию запрещает выполнение `.ps1`-файлов. Запусти через
`.cmd`-обёртку или явно:

```powershell
.\scripts\install.cmd
# или
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1
```

**SmartScreen пишет «Windows protected your PC».**

Утилита не подписана сертификатом. Нажми «More info» → «Run anyway».
Либо собери сам из исходников — тогда вообще никаких предупреждений не будет.

**`cleaner: command not found` после установки.**

Профиль не перезагружен. Выполни:

```powershell
. $PROFILE
```

Или перезапусти PowerShell.

**`cleaner run` не может очистить `C:\Windows\Temp`.**

Значит UAC не поднялся. Проверь, что ты не запускаешь exe напрямую
в обход модуля. Команда `cleaner run` (не `.\TempCleaner.exe run`)
поднимает UAC автоматически.

**Хочу добавить свою папку в очистку.**

```powershell
cleaner run --path "D:\Cache" --path "E:\Games\cache"
```

Помни: `--path` **заменяет** стандартные цели. Если нужны и стандартные —
перечисли их явно.

**Можно запускать по расписанию?**

Да. Собери exe, зарегистрируй задачу в Планировщике задач Windows
(пример — в разделе «Сборка вручную»). Обязательно используй флаг
`--min-age` с запасом (например, `1440` — сутки), чтобы не удалить
файлы активных приложений.

**Утилита удалила что-то нужное. Можно восстановить?**

Нет. Файлы удаляются через `DeleteFileW` — без корзины. Всегда
начинай с `cleaner dry` и внимательно смотри список.

**Сколько это занимает места?**

Exe ~200 KB. Модуль `cleaner.ps1` ~10 KB. Никаких зависимостей.

---

## Лицензия

MIT. См. [LICENSE](LICENSE).