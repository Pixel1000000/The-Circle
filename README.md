# The-Circle

2D roguelite prototype on SFML.

## Быстрый старт (Windows)

Запустите из обычного PowerShell (не нужен "x64 Native Tools Command Prompt"):

```
.\setup.ps1
```

Скрипт сам проверит, что установлены Conan 2.x, CMake >= 3.20, Ninja и Visual
Studio с компонентом C++; подберёт подходящий профиль Conan под установленную
у вас версию MSVC, поставит зависимости и сделает первую сборку
(`build\TheCircle.exe`). Если чего-то не хватает, скрипт остановится и выведет
список того, что нужно установить.

## Пересборка после изменений

Для последующих пересборок (после `setup.ps1`) можно не повторять проверку
инструментов и `conan install` - используйте:

```
.\build.ps1
.\build.ps1 -Run            # собрать и сразу запустить TheCircle.exe
.\build.ps1 -Configuration Debug
```

## Build with Conan (вручную)

### 1. Установить зависимости (один раз, или при изменении conanfile.txt)
conan install . --output-folder=build --build=missing -pr:h conan/profiles/windows-msvc -pr:b conan/profiles/windows-msvc -s build_type=Release -c tools.cmake.cmaketoolchain:generator=Ninja -c tools.microsoft.msbuild:vs_version=18

### 2. Открыть "x64 Native Tools Command Prompt for VS 2026" (или вызвать vcvarsall.bat x64), затем:
cmake --preset conan-release
cmake --build --preset conan-release

### 3. Запустить
build\TheCircle.exe



If Conan cannot reach `conancenter` because of SSL certificate validation on Windows/Python, fix the local certificate chain first and rerun `conan install`.
