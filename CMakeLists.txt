cmake_minimum_required(VERSION 3.20)
project(vulkan_compute_shader)

find_package(Vulkan REQUIRED)

add_executable(vulkan_compute_shader "main.cpp")
add_custom_command(
TARGET vulkan_compute_shader
COMMAND glslangValidator ${CMAKE_SOURCE_DIR}/add.comp -V -o ${CMAKE_SOURCE_DIR}/add.spv
DEPENDS "${CMAKE_SOURCE_DIR}/add.comp"
)

add_custom_command(
TARGET vulkan_compute_shader
COMMAND glslangValidator ${CMAKE_SOURCE_DIR}/distance.comp -V -o ${CMAKE_SOURCE_DIR}/distance.spv
DEPENDS "${CMAKE_SOURCE_DIR}/distance.comp"
)

add_library(vulkan_helper vulkan_helper.cc)
target_link_libraries(vulkan_helper PUBLIC Vulkan::Vulkan)
set_property(TARGET vulkan_helper PROPERTY CXX_STANDARD 17)

target_link_libraries(vulkan_compute_shader PRIVATE Vulkan::Vulkan vulkan_helper)
set_property(TARGET vulkan_compute_shader PROPERTY CXX_STANDARD 17)


find_package(GTest REQUIRED)
add_executable(match_bench_mark match_test.cc)

target_link_libraries(match_bench_mark PUBLIC Vulkan::Vulkan vulkan_helper GTest::GTest)
set_property(TARGET match_bench_mark PROPERTY CXX_STANDARD 17)