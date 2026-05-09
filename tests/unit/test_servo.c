#include <stdbool.h>
#include <stdio.h>

#include "pca9685_fake.h"
#include "servo.h"

#define SERVO_TEST_PI_F 3.14159265358979323846f
#define SERVO_TEST_DEG_TO_RAD(angle_deg) ((angle_deg) * (SERVO_TEST_PI_F / 180.0f))
#define SERVO_TEST_FLOAT_TOLERANCE 0.0001f

static bool float_is_close(float actual, float expected, float tolerance)
{
    float delta = actual - expected;

    if (delta < 0.0f)
    {
        delta = -delta;
    }

    return delta <= tolerance;
}

#define TEST_ASSERT_TRUE(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            printf("Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_INT_EQUAL(expected, actual) \
    do \
    { \
        if ((expected) != (actual)) \
        { \
            printf("Assertion failed at %s:%d: expected %d, got %d\n", __FILE__, __LINE__, (int)(expected), (int)(actual)); \
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

#define TEST_ASSERT_UINT16_EQUAL(expected, actual) \
    do \
    { \
        if ((expected) != (actual)) \
        { \
            printf("Assertion failed at %s:%d: expected %u, got %u\n", __FILE__, __LINE__, (unsigned int)(expected), (unsigned int)(actual)); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_FLOAT_CLOSE(expected, actual) \
    do \
    { \
        if (!float_is_close((actual), (expected), SERVO_TEST_FLOAT_TOLERANCE)) \
        { \
            printf("Assertion failed at %s:%d: expected %.6f, got %.6f\n", __FILE__, __LINE__, (double)(expected), (double)(actual)); \
            return false; \
        } \
    } while (0)

static const pca9685_device_t test_device = {
    .instance = BSP_I2C_INSTANCE_I2C1,
    .address = PCA9685_I2C_ADDRESS_DEFAULT,
    .oscillator_frequency_hz = PCA9685_OSCILLATOR_FREQUENCY_HZ,
    .pwm_frequency_hz = 50U,
};

static const servo_config_t test_config = {
    .name = "unit_servo",
    .channel = 3U,
    .minimum_angle_rad = -SERVO_TEST_DEG_TO_RAD(45.0f),
    .maximum_angle_rad = SERVO_TEST_DEG_TO_RAD(45.0f),
    .offset_rad = 0.0f,
    .minimum_pulse_width_us = 1000U,
    .maximum_pulse_width_us = 2000U,
};

static bool test_servo_init_centers_runtime_state(void)
{
    servo_t servo;

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_init(&servo, &test_config, &test_device));
    TEST_ASSERT_FLOAT_CLOSE(0.0f, servo.current_angle_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, servo.target_angle_rad);
    TEST_ASSERT_UINT16_EQUAL(1000U, servo.minimum_pulse_width_us);
    TEST_ASSERT_UINT16_EQUAL(2000U, servo.maximum_pulse_width_us);
    return true;
}

static bool test_servo_clamp_angle_respects_limits(void)
{
    servo_t servo;
    float clamped_angle_rad;

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_init(&servo, &test_config, &test_device));
    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_clamp_angle_rad(&servo, SERVO_TEST_DEG_TO_RAD(-90.0f), &clamped_angle_rad));
    TEST_ASSERT_FLOAT_CLOSE(-SERVO_TEST_DEG_TO_RAD(45.0f), clamped_angle_rad);

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_clamp_angle_rad(&servo, SERVO_TEST_DEG_TO_RAD(90.0f), &clamped_angle_rad));
    TEST_ASSERT_FLOAT_CLOSE(SERVO_TEST_DEG_TO_RAD(45.0f), clamped_angle_rad);
    return true;
}

static bool test_servo_angle_to_pulse_maps_midpoint_and_limits(void)
{
    servo_t servo;
    uint16_t pulse_width_us;

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_init(&servo, &test_config, &test_device));

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_angle_rad_to_pulse_us(&servo, 0.0f, &pulse_width_us));
    TEST_ASSERT_UINT16_EQUAL(1500U, pulse_width_us);

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_angle_rad_to_pulse_us(&servo, SERVO_TEST_DEG_TO_RAD(-45.0f), &pulse_width_us));
    TEST_ASSERT_UINT16_EQUAL(1000U, pulse_width_us);

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_angle_rad_to_pulse_us(&servo, SERVO_TEST_DEG_TO_RAD(45.0f), &pulse_width_us));
    TEST_ASSERT_UINT16_EQUAL(2000U, pulse_width_us);
    return true;
}

