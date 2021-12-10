cmake_minimum_required(VERSION 3.20)
project(vulkan_compute_shader)

find_package(Vulkan REQUIRED)

add_executable(vulkan_compute_shader "main.cpp")
add_custom_command(
TARGET vulkan_compute_shader
COMMAND glslangValidator ${CMAKE_SOURCE_DIR}/add.comp -V
DEPENDS "${CMAKE_SOURCE_DIR}/add.comp"
)

add_library(vulkan_helper vulkan_helper.cc)
target_link_libraries(vulkan_helper PUBLIC Vulkan::Vulkan)
set_property(TARGET vulkan_helper PROPERTY CXX_STANDARD 17)

target_link_libraries(vulkan_compute_shader PRIVATE Vulkan::Vulkan vulkan_helper)
set_property(TARGET vulkan_compute_shader PROPERTY CXX_STANDARD 17)