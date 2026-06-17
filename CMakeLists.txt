cmake_minimum_required(VERSION 3.20)

project(TheCircle
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TC_DEBUG_MODE "Enable debug overlay" ON)

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

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)
