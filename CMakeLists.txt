cmake_minimum_required(VERSION 3.13)

project(lv_number_flow LANGUAGES C)

add_library(numberflow
    src/numberflow.c
    src/curves.c
)

# 头文件目录
target_include_directories(numberflow
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

option(NUMBERFLOW_BUILD_LVGL8 "Build the LVGL 8 integration" ON)
if(NUMBERFLOW_BUILD_LVGL8)
    add_library(lv_number_flow integration/lvgl8/lv_numberflow.c)
    target_include_directories(lv_number_flow PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/integration/lvgl8)
    target_link_libraries(lv_number_flow PUBLIC numberflow lvgl)
endif()
