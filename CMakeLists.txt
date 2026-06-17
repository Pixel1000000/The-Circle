cmake_minimum_required(VERSION 3.20)

project(TheCircle
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TC_DEBUG_MODE "Enable debug overlay" OFF)

find_package(SFML REQUIRED COMPONENTS Graphics Window System Audio)
find_package(EnTT REQUIRED)
find_package(nlohmann_json REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        src
)

if(TC_DEBUG_MODE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TC_DEBUG)
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
        EnTT::EnTT
        nlohmann_json::nlohmann_json
)

target_compile_features(${PROJECT_NAME}
    PRIVATE
        cxx_std_17
)

# Custom target with no OUTPUT is always considered out-of-date, so it
# re-runs on every build invocation even when Ninja finds nothing to
# recompile/relink for ${PROJECT_NAME} - otherwise a stale assets/ copy
# from an old build directory can silently shadow repo config changes
# (e.g. lang/*.json, enemies.json).
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)
add_dependencies(${PROJECT_NAME} copy_assets)
