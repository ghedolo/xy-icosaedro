set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(XPACK_DIR "${CMAKE_CURRENT_LIST_DIR}/../xpack-arm-none-eabi-gcc-15.2.1-1.1/bin")

set(CMAKE_C_COMPILER   "${XPACK_DIR}/arm-none-eabi-gcc" CACHE STRING "")
set(CMAKE_CXX_COMPILER "${XPACK_DIR}/arm-none-eabi-g++" CACHE STRING "")
set(CMAKE_ASM_COMPILER "${XPACK_DIR}/arm-none-eabi-gcc" CACHE STRING "")
set(CMAKE_AR           "${XPACK_DIR}/arm-none-eabi-ar"  CACHE STRING "")
set(CMAKE_RANLIB       "${XPACK_DIR}/arm-none-eabi-ranlib" CACHE STRING "")
set(CMAKE_OBJCOPY      "${XPACK_DIR}/arm-none-eabi-objcopy" CACHE STRING "")
set(CMAKE_SIZE         "${XPACK_DIR}/arm-none-eabi-size" CACHE STRING "")

set(CMAKE_C_FLAGS_INIT   "-mcpu=cortex-m0plus -mthumb")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb")
set(CMAKE_ASM_FLAGS_INIT "-mcpu=cortex-m0plus -mthumb")

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
