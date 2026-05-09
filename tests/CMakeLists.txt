set(SERVO_UNIT_TEST_TARGET servo_unit_tests)

add_executable(${SERVO_UNIT_TEST_TARGET}
    ${CMAKE_CURRENT_SOURCE_DIR}/unit/test_servo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes/pca9685_fake.c
    ${CMAKE_SOURCE_DIR}/drivers/servo/servo.c
)

target_include_directories(${SERVO_UNIT_TEST_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/fakes
    ${CMAKE_SOURCE_DIR}/bsp
    ${CMAKE_SOURCE_DIR}/drivers/pca9685
    ${CMAKE_SOURCE_DIR}/drivers/servo
)

if(MSVC)
    target_compile_options(${SERVO_UNIT_TEST_TARGET} PRIVATE /W4 /WX)
else()
    target_compile_options(${SERVO_UNIT_TEST_TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME ${SERVO_UNIT_TEST_TARGET} COMMAND ${SERVO_UNIT_TEST_TARGET})
set_tests_properties(${SERVO_UNIT_TEST_TARGET} PROPERTIES LABELS "unit")
