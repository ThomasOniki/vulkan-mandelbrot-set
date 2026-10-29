cmake_minimum_required(VERSION 3.24)
project(vulkan-mandelbrot-set C)

file(GLOB SOURCE_FILES "src/*.c")
add_executable(vulkan-mandelbrot-set "${SOURCE_FILES}")

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

target_link_libraries(vulkan-mandelbrot-set PRIVATE Vulkan::Vulkan glfw)
target_precompile_headers(vulkan-mandelbrot-set PRIVATE "src/pch.h")

set_target_properties(vulkan-mandelbrot-set PROPERTIES C_STANDARD 17)
set_target_properties(vulkan-mandelbrot-set PROPERTIES C_STANDARD_REQUIRED ON)
set_target_properties(vulkan-mandelbrot-set PROPERTIES C_EXTENSIONS OFF)
