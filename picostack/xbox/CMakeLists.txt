cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico2_w)

include(pico_sdk_import.cmake)
project(xboxpico C CXX ASM)
pico_sdk_init()
add_compile_definitions(CYW43_ENABLE_BLUETOOTH=1)
add_executable(xboxpico xboxpico.c picostack.c btlibp.c)
target_compile_definitions(xboxpico PRIVATE PICO_STACK_SIZE=4096)
target_link_libraries(xboxpico pico_stdlib)
target_link_libraries(xboxpico pico_cyw43_arch_none)
pico_enable_stdio_usb(xboxpico 1)
pico_enable_stdio_uart(xboxpico 0)
pico_add_extra_outputs(xboxpico)

