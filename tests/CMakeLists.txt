set(SERVO_UNIT_TEST_TARGET servo_unit_tests)
set(DEBUG_COMMAND_HANDLER_UNIT_TEST_TARGET debug_command_handler_unit_tests)
set(DEBUG_COMMAND_PARSER_UNIT_TEST_TARGET debug_command_parser_unit_tests)
set(DEBUG_COMMAND_SHELL_UNIT_TEST_TARGET debug_command_shell_unit_tests)
set(ROBOT_ARM_UNIT_TEST_TARGET robot_arm_unit_tests)
set(ROBOT_STARTUP_UNIT_TEST_TARGET robot_startup_unit_tests)
set(BOOT_SELF_TEST_UNIT_TEST_TARGET boot_self_test_unit_tests)
set(DEBUG_COMMAND_INTEGRATION_TEST_TARGET debug_command_integration_tests)

add_executable(${SERVO_UNIT_TEST_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_servo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes/pca9685_fake.c
    ${CMAKE_SOURCE_DIR}/drivers/servo/servo.c
)

add_executable(${DEBUG_COMMAND_PARSER_UNIT_TEST_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_debug_command_parser.c
    ${CMAKE_SOURCE_DIR}/app/debug_command_parser.c
)

add_executable(${DEBUG_COMMAND_HANDLER_UNIT_TEST_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_debug_command_handler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes/pca9685_fake.c
    ${CMAKE_SOURCE_DIR}/app/debug_command_handler.c
    ${CMAKE_SOURCE_DIR}/app/debug_command_parser.c
    ${CMAKE_SOURCE_DIR}/drivers/servo/servo.c
    ${CMAKE_SOURCE_DIR}/robot/robot_arm.c
    ${CMAKE_SOURCE_DIR}/robot/robot_arm_joint_calibration.c
)

add_executable(${DEBUG_COMMAND_SHELL_UNIT_TEST_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_debug_command_shell.c
    ${CMAKE_SOURCE_DIR}/app/debug_command_parser.c
    ${CMAKE_SOURCE_DIR}/app/debug_command_shell.c
)

add_executable(${ROBOT_ARM_UNIT_TEST_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_robot_arm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes/pca9685_fake.c
    ${CMAKE_SOURCE_DIR}/drivers/servo/servo.c
    ${CMAKE_SOURCE_DIR}/robot/robot_arm.c
    ${CMAKE_SOURCE_DIR}/robot/robot_arm_joint_calibration.c
)

add_executable(${ROBOT_STARTUP_UNIT_TEST_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_robot_startup.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes/pca9685_fake.c
    ${CMAKE_SOURCE_DIR}/app/robot_startup.c
    ${CMAKE_SOURCE_DIR}/drivers/servo/servo.c
    ${CMAKE_SOURCE_DIR}/robot/robot_arm.c
    ${CMAKE_SOURCE_DIR}/robot/robot_arm_joint_calibration.c
)

add_executable(${BOOT_SELF_TEST_UNIT_TEST_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_boot_self_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes/pca9685_fake.c
    ${CMAKE_SOURCE_DIR}/app/boot_self_test.c
    ${CMAKE_SOURCE_DIR}/app/runtime_log.c
    ${CMAKE_SOURCE_DIR}/drivers/servo/servo.c
    ${CMAKE_SOURCE_DIR}/robot/robot_arm.c
    ${CMAKE_SOURCE_DIR}/robot/robot_arm_joint_calibration.c
)

target_compile_definitions(${BOOT_SELF_TEST_UNIT_TEST_TARGET} PRIVATE
    RUNTIME_LOG_MIN_LEVEL=${ROBOT_ARM_RUNTIME_LOG_MIN_LEVEL_DEFINE}
)

add_executable(${DEBUG_COMMAND_INTEGRATION_TEST_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_debug_command_integration.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes/pca9685_fake.c
    ${CMAKE_SOURCE_DIR}/app/debug_command_handler.c
    ${CMAKE_SOURCE_DIR}/app/debug_command_parser.c
    ${CMAKE_SOURCE_DIR}/app/debug_command_shell.c
    ${CMAKE_SOURCE_DIR}/drivers/servo/servo.c
    ${CMAKE_SOURCE_DIR}/robot/robot_arm.c
    ${CMAKE_SOURCE_DIR}/robot/robot_arm_joint_calibration.c
)

target_include_directories(${SERVO_UNIT_TEST_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes
    ${CMAKE_SOURCE_DIR}/bsp
    ${CMAKE_SOURCE_DIR}/drivers/pca9685
    ${CMAKE_SOURCE_DIR}/drivers/servo
)

target_include_directories(${DEBUG_COMMAND_PARSER_UNIT_TEST_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/bsp
    ${CMAKE_SOURCE_DIR}/drivers/pca9685
    ${CMAKE_SOURCE_DIR}/drivers/servo
    ${CMAKE_SOURCE_DIR}/robot
)

target_include_directories(${DEBUG_COMMAND_HANDLER_UNIT_TEST_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/bsp
    ${CMAKE_SOURCE_DIR}/drivers/pca9685
    ${CMAKE_SOURCE_DIR}/drivers/servo
    ${CMAKE_SOURCE_DIR}/robot
)

target_include_directories(${DEBUG_COMMAND_SHELL_UNIT_TEST_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/bsp
    ${CMAKE_SOURCE_DIR}/drivers/pca9685
    ${CMAKE_SOURCE_DIR}/drivers/servo
    ${CMAKE_SOURCE_DIR}/robot
)

target_include_directories(${ROBOT_ARM_UNIT_TEST_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/bsp
    ${CMAKE_SOURCE_DIR}/drivers/pca9685
    ${CMAKE_SOURCE_DIR}/drivers/servo
    ${CMAKE_SOURCE_DIR}/robot
)

target_include_directories(${ROBOT_STARTUP_UNIT_TEST_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/bsp
    ${CMAKE_SOURCE_DIR}/drivers/pca9685
    ${CMAKE_SOURCE_DIR}/drivers/servo
    ${CMAKE_SOURCE_DIR}/robot
)

target_include_directories(${BOOT_SELF_TEST_UNIT_TEST_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/bsp
    ${CMAKE_SOURCE_DIR}/drivers/pca9685
    ${CMAKE_SOURCE_DIR}/drivers/servo
    ${CMAKE_SOURCE_DIR}/robot
)

target_include_directories(${DEBUG_COMMAND_INTEGRATION_TEST_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/bsp
    ${CMAKE_SOURCE_DIR}/drivers/pca9685
    ${CMAKE_SOURCE_DIR}/drivers/servo
    ${CMAKE_SOURCE_DIR}/robot
)

if(MSVC)
    target_compile_options(${SERVO_UNIT_TEST_TARGET} PRIVATE /W4 /WX)
    target_compile_options(${DEBUG_COMMAND_HANDLER_UNIT_TEST_TARGET} PRIVATE /W4 /WX)
    target_compile_options(${DEBUG_COMMAND_PARSER_UNIT_TEST_TARGET} PRIVATE /W4 /WX)
    target_compile_options(${DEBUG_COMMAND_SHELL_UNIT_TEST_TARGET} PRIVATE /W4 /WX)
    target_compile_options(${ROBOT_ARM_UNIT_TEST_TARGET} PRIVATE /W4 /WX)
    target_compile_options(${ROBOT_STARTUP_UNIT_TEST_TARGET} PRIVATE /W4 /WX)
    target_compile_options(${BOOT_SELF_TEST_UNIT_TEST_TARGET} PRIVATE /W4 /WX)
    target_compile_options(${DEBUG_COMMAND_INTEGRATION_TEST_TARGET} PRIVATE /W4 /WX)
else()
    target_compile_options(${SERVO_UNIT_TEST_TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(${DEBUG_COMMAND_HANDLER_UNIT_TEST_TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(${DEBUG_COMMAND_PARSER_UNIT_TEST_TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(${DEBUG_COMMAND_SHELL_UNIT_TEST_TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(${ROBOT_ARM_UNIT_TEST_TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(${ROBOT_STARTUP_UNIT_TEST_TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(${BOOT_SELF_TEST_UNIT_TEST_TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(${DEBUG_COMMAND_INTEGRATION_TEST_TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME ${SERVO_UNIT_TEST_TARGET} COMMAND ${SERVO_UNIT_TEST_TARGET})
add_test(NAME ${DEBUG_COMMAND_HANDLER_UNIT_TEST_TARGET} COMMAND ${DEBUG_COMMAND_HANDLER_UNIT_TEST_TARGET})
add_test(NAME ${DEBUG_COMMAND_PARSER_UNIT_TEST_TARGET} COMMAND ${DEBUG_COMMAND_PARSER_UNIT_TEST_TARGET})
add_test(NAME ${DEBUG_COMMAND_SHELL_UNIT_TEST_TARGET} COMMAND ${DEBUG_COMMAND_SHELL_UNIT_TEST_TARGET})
add_test(NAME ${ROBOT_ARM_UNIT_TEST_TARGET} COMMAND ${ROBOT_ARM_UNIT_TEST_TARGET})
add_test(NAME ${ROBOT_STARTUP_UNIT_TEST_TARGET} COMMAND ${ROBOT_STARTUP_UNIT_TEST_TARGET})
add_test(NAME ${BOOT_SELF_TEST_UNIT_TEST_TARGET} COMMAND ${BOOT_SELF_TEST_UNIT_TEST_TARGET})
add_test(NAME ${DEBUG_COMMAND_INTEGRATION_TEST_TARGET} COMMAND ${DEBUG_COMMAND_INTEGRATION_TEST_TARGET})
set_tests_properties(${SERVO_UNIT_TEST_TARGET} PROPERTIES LABELS "unit")
set_tests_properties(${DEBUG_COMMAND_HANDLER_UNIT_TEST_TARGET} PROPERTIES LABELS "unit")
set_tests_properties(${DEBUG_COMMAND_PARSER_UNIT_TEST_TARGET} PROPERTIES LABELS "unit")
set_tests_properties(${DEBUG_COMMAND_SHELL_UNIT_TEST_TARGET} PROPERTIES LABELS "unit")
set_tests_properties(${ROBOT_ARM_UNIT_TEST_TARGET} PROPERTIES LABELS "unit")
set_tests_properties(${ROBOT_STARTUP_UNIT_TEST_TARGET} PROPERTIES LABELS "unit")
set_tests_properties(${BOOT_SELF_TEST_UNIT_TEST_TARGET} PROPERTIES LABELS "unit")
set_tests_properties(${DEBUG_COMMAND_INTEGRATION_TEST_TARGET} PROPERTIES LABELS "integration")
