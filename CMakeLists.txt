cmake_minimum_required(VERSION 3.20)
project(vulkan_compute_shader)

find_package(Vulkan REQUIRED)

add_executable(vulkan_compute_shader "main.cpp")
target_link_libraries(vulkan_compute_shader PRIVATE Vulkan::Vulkan)
set_property(TARGET vulkan_compute_shader PROPERTY CXX_STANDARD 17)