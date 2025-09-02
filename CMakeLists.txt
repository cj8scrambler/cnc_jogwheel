cmake_minimum_required(VERSION 3.13...3.27)

include(pico_sdk_import.cmake)

project(cnc_jogwheel C CXX ASM)
pico_sdk_init()

add_executable(${PROJECT_NAME}
  src/main.cpp
  )
target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_adc)
