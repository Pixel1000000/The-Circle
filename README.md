# The-Circle

2D roguelite prototype on SFML.

## Build with Conan

```powershell
conan install . --output-folder=build --build=missing -pr:h conan/profiles/windows-msvc -pr:b conan/profiles/windows-msvc -r conan-center
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="build/conan_toolchain.cmake"
cmake --build build
```

If Conan cannot reach `conancenter` because of SSL certificate validation on Windows/Python, fix the local certificate chain first and rerun `conan install`.
