cmake_minimum_required(VERSION 3.22)

project(robot_arm_stm32_bootstrap LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

message(STATUS "Configuring V0 firmware baseline with freestanding startup and linker support.")

set(FIRMWARE_TARGET robot_arm_bootstrap)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/core/linker_script.ld)
set(MAP_FILE ${CMAKE_CURRENT_BINARY_DIR}/${FIRMWARE_TARGET}.map)

add_executable(${FIRMWARE_TARGET}
    app/main.c
    bsp/board_nucleo_f767zi.c
    bsp/bsp_gpio.c
    bsp/bsp_i2c.c
    bsp/bsp_uart.c
    core/startup.c
    drivers/pca9685/pca9685.c
    core/system_stm32f7xx.c
)

set_target_properties(${FIRMWARE_TARGET} PROPERTIES
    SUFFIX .elf
)

target_include_directories(${FIRMWARE_TARGET} PRIVATE
    app
    bsp
    comms
    core
    drivers/pca9685
    drivers/servo
    robot
)

target_compile_definitions(${FIRMWARE_TARGET} PRIVATE
    STM32F767xx
)

target_compile_options(${FIRMWARE_TARGET} PRIVATE
    -mcpu=cortex-m7
    -mthumb
    -ffreestanding
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

target_link_options(${FIRMWARE_TARGET} PRIVATE
    -mcpu=cortex-m7
    -mthumb
    -nostdlib
    -Wl,--gc-sections
    -Wl,--build-id=none
    -Wl,-Map=${MAP_FILE}
    -T${LINKER_SCRIPT}
)

target_link_libraries(${FIRMWARE_TARGET} PRIVATE
    gcc
)

if(CMAKE_SIZE)
    add_custom_command(TARGET ${FIRMWARE_TARGET} POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${FIRMWARE_TARGET}>
        COMMENT "Printing firmware size"
    )
endif()