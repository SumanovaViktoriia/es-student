cmake_minimum_required(VERSION 3.15)

set(PICO_BOARD pico)
set(PICO_PLATFORM rp2040)

include(pico_sdk_import.cmake)

project(231_firmware_template)

pico_sdk_init()

add_compile_options(-Wall)

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/logging/log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/device/device.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stdio-text-protocol/stdio-text-protocol.c
    ${CMAKE_CURRENT_SOURCE_DIR}/api/api.c
    ${CMAKE_CURRENT_SOURCE_DIR}/led-task/led-task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/button-task/button-task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/pi-task/pi-task.c
    ${CMAKE_CURRENT_SOURCE_DIR}/profiling/profiling.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/logging
    ${CMAKE_CURRENT_SOURCE_DIR}/device
    ${CMAKE_CURRENT_SOURCE_DIR}/stdio-text-protocol
    ${CMAKE_CURRENT_SOURCE_DIR}/api
    ${CMAKE_CURRENT_SOURCE_DIR}/led-task
    ${CMAKE_CURRENT_SOURCE_DIR}/button-task
    ${CMAKE_CURRENT_SOURCE_DIR}/pi-task
    ${CMAKE_CURRENT_SOURCE_DIR}/profiling
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_unique_id
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    DEVICE_BOARD="${PICO_BOARD}"
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

pico_add_extra_outputs(${PROJECT_NAME})