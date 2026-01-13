set(common 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/common.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/common/config.c")
    
set(applications
    "${CMAKE_CURRENT_SOURCE_DIR}/src/applications/modbusTask.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/applications/mqtt/mqttTask.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/applications/networkTask.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/applications/screenTask.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/applications/ota/ota.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/applications/business/ledStripIndicationTask.c")

set(modules
    "${CMAKE_CURRENT_SOURCE_DIR}/src/modules/display/screen.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/modules/network/ethernet.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/modules/network/wireless.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/modules/storage/nvs_storage.c")

set(hardware
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hardware/ledstrip/ledstrip.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hardware/gpio/gpio_output.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hardware/gpio/gpio_input.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hardware/usb/hid_host.c")

file(GLOB pages_srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/modules/display/pages/*.c")
file(GLOB mqtt_srcs "${CMAKE_CURRENT_SOURCE_DIR}/src/applications/mqtt/types/*.c")

set(include_dirs
    "${CMAKE_CURRENT_LIST_DIR}/."
    "${CMAKE_CURRENT_LIST_DIR}/inc/.")

idf_component_register(SRCS "${common}" "${applications}" "${modules}" "${hardware}" "${pages_srcs}" "${mqtt_srcs}"                       
                       INCLUDE_DIRS "${include_dirs}"
                       REQUIRES ${requires})                       
