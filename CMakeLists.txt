cmake_minimum_required(VERSION 3.13...3.27)

include(pico_sdk_import.cmake)

project(cnc_jogwheel C CXX ASM)
pico_sdk_init()

add_executable(${PROJECT_NAME}
  src/main.c
  src/io.c
  )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc) 

target_link_libraries(${PROJECT_NAME} pico_stdlib hardware_adc)

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(${PROJECT_NAME} pico_cyw43_arch_none)
endif()

# enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

set_property(TARGET ${PROJECT_NAME} PROPERTY COMPILE_WARNING_AS_ERROR ON)

pico_add_extra_outputs(${PROJECT_NAME})