static bool test_servo_angle_to_pulse_supports_reversed_pulse_endpoints(void)
{
    servo_t servo;
    servo_config_t reversed_config = test_config;
    uint16_t pulse_width_us;

    reversed_config.minimum_pulse_width_us = 2000U;
    reversed_config.maximum_pulse_width_us = 1000U;

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_init(&servo, &reversed_config, &test_device));

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_angle_rad_to_pulse_us(&servo, SERVO_TEST_DEG_TO_RAD(-45.0f), &pulse_width_us));
    TEST_ASSERT_UINT16_EQUAL(2000U, pulse_width_us);

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_angle_rad_to_pulse_us(&servo, 0.0f, &pulse_width_us));
    TEST_ASSERT_UINT16_EQUAL(1500U, pulse_width_us);

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_angle_rad_to_pulse_us(&servo, SERVO_TEST_DEG_TO_RAD(45.0f), &pulse_width_us));
    TEST_ASSERT_UINT16_EQUAL(1000U, pulse_width_us);
    return true;
}

static bool test_servo_set_angle_updates_runtime_and_uses_pca9685(void)
{
    servo_t servo;
    pca9685_fake_state_t *fake_state;

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_init(&servo, &test_config, &test_device));
    pca9685_fake_reset();
    fake_state = pca9685_fake_state();

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_set_angle_immediate_rad(&servo, SERVO_TEST_DEG_TO_RAD(22.5f)));
    TEST_ASSERT_UINT32_EQUAL(1U, fake_state->call_count);
    TEST_ASSERT_TRUE(fake_state->last_device == &test_device);
    TEST_ASSERT_UINT16_EQUAL(3U, fake_state->last_channel);
    TEST_ASSERT_UINT16_EQUAL(1750U, fake_state->last_pulse_width_us);
    TEST_ASSERT_FLOAT_CLOSE(SERVO_TEST_DEG_TO_RAD(22.5f), servo.current_angle_rad);
    TEST_ASSERT_FLOAT_CLOSE(SERVO_TEST_DEG_TO_RAD(22.5f), servo.target_angle_rad);
    return true;
}

static bool test_servo_init_rejects_invalid_configuration(void)
{
    servo_t servo;
    servo_config_t invalid_config = test_config;

    TEST_ASSERT_INT_EQUAL(SERVO_ERR_INVALID_ARGUMENT, servo_init(0, &test_config, &test_device));
    TEST_ASSERT_INT_EQUAL(SERVO_ERR_INVALID_ARGUMENT, servo_init(&servo, 0, &test_device));
    TEST_ASSERT_INT_EQUAL(SERVO_ERR_INVALID_ARGUMENT, servo_init(&servo, &test_config, 0));

    invalid_config.channel = PCA9685_CHANNEL_COUNT;
    TEST_ASSERT_INT_EQUAL(SERVO_ERR_INVALID_ARGUMENT, servo_init(&servo, &invalid_config, &test_device));

    invalid_config = test_config;
    invalid_config.minimum_angle_rad = invalid_config.maximum_angle_rad;
    TEST_ASSERT_INT_EQUAL(SERVO_ERR_INVALID_ARGUMENT, servo_init(&servo, &invalid_config, &test_device));

    invalid_config = test_config;
    invalid_config.minimum_pulse_width_us = invalid_config.maximum_pulse_width_us;
    TEST_ASSERT_INT_EQUAL(SERVO_ERR_INVALID_ARGUMENT, servo_init(&servo, &invalid_config, &test_device));
    return true;
}

static bool test_servo_angle_to_pulse_applies_offset_and_post_offset_clamp(void)
{
    servo_t servo;
    servo_config_t offset_config = test_config;
    uint16_t pulse_width_us;

    offset_config.offset_rad = SERVO_TEST_DEG_TO_RAD(10.0f);
    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_init(&servo, &offset_config, &test_device));

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_angle_rad_to_pulse_us(&servo, 0.0f, &pulse_width_us));
    TEST_ASSERT_UINT16_EQUAL(1611U, pulse_width_us);

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_angle_rad_to_pulse_us(&servo, SERVO_TEST_DEG_TO_RAD(40.0f), &pulse_width_us));
    TEST_ASSERT_UINT16_EQUAL(2000U, pulse_width_us);
    return true;
}

static bool test_servo_set_angle_clamps_requested_angle_before_updating_state(void)
{
    servo_t servo;
    pca9685_fake_state_t *fake_state;

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_init(&servo, &test_config, &test_device));
    pca9685_fake_reset();
    fake_state = pca9685_fake_state();

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_set_angle_immediate_deg(&servo, 90.0f));
    TEST_ASSERT_UINT32_EQUAL(1U, fake_state->call_count);
    TEST_ASSERT_UINT16_EQUAL(2000U, fake_state->last_pulse_width_us);
    TEST_ASSERT_FLOAT_CLOSE(SERVO_TEST_DEG_TO_RAD(45.0f), servo.current_angle_rad);
    TEST_ASSERT_FLOAT_CLOSE(SERVO_TEST_DEG_TO_RAD(45.0f), servo.target_angle_rad);
    return true;
}

