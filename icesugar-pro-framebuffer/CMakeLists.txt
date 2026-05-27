cmake_minimum_required(VERSION 3.12)

set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350-arm-s)

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)
#include($HOME/.pico-sdk/sdk/2.2.0/pico_sdk_init.cmake)

# Pull in SDK (must be before project)
project(spi_display C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_subdirectory(sw)
add_subdirectory(hw)