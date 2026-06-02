set(CONFIG_LV_BUILD_EXAMPLES ON)
set(CONFIG_LV_BUILD_DEMOS ON)
set(LV_BUILD_CONF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include/lv_conf.h CACHE FILEPATH "LVGL configuration file" FORCE)

add_subdirectory(lvgl)

include_directories(include)
include_directories(lvgl)
include_directories(lvgl/demos)

add_custom_target(sw ALL
     DEPENDS
          cs122_demo
)

add_definitions(-DBUILD_LVGL=1)
add_executable(cs122_demo
     src/main.cpp
     src/spi_display.cpp
     src/cs122_app.cpp
     src/CRSFReader.cpp
     src/PPMReader.cpp
     src/PPMEncoder.cpp
     src/lvgl_touch.cpp
     src/lvgl_clock.cpp
     src/lvgl_channel_monitor.cpp
     src/lvgl_pie_chart.cpp
     src/lvgl_compass.cpp
     src/lvgl_arc_label.cpp
     src/lvgl_heart_rate.cpp
     src/lvgl_demo_widgets.cpp
)

# pull in common dependencies and additional spi hardware support
target_link_libraries(cs122_demo pico_stdlib hardware_uart hardware_spi hardware_adc pico_cyw43_arch_none lvgl::lvgl)

# Route diagnostics over USB CDC. UART0 is used by the CRSF monitor.
pico_enable_stdio_uart(cs122_demo 0)
pico_enable_stdio_usb(cs122_demo 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(cs122_demo)