static bool test_servo_set_angle_reports_pca9685_failure_without_mutating_runtime_state(void)
{
    servo_t servo;
    pca9685_fake_state_t *fake_state;

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_init(&servo, &test_config, &test_device));
    pca9685_fake_reset();
    fake_state = pca9685_fake_state();
    fake_state->next_status = PCA9685_ERR_I2C;

    TEST_ASSERT_INT_EQUAL(SERVO_ERR_PCA9685, servo_set_angle_immediate_rad(&servo, SERVO_TEST_DEG_TO_RAD(15.0f)));
    TEST_ASSERT_UINT32_EQUAL(1U, fake_state->call_count);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, servo.current_angle_rad);
    TEST_ASSERT_FLOAT_CLOSE(0.0f, servo.target_angle_rad);
    return true;
}

static bool test_servo_functions_reject_invalid_runtime_arguments(void)
{
    servo_t servo;
    float clamped_angle_rad;
    uint16_t pulse_width_us;

    TEST_ASSERT_INT_EQUAL(SERVO_OK, servo_init(&servo, &test_config, &test_device));

    TEST_ASSERT_INT_EQUAL(SERVO_ERR_INVALID_ARGUMENT, servo_clamp_angle_rad(0, 0.0f, &clamped_angle_rad));
    TEST_ASSERT_INT_EQUAL(SERVO_ERR_INVALID_ARGUMENT, servo_clamp_angle_rad(&servo, 0.0f, 0));
    TEST_ASSERT_INT_EQUAL(SERVO_ERR_INVALID_ARGUMENT, servo_angle_rad_to_pulse_us(0, 0.0f, &pulse_width_us));
    TEST_ASSERT_INT_EQUAL(SERVO_ERR_INVALID_ARGUMENT, servo_angle_rad_to_pulse_us(&servo, 0.0f, 0));
    TEST_ASSERT_INT_EQUAL(SERVO_ERR_INVALID_ARGUMENT, servo_set_angle_immediate_rad(0, 0.0f));
    TEST_ASSERT_INT_EQUAL(SERVO_ERR_INVALID_ARGUMENT, servo_set_angle_immediate_deg(0, 0.0f));
    return true;
}

int main(void)
{
    const struct
    {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "servo_init_centers_runtime_state", test_servo_init_centers_runtime_state },
        { "servo_init_rejects_invalid_configuration", test_servo_init_rejects_invalid_configuration },
        { "servo_clamp_angle_respects_limits", test_servo_clamp_angle_respects_limits },
        { "servo_angle_to_pulse_maps_midpoint_and_limits", test_servo_angle_to_pulse_maps_midpoint_and_limits },
        { "servo_angle_to_pulse_supports_reversed_pulse_endpoints", test_servo_angle_to_pulse_supports_reversed_pulse_endpoints },
        { "servo_angle_to_pulse_applies_offset_and_post_offset_clamp", test_servo_angle_to_pulse_applies_offset_and_post_offset_clamp },
        { "servo_set_angle_updates_runtime_and_uses_pca9685", test_servo_set_angle_updates_runtime_and_uses_pca9685 },
        { "servo_set_angle_clamps_requested_angle_before_updating_state", test_servo_set_angle_clamps_requested_angle_before_updating_state },
        { "servo_set_angle_reports_pca9685_failure_without_mutating_runtime_state", test_servo_set_angle_reports_pca9685_failure_without_mutating_runtime_state },
        { "servo_functions_reject_invalid_runtime_arguments", test_servo_functions_reject_invalid_runtime_arguments },
    };
    unsigned int test_index;
    unsigned int failed_count = 0U;

    for (test_index = 0U; test_index < (unsigned int)(sizeof(tests) / sizeof(tests[0])); test_index++)
    {
        if (!tests[test_index].function())
        {
            printf("FAIL %s\n", tests[test_index].name);
            failed_count++;
        }
        else
        {
            printf("PASS %s\n", tests[test_index].name);
        }
    }

    if (failed_count > 0U)
    {
        printf("%u test(s) failed.\n", failed_count);
        return 1;
    }

    printf("All %u servo unit tests passed.\n", (unsigned int)(sizeof(tests) / sizeof(tests[0])));
    return 0;
}