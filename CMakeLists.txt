cmake_minimum_required(VERSION 3.13)

project(lv_number_flow)

add_library(lv_number_flow
    src/lv_numberflow.c
)

# 头文件目录
target_include_directories(lv_number_flow
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(lv_number_flow
    PUBLIC
        lvgl
)
