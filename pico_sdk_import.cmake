# This is a copy of pico_sdk_import.cmake from the pico-sdk.
# It can be copied from $PICO_SDK_PATH/external/pico_sdk_import.cmake

if (DEFINED ENV{PICO_SDK_PATH} AND (NOT PICO_SDK_PATH))
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()

if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH not set. "
            "Set PICO_SDK_PATH via -DPICO_SDK_PATH= or PICO_SDK_PATH env variable.")
endif()

get_filename_component(PICO_SDK_PATH "${PICO_SDK_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
if (NOT EXISTS ${PICO_SDK_PATH})
    message(FATAL_ERROR "PICO_SDK_PATH '${PICO_SDK_PATH}' does not exist.")
endif()

set(PICO_SDK_INIT_CMAKE_FILE ${PICO_SDK_PATH}/pico_sdk_init.cmake)
if (NOT EXISTS ${PICO_SDK_INIT_CMAKE_FILE})
    message(FATAL_ERROR "Not found: ${PICO_SDK_INIT_CMAKE_FILE}\n"
            "Is '${PICO_SDK_PATH}' a valid pico-sdk directory?")
endif()

include(${PICO_SDK_INIT_CMAKE_FILE})
