# The-Circle

2D roguelite prototype on SFML.

## Для игрока (просто запустить готовую сборку)

1. Скачайте zip-архив со сборкой игры (`build.zip` / релизный архив).
2. Распакуйте архив - внутри будет папка `build`. Можно разместить её куда
   угодно, переносить из архива не обязательно ничего, кроме самой папки
   `build`.
3. Зайдите в папку `build` и запустите `TheCircle.exe`.

### Требования к софту

Для запуска нужен **Microsoft Visual C++ Redistributable (x64), версии
2015-2022**. Если при запуске `TheCircle.exe` появляется ошибка про
отсутствующие `.dll` (например, `VCRUNTIME140.dll` или `MSVCP140.dll`),
установите его с официального сайта Microsoft:

https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist

(прямая ссылка на установщик x64:
https://aka.ms/vs/17/release/vc_redist.x64.exe)

После установки перезапустите `TheCircle.exe`.

## Быстрый старт для разработчика (Windows)

Запустите из обычного PowerShell (не нужен "x64 Native Tools Command Prompt"):

```
.\setup.ps1
.\setup.ps1 -Configuration Debug
```

Скрипт сам проверит, что установлены Conan 2.x, CMake >= 3.20, Ninja и Visual
Studio с компонентом C++; подберёт подходящий профиль Conan под установленную
у вас версию MSVC, поставит зависимости и сделает первую сборку
(`build\TheCircle.exe`). Если чего-то не хватает, скрипт остановится и выведет
список того, что нужно установить.

Conan ставит зависимости отдельно под каждую конфигурацию (`Release`/`Debug`),
поэтому `setup.ps1` нужно один раз выполнить под каждой конфигурацией, которую
собираетесь собирать (по умолчанию `-Configuration Release`).

Debug-сборка (`-Configuration Debug`) включает оверлей отладки (см. ниже) -
он отсутствует в Release.

## Пересборка после изменений

Для последующих пересборок (после `setup.ps1` под нужной конфигурацией) можно
не повторять проверку инструментов и `conan install` - используйте:

```
.\build.ps1
.\build.ps1 -Run            # собрать и сразу запустить TheCircle.exe
.\build.ps1 -Configuration Debug
```

Если для выбранной `-Configuration` ещё не выполняли `setup.ps1` с этой же
конфигурацией, `build.ps1` остановится и подскажет, что запустить
(`.\setup.ps1 -Configuration <Release|Debug>`).

## Дебаг-оверлей (Debug Room)

В Debug-сборке (`TC_DEBUG_MODE=ON`, включается автоматически при
`-Configuration Debug`) во время игры доступна клавиша **F1** - она ставит
игру на паузу и открывает оверлей отладки с табами Spawn / Player / World /
Display (спавн врагов и боссов, чит-настройки игрока, прогрессия мира,
визуальные дебаг-флаги). Повторное нажатие F1 или кнопка `×` закрывают
оверлей. В Release-сборке весь этот код не компилируется.

Debug-сборка собирается с обычным (Release) CRT, а не debug-вариантом
(`MSVCP140D.dll` и т.п.) - такой debug-CRT ставится только вместе с Visual
Studio и не входит в стандартный VC++ Redistributable. Поэтому Debug-сборку
можно передать тестеру так же, как Release - ему достаточно того же
redistributable из раздела "Для игрока" выше.

## Build with Conan (вручную)

### 1. Установить зависимости (один раз, или при изменении conanfile.txt)
conan install . --output-folder=build --build=missing -pr:h conan/profiles/windows-msvc -pr:b conan/profiles/windows-msvc -s build_type=Release -c tools.cmake.cmaketoolchain:generator=Ninja -c tools.microsoft.msbuild:vs_version=18

### 2. Открыть "x64 Native Tools Command Prompt for VS 2026" (или вызвать vcvarsall.bat x64), затем:
cmake --preset conan-release
cmake --build --preset conan-release

### 3. Запустить
build\TheCircle.exe



If Conan cannot reach `conancenter` because of SSL certificate validation on Windows/Python, fix the local certificate chain first and rerun `conan install`.
