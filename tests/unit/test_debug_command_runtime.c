#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bsp_i2c.h"
#include "bsp_uart.h"
#include "debug_command_handler.h"
#include "debug_command_runtime.h"
#include "debug_command_shell.h"
#include "pca9685.h"
#include "runtime_motion.h"
#include "runtime_status.h"

uint32_t SystemCoreClock = 16000000U;

static bool stub_debug_uart_overflowed = false;
static bool stub_debug_uart_has_pending_input = false;
static bool stub_runtime_motion_init_called = false;
static bool stub_runtime_motion_configure_called = false;
static bool stub_runtime_motion_clear_called = false;
static bool stub_runtime_motion_service_called = false;
static uint16_t stub_runtime_motion_configure_service_ticks_per_update = 0U;

#define TEST_ASSERT_TRUE(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            printf("Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while (0)

#define TEST_ASSERT_FALSE(condition) \
    do \
    { \
        if (condition) \
        { \
            printf("Assertion failed at %s:%d: expected false for %s\n", __FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while (0)

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

void runtime_status_capture_fault_and_reset(runtime_status_fault_kind_t fault_kind)
{
    (void)fault_kind;
}

void runtime_motion_init(runtime_motion_t *motion)
{
    (void)motion;
    stub_runtime_motion_init_called = true;
}

void runtime_motion_configure(
    runtime_motion_t *motion,
    robot_arm_t *robot,
    runtime_motion_recover_robot_fn recover_robot,
    void *recover_context,
    uint16_t service_ticks_per_update)
{
    (void)motion;
    (void)robot;
    (void)recover_robot;
    (void)recover_context;
    stub_runtime_motion_configure_called = true;
    stub_runtime_motion_configure_service_ticks_per_update = service_ticks_per_update;

    if (robot == 0)
    {
        stub_runtime_motion_clear_called = true;
    }
}

void runtime_motion_clear(runtime_motion_t *motion)
{
    (void)motion;
    stub_runtime_motion_clear_called = true;
}

bool runtime_motion_has_pending_request(const runtime_motion_t *motion)
{
    (void)motion;
    return false;
}

bool runtime_motion_schedule_home(runtime_motion_t *motion)
{
    (void)motion;
    return true;
}

bool runtime_motion_schedule_pose(runtime_motion_t *motion, const robot_arm_pose_t *pose)
{
    (void)motion;
    (void)pose;
    return true;
}

bool runtime_motion_service(runtime_motion_t *motion)
{
    (void)motion;
    stub_runtime_motion_service_called = true;
    return true;
}

uint32_t runtime_tick_now_ms(void)
{
    return 0U;
}

bool runtime_tick_deadline_reached(uint32_t start_ms, uint32_t delay_ms, uint32_t now_ms)
{
    (void)start_ms;
    (void)delay_ms;
    (void)now_ms;
    return true;
}

bsp_i2c_status_t board_nucleo_f767zi_init_pca9685_i2c(void)
{
    return BSP_I2C_OK;
}

bool board_nucleo_f767zi_debug_uart_overflowed(void)
{
    return stub_debug_uart_overflowed;
}

bool board_nucleo_f767zi_debug_uart_has_pending_input(void)
{
    return stub_debug_uart_has_pending_input;
}

void board_nucleo_f767zi_clear_debug_uart_overflow(void)
{
    stub_debug_uart_overflowed = false;
}

bsp_uart_status_t board_nucleo_f767zi_read_debug_byte(uint8_t *byte)
{
    (void)byte;
    return BSP_UART_ERR_NO_DATA;
}

pca9685_status_t pca9685_init(pca9685_device_t *device, bsp_i2c_instance_t instance, uint8_t address)
{
    (void)device;
    (void)instance;
    (void)address;
    return PCA9685_OK;
}

pca9685_status_t pca9685_set_pwm_frequency(pca9685_device_t *device, uint16_t frequency_hz)
{
    (void)device;
    (void)frequency_hz;
    return PCA9685_OK;
}

robot_arm_status_t robot_arm_init(robot_arm_t *robot, const pca9685_device_t *device)
{
    (void)robot;
    (void)device;
    return ROBOT_ARM_OK;
}

robot_arm_status_t robot_arm_assume_home(robot_arm_t *robot)
{
    (void)robot;
    return ROBOT_ARM_OK;
}

void debug_command_handler_execute(
    const char *command_line,
    const debug_command_handler_context_t *command_context,
    const debug_command_handler_io_t *io,
    void *io_context)
{
    (void)command_line;
    (void)command_context;
    (void)io;
    (void)io_context;
}

void debug_command_shell_handle_transport_overflow(
    debug_command_shell_t *shell,
    const debug_command_shell_io_t *io,
    void *context)
{
    (void)shell;
    (void)io;
    (void)context;
}

void debug_command_shell_handle_transport_read_error(
    debug_command_shell_t *shell,
    const debug_command_shell_io_t *io,
    void *context)
{
    (void)shell;
    (void)io;
    (void)context;
}

void debug_command_shell_process_byte(
    debug_command_shell_t *shell,
    uint8_t received_byte,
    const debug_command_shell_io_t *io,
    void *context)
{
    (void)shell;
    (void)received_byte;
    (void)io;
    (void)context;
}

static void reset_stubs(void)
{
    stub_debug_uart_overflowed = false;
    stub_debug_uart_has_pending_input = false;
    stub_runtime_motion_init_called = false;
    stub_runtime_motion_configure_called = false;
    stub_runtime_motion_clear_called = false;
    stub_runtime_motion_service_called = false;
    stub_runtime_motion_configure_service_ticks_per_update = 0U;
}

static bool test_has_pending_work_rejects_unready_uart(void)
{
    reset_stubs();
    stub_debug_uart_overflowed = true;
    stub_debug_uart_has_pending_input = true;

    TEST_ASSERT_FALSE(debug_command_runtime_has_pending_work(false));
    return true;
}

static bool test_has_pending_work_reports_overflow(void)
{
    reset_stubs();
    stub_debug_uart_overflowed = true;

    TEST_ASSERT_TRUE(debug_command_runtime_has_pending_work(true));
    return true;
}

static bool test_has_pending_work_reports_buffered_input(void)
{
    reset_stubs();
    stub_debug_uart_has_pending_input = true;

    TEST_ASSERT_TRUE(debug_command_runtime_has_pending_work(true));
    return true;
}

static bool test_has_pending_work_reports_idle_when_no_work_exists(void)
{
    reset_stubs();

    TEST_ASSERT_FALSE(debug_command_runtime_has_pending_work(true));
    return true;
}

static bool test_service_motion_configures_and_services_when_robot_ready(void)
{
    robot_arm_t robot;
    pca9685_device_t device = { 0 };

    reset_stubs();
    device.pwm_frequency_hz = 50U;
    debug_command_runtime_service_motion(true, &robot, &device);

    TEST_ASSERT_TRUE(stub_runtime_motion_init_called);
    TEST_ASSERT_TRUE(stub_runtime_motion_configure_called);
    TEST_ASSERT_TRUE(stub_runtime_motion_service_called);
    TEST_ASSERT_FALSE(stub_runtime_motion_clear_called);
    TEST_ASSERT_TRUE(stub_runtime_motion_configure_service_ticks_per_update == 20U);
    return true;
}

static bool test_service_motion_derives_motion_cadence_from_pwm_frequency(void)
{
    robot_arm_t robot;
    pca9685_device_t device = { 0 };

    reset_stubs();
    device.pwm_frequency_hz = 100U;
    debug_command_runtime_service_motion(true, &robot, &device);

    TEST_ASSERT_TRUE(stub_runtime_motion_configure_service_ticks_per_update == 10U);
    return true;
}

static bool test_service_motion_only_configures_when_robot_not_ready(void)
{
    robot_arm_t robot;
    pca9685_device_t device;

    reset_stubs();
    debug_command_runtime_service_motion(false, &robot, &device);

    TEST_ASSERT_TRUE(stub_runtime_motion_configure_called);
    TEST_ASSERT_FALSE(stub_runtime_motion_service_called);
    TEST_ASSERT_TRUE(stub_runtime_motion_clear_called);
    return true;
}

int main(void)
{
    const struct
    {
        const char *name;
        bool (*function)(void);
    } tests[] = {
        { "has_pending_work_rejects_unready_uart", test_has_pending_work_rejects_unready_uart },
        { "has_pending_work_reports_overflow", test_has_pending_work_reports_overflow },
        { "has_pending_work_reports_buffered_input", test_has_pending_work_reports_buffered_input },
        { "has_pending_work_reports_idle_when_no_work_exists", test_has_pending_work_reports_idle_when_no_work_exists },
        { "service_motion_configures_and_services_when_robot_ready", test_service_motion_configures_and_services_when_robot_ready },
        { "service_motion_derives_motion_cadence_from_pwm_frequency", test_service_motion_derives_motion_cadence_from_pwm_frequency },
        { "service_motion_only_configures_when_robot_not_ready", test_service_motion_only_configures_when_robot_not_ready },
    };
    unsigned int index;
    unsigned int failed_count = 0U;

    for (index = 0U; index < (unsigned int)(sizeof(tests) / sizeof(tests[0])); index++)
    {
        if (!tests[index].function())
        {
            printf("FAIL %s\n", tests[index].name);
            failed_count++;
        }
        else
        {
            printf("PASS %s\n", tests[index].name);
        }
    }

    if (failed_count > 0U)
    {
        printf("%u test(s) failed.\n", failed_count);
        return 1;
    }

    printf("All %u debug command runtime unit tests passed.\n", (unsigned int)(sizeof(tests) / sizeof(tests[0])));
    return 0;
}