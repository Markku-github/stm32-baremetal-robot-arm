#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "board_nucleo_f767zi.h"
#include "boot_self_test.h"
#include "debug_console.h"
#include "pca9685_fake.h"
#include "robot_arm.h"

static pca9685_device_t test_device = {
    .instance = BSP_I2C_INSTANCE_I2C1,
    .address = PCA9685_I2C_ADDRESS_DEFAULT,
    .oscillator_frequency_hz = PCA9685_OSCILLATOR_FREQUENCY_HZ,
    .pwm_frequency_hz = 50U,
};

#define TEST_ASSERT_TRUE(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            printf("Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_UINT16_EQUAL(expected, actual) \
    do \
    { \
        if ((expected) != (actual)) \
        { \
            printf("Assertion failed at %s:%d: expected %u, got %u\n", __FILE__, __LINE__, (unsigned int)(expected), (unsigned int)(actual)); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_UINT32_EQUAL(expected, actual) \
    do \
    { \
        if ((expected) != (actual)) \
        { \
            printf("Assertion failed at %s:%d: expected %lu, got %lu\n", __FILE__, __LINE__, (unsigned long)(expected), (unsigned long)(actual)); \
            return false; \
        } \
    } while (0)

bsp_i2c_status_t board_nucleo_f767zi_init_pca9685_i2c(void)
{
    return BSP_I2C_OK;
}

void board_nucleo_f767zi_write_debug_string(const char *message)
{
    (void)message;
}

void debug_console_write_prompt(void)
{
}

void debug_console_write_hex_byte(uint8_t value)
{
    (void)value;
}

void debug_console_write_hex_word(uint16_t value)
{
    (void)value;
}

void debug_console_write_string_adapter(void *context, const char *text)
{
    (void)context;
    (void)text;
}

void debug_console_write_byte_adapter(void *context, uint8_t byte)
{
    (void)context;
    (void)byte;
}

void debug_console_write_prompt_adapter(void *context)
{
    (void)context;
}

static bool test_boot_self_test_run_robot_direct_pose_uses_runtime_readback_mapping(void)
{
    pca9685_fake_state_t *fake_state;

    pca9685_fake_reset();
    fake_state = pca9685_fake_state();

    TEST_ASSERT_TRUE(boot_self_test_run_robot_direct_pose(true, &test_device));
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->call_count);
    TEST_ASSERT_UINT32_EQUAL((uint32_t)ROBOT_ARM_JOINT_COUNT, fake_state->read_call_count);
    TEST_ASSERT_UINT32_EQUAL(1U, fake_state->disable_call_count);
    TEST_ASSERT_UINT16_EQUAL(1322U, fake_state->last_pulse_width_us_by_channel[ROBOT_ARM_JOINT_SHOULDER]);
    TEST_ASSERT_UINT16_EQUAL(0x010FU, fake_state->last_off_count_by_channel[ROBOT_ARM_JOINT_SHOULDER]);
    return true;
}

static bool test_boot_self_test_run_robot_direct_pose_validates_arguments(void)
{
    TEST_ASSERT_TRUE(!boot_self_test_run_robot_direct_pose(false, &test_device));
    TEST_ASSERT_TRUE(!boot_self_test_run_robot_direct_pose(true, 0));
    return true;
}

int main(void)
{
    const struct
    {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "boot_self_test_run_robot_direct_pose_uses_runtime_readback_mapping", test_boot_self_test_run_robot_direct_pose_uses_runtime_readback_mapping },
        { "boot_self_test_run_robot_direct_pose_validates_arguments", test_boot_self_test_run_robot_direct_pose_validates_arguments },
    };
    uint32_t index;

    for (index = 0U; index < (sizeof(tests) / sizeof(tests[0])); index++)
    {
        if (!tests[index].function())
        {
            printf("FAIL %s\n", tests[index].name);
            return 1;
        }

        printf("PASS %s\n", tests[index].name);
    }

    printf("All boot_self_test tests passed.\n");
    return 0;
}