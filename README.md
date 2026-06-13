# The-Circle

2D roguelite prototype on SFML.

## Build with Conan

### 1. Установить зависимости (один раз, или при изменении conanfile.txt)
conan install . --output-folder=build --build=missing -pr:h conan/profiles/windows-msvc -pr:b conan/profiles/windows-msvc -s build_type=Release -c tools.cmake.cmaketoolchain:generator=Ninja -c tools.microsoft.msbuild:vs_version=18

### 2. Открыть "x64 Native Tools Command Prompt for VS 2026" (или вызвать vcvarsall.bat x64), затем:
cmake --preset conan-release
cmake --build --preset conan-release

### 3. Запустить
build\TheCircle.exe



If Conan cannot reach `conancenter` because of SSL certificate validation on Windows/Python, fix the local certificate chain first and rerun `conan install`.
