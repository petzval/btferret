cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico2_w)

include(pico_sdk_import.cmake)
project(btfpico C CXX ASM)
pico_sdk_init()
add_compile_definitions(CYW43_ENABLE_BLUETOOTH=1)
add_executable(btfpico btfpico.c)
target_link_libraries(btfpico pico_stdlib)
target_link_libraries(btfpico pico_cyw43_arch_none)

# btstack is not used, but if this causes compile to fail
# uncomment the following four instructions to link btstack
# and uncomment #define USE_BTSTACK in btfpico.c

# add_compile_definitions(CYW43_DISABLE_BT_INIT=1)  
# target_link_libraries(btfpico pico_btstack_cyw43)
# target_link_libraries(btfpico pico_btstack_ble)
# target_include_directories(btfpico PRIVATE ${CMAKE_CURRENT_LIST_DIR})


pico_enable_stdio_usb(btfpico 1)
pico_enable_stdio_uart(btfpico 0)
pico_add_extra_outputs(btfpico)

