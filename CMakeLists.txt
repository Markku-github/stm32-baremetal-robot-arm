cmake_minimum_required(VERSION 3.22)

project(robot_arm_stm32_bootstrap LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

message(STATUS "Configuring bootstrap compile target. Add startup code, linker script, and MCU-specific sources during V0.")

add_library(robot_arm_bootstrap OBJECT
    app/main.c
)

target_include_directories(robot_arm_bootstrap PRIVATE
    app
    bsp
    comms
    core
    drivers/pca9685
    drivers/servo
    robot
)

target_compile_definitions(robot_arm_bootstrap PRIVATE
    STM32F767xx
)

target_compile_options(robot_arm_bootstrap PRIVATE
    -mcpu=cortex-m7
    -mthumb
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wall
    -Wextra
    -Wpedantic
)