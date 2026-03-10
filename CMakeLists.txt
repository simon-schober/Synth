cmake_minimum_required(VERSION 3.20)
project(SynthPOC)

find_package(imgui CONFIG REQUIRED)

add_executable(SynthPOC 
    "src/main.cpp" 
    "src/helpers.h" "src/helpers.cpp"
    "src/audio.h" "src/audio.cpp"
)

target_link_libraries(SynthPOC PRIVATE imgui::imgui d3d12 dxgi d3dcompiler user32)
target_compile_features(SynthPOC PRIVATE cxx_std_20)

# Optional: Add src/ to include path so you can use #include "helpers.h" instead of #include "src/helpers.h"
target_include_directories(SynthPOC PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")