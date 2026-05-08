set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(ARM_NONE_EABI_GCC arm-none-eabi-gcc)
if(NOT ARM_NONE_EABI_GCC)
    message(FATAL_ERROR "arm-none-eabi-gcc was not found in PATH.")
endif()

find_program(ARM_NONE_EABI_AR arm-none-eabi-ar)
find_program(ARM_NONE_EABI_RANLIB arm-none-eabi-ranlib)
find_program(ARM_NONE_EABI_OBJCOPY arm-none-eabi-objcopy)
find_program(ARM_NONE_EABI_SIZE arm-none-eabi-size)

set(CMAKE_C_COMPILER "${ARM_NONE_EABI_GCC}")
set(CMAKE_ASM_COMPILER "${ARM_NONE_EABI_GCC}")

if(ARM_NONE_EABI_AR)
    set(CMAKE_AR "${ARM_NONE_EABI_AR}")
endif()

if(ARM_NONE_EABI_RANLIB)
    set(CMAKE_RANLIB "${ARM_NONE_EABI_RANLIB}")
endif()

if(ARM_NONE_EABI_OBJCOPY)
    set(CMAKE_OBJCOPY "${ARM_NONE_EABI_OBJCOPY}" CACHE FILEPATH "objcopy path")
endif()

if(ARM_NONE_EABI_SIZE)
    set(CMAKE_SIZE "${ARM_NONE_EABI_SIZE}" CACHE FILEPATH "size tool path")
endif()

set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m7 -mthumb")
set(CMAKE_ASM_FLAGS_INIT "-mcpu=cortex-m7 -mthumb")