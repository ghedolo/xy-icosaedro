cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../pico-sdk)
include(pico_sdk_import.cmake)

project(icosaedro C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(icosaedro
    main.c
    renderer.c
)

pico_generate_pio_header(icosaedro ${CMAKE_CURRENT_LIST_DIR}/audio_i2s.pio)

target_link_libraries(icosaedro
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_gpio
    hardware_clocks
)

pico_enable_stdio_usb(icosaedro 1)
pico_enable_stdio_uart(icosaedro 0)

pico_add_extra_outputs(icosaedro)

add_custom_target(flash
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/flash.sh ${CMAKE_CURRENT_BINARY_DIR}/icosaedro.uf2
    DEPENDS icosaedro
    COMMENT "Flash icosaedro.uf2 via 1200-baud USB reset"
)
