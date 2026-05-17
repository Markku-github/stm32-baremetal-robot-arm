/**
 ******************************************************************************
 * @file    runtime_motion.c
 * @brief   Early periodic motion-request handoff for v0.3.x motion safety work
 ******************************************************************************
 */

#include "runtime_motion.h"

#include "runtime_log.h"

#define RUNTIME_MOTION_TARGET_SPEED_RAD_PER_SECOND 0.69813170079773179f
#define RUNTIME_MOTION_TARGET_ACCELERATION_RAD_PER_SECOND2 4.36332312998582398f
#define RUNTIME_MOTION_MIN_PROFILED_STEP_RAD 0.00872664625997164788f
#define RUNTIME_MOTION_SERVICE_TICK_SECONDS 0.001f

static uint16_t runtime_motion_normalize_service_ticks_per_update(uint16_t service_ticks_per_update)
{
    return (service_ticks_per_update == 0U) ? 1U : service_ticks_per_update;
}

static float runtime_motion_update_period_seconds(const runtime_motion_t *motion)
{
    const uint16_t service_ticks_per_update =
        (motion == 0) ? 1U : runtime_motion_normalize_service_ticks_per_update(motion->service_ticks_per_update);

    return (float)service_ticks_per_update * RUNTIME_MOTION_SERVICE_TICK_SECONDS;
}

static float runtime_motion_max_step_limit_rad(const runtime_motion_t *motion)
{
    return RUNTIME_MOTION_TARGET_SPEED_RAD_PER_SECOND * runtime_motion_update_period_seconds(motion);
}

static float runtime_motion_step_increment_rad(const runtime_motion_t *motion)
{
    const float update_period_seconds = runtime_motion_update_period_seconds(motion);

    return RUNTIME_MOTION_TARGET_ACCELERATION_RAD_PER_SECOND2 * update_period_seconds * update_period_seconds;
}

static float runtime_motion_min_step_limit_rad(const runtime_motion_t *motion)
{
    const float max_step_limit_rad = runtime_motion_max_step_limit_rad(motion);

    return (max_step_limit_rad < RUNTIME_MOTION_MIN_PROFILED_STEP_RAD)
        ? max_step_limit_rad
        : RUNTIME_MOTION_MIN_PROFILED_STEP_RAD;
}

static float runtime_motion_clamp_step_limit_rad(float step_limit_rad, float min_step_limit_rad, float max_step_limit_rad)
{
    if (step_limit_rad < min_step_limit_rad)
    {
        return min_step_limit_rad;
    }

    if (step_limit_rad > max_step_limit_rad)
    {
        return max_step_limit_rad;
    }

    return step_limit_rad;
}

static void runtime_motion_zero_pose(robot_arm_pose_t *pose)
{
    if (pose == 0)
    {
        return;
    }

    pose->base_rad = 0.0f;
    pose->shoulder_rad = 0.0f;
    pose->elbow_rad = 0.0f;
    pose->wrist_tilt_rad = 0.0f;
    pose->wrist_rotate_rad = 0.0f;
    pose->gripper_rad = 0.0f;
}

static float runtime_motion_absolute_difference(float left, float right)
{
    const float delta = left - right;

    return (delta < 0.0f) ? -delta : delta;
}

static float runtime_motion_max_remaining_distance_rad(
    const robot_arm_pose_t *current_pose,
    const robot_arm_pose_t *target_pose)
{
    float max_difference_rad;
    float difference_rad;

    if ((current_pose == 0) || (target_pose == 0))
    {
        return 0.0f;
    }

    max_difference_rad = runtime_motion_absolute_difference(current_pose->base_rad, target_pose->base_rad);

    difference_rad = runtime_motion_absolute_difference(current_pose->shoulder_rad, target_pose->shoulder_rad);
    if (difference_rad > max_difference_rad)
    {
        max_difference_rad = difference_rad;
    }

    difference_rad = runtime_motion_absolute_difference(current_pose->elbow_rad, target_pose->elbow_rad);
    if (difference_rad > max_difference_rad)
    {
        max_difference_rad = difference_rad;
    }

    difference_rad = runtime_motion_absolute_difference(current_pose->wrist_tilt_rad, target_pose->wrist_tilt_rad);
    if (difference_rad > max_difference_rad)
    {
        max_difference_rad = difference_rad;
    }

    difference_rad = runtime_motion_absolute_difference(current_pose->wrist_rotate_rad, target_pose->wrist_rotate_rad);
    if (difference_rad > max_difference_rad)
    {
        max_difference_rad = difference_rad;
    }

    difference_rad = runtime_motion_absolute_difference(current_pose->gripper_rad, target_pose->gripper_rad);
    if (difference_rad > max_difference_rad)
    {
        max_difference_rad = difference_rad;
    }

    return max_difference_rad;
}

static float runtime_motion_profiled_step_limit_rad(runtime_motion_t *motion)
{
    const float max_step_limit_rad = runtime_motion_max_step_limit_rad(motion);
    const float min_step_limit_rad = runtime_motion_min_step_limit_rad(motion);
    const float step_increment_rad = runtime_motion_step_increment_rad(motion);
    const float update_period_seconds = runtime_motion_update_period_seconds(motion);
    const float remaining_distance_rad = runtime_motion_max_remaining_distance_rad(&motion->current_pose, &motion->target_pose);
    float step_limit_rad;
    float current_speed_rad_per_second;
    float stopping_distance_rad;

    if ((motion == 0) || !motion->motion_active)
    {
        return 0.0f;
    }

    if (motion->current_step_limit_rad <= 0.0f)
    {
        return min_step_limit_rad;
    }

    step_limit_rad = motion->current_step_limit_rad;
    current_speed_rad_per_second = step_limit_rad / update_period_seconds;
    stopping_distance_rad =
        (current_speed_rad_per_second * current_speed_rad_per_second)
        / (2.0f * RUNTIME_MOTION_TARGET_ACCELERATION_RAD_PER_SECOND2);

    if (remaining_distance_rad <= stopping_distance_rad)
    {
        step_limit_rad -= step_increment_rad;
    }
    else
    {
        step_limit_rad += step_increment_rad;
    }

    return runtime_motion_clamp_step_limit_rad(step_limit_rad, min_step_limit_rad, max_step_limit_rad);
}

static float runtime_motion_step_joint_toward_target(
    float current_angle_rad,
    float target_angle_rad,
    float step_limit_rad)
{
    const float delta = target_angle_rad - current_angle_rad;

    if (delta > step_limit_rad)
    {
        return current_angle_rad + step_limit_rad;
    }

    if (delta < -step_limit_rad)
    {
        return current_angle_rad - step_limit_rad;
    }

    return target_angle_rad;
}

static void runtime_motion_build_next_pose(
    const robot_arm_pose_t *current_pose,
    const robot_arm_pose_t *target_pose,
    float step_limit_rad,
    robot_arm_pose_t *next_pose)
{
    if ((current_pose == 0) || (target_pose == 0) || (next_pose == 0))
    {
        return;
    }

    next_pose->base_rad = runtime_motion_step_joint_toward_target(current_pose->base_rad, target_pose->base_rad, step_limit_rad);
    next_pose->shoulder_rad = runtime_motion_step_joint_toward_target(current_pose->shoulder_rad, target_pose->shoulder_rad, step_limit_rad);
    next_pose->elbow_rad = runtime_motion_step_joint_toward_target(current_pose->elbow_rad, target_pose->elbow_rad, step_limit_rad);
    next_pose->wrist_tilt_rad = runtime_motion_step_joint_toward_target(current_pose->wrist_tilt_rad, target_pose->wrist_tilt_rad, step_limit_rad);
    next_pose->wrist_rotate_rad = runtime_motion_step_joint_toward_target(current_pose->wrist_rotate_rad, target_pose->wrist_rotate_rad, step_limit_rad);
    next_pose->gripper_rad = runtime_motion_step_joint_toward_target(current_pose->gripper_rad, target_pose->gripper_rad, step_limit_rad);
}

static bool runtime_motion_pose_matches(const robot_arm_pose_t *left_pose, const robot_arm_pose_t *right_pose)
{
    return (left_pose != 0)
        && (right_pose != 0)
        && (runtime_motion_absolute_difference(left_pose->base_rad, right_pose->base_rad) <= 0.000001f)
        && (runtime_motion_absolute_difference(left_pose->shoulder_rad, right_pose->shoulder_rad) <= 0.000001f)
        && (runtime_motion_absolute_difference(left_pose->elbow_rad, right_pose->elbow_rad) <= 0.000001f)
        && (runtime_motion_absolute_difference(left_pose->wrist_tilt_rad, right_pose->wrist_tilt_rad) <= 0.000001f)
        && (runtime_motion_absolute_difference(left_pose->wrist_rotate_rad, right_pose->wrist_rotate_rad) <= 0.000001f)
        && (runtime_motion_absolute_difference(left_pose->gripper_rad, right_pose->gripper_rad) <= 0.000001f);
}

static robot_arm_status_t runtime_motion_begin_pending_request(runtime_motion_t *motion)
{
    if ((motion == 0) || (motion->robot == 0) || !motion->pending_request || motion->motion_active)
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    if (robot_arm_get_current_pose(motion->robot, &motion->current_pose) != ROBOT_ARM_OK)
    {
        return ROBOT_ARM_ERR_SERVO;
    }

    if (motion->pending_home)
    {
        if (robot_arm_get_home_pose(motion->robot, &motion->target_pose) != ROBOT_ARM_OK)
        {
            return ROBOT_ARM_ERR_SERVO;
        }
    }
    else
    {
        motion->target_pose = motion->pending_pose;
    }

    motion->motion_active = true;
    motion->motion_home = motion->pending_home;
    motion->pending_request = false;
    motion->pending_home = false;
    motion->service_ticks_until_update = runtime_motion_normalize_service_ticks_per_update(motion->service_ticks_per_update);
    motion->current_step_limit_rad = 0.0f;
    return ROBOT_ARM_OK;
}

static bool runtime_motion_active_step_due(runtime_motion_t *motion)
{
    if ((motion == 0) || !motion->motion_active)
    {
        return false;
    }

    if (motion->service_ticks_until_update > 1U)
    {
        motion->service_ticks_until_update--;
        return false;
    }

    motion->service_ticks_until_update = runtime_motion_normalize_service_ticks_per_update(motion->service_ticks_per_update);
    return true;
}

static robot_arm_status_t runtime_motion_complete_active_motion(runtime_motion_t *motion)
{
    robot_arm_status_t status;
    robot_arm_pose_t next_pose;
    const float step_limit_rad = runtime_motion_profiled_step_limit_rad(motion);

    if ((motion == 0) || (motion->robot == 0) || !motion->motion_active)
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    runtime_motion_build_next_pose(&motion->current_pose, &motion->target_pose, step_limit_rad, &next_pose);

    status = robot_arm_set_pose_immediate(motion->robot, &next_pose);
    if (status != ROBOT_ARM_OK)
    {
        return status;
    }

    motion->current_pose = next_pose;
    motion->current_step_limit_rad = step_limit_rad;

    if (runtime_motion_pose_matches(&motion->current_pose, &motion->target_pose))
    {
        motion->motion_active = false;
        motion->motion_home = false;
        motion->service_ticks_until_update = runtime_motion_normalize_service_ticks_per_update(motion->service_ticks_per_update);
        motion->current_step_limit_rad = 0.0f;
    }

    return ROBOT_ARM_OK;
}

void runtime_motion_init(runtime_motion_t *motion)
{
    if (motion == 0)
    {
        return;
    }

    motion->robot = 0;
    motion->recover_robot = 0;
    motion->recover_context = 0;
    runtime_motion_zero_pose(&motion->current_pose);
    runtime_motion_zero_pose(&motion->target_pose);
    runtime_motion_zero_pose(&motion->pending_pose);
    motion->pending_request = false;
    motion->pending_home = false;
    motion->motion_active = false;
    motion->motion_home = false;
    motion->service_ticks_per_update = 1U;
    motion->service_ticks_until_update = 1U;
    motion->current_step_limit_rad = 0.0f;
}

void runtime_motion_configure(
    runtime_motion_t *motion,
    robot_arm_t *robot,
    runtime_motion_recover_robot_fn recover_robot,
    void *recover_context,
    uint16_t service_ticks_per_update)
{
    const uint16_t normalized_service_ticks_per_update =
        runtime_motion_normalize_service_ticks_per_update(service_ticks_per_update);

    if (motion == 0)
    {
        return;
    }

    motion->robot = robot;
    motion->recover_robot = recover_robot;
    motion->recover_context = recover_context;
    motion->service_ticks_per_update = normalized_service_ticks_per_update;

    if ((motion->service_ticks_until_update == 0U)
        || (motion->service_ticks_until_update > normalized_service_ticks_per_update))
    {
        motion->service_ticks_until_update = normalized_service_ticks_per_update;
    }

    if (robot == 0)
    {
        runtime_motion_clear(motion);
    }
}

void runtime_motion_clear(runtime_motion_t *motion)
{
    const uint16_t normalized_service_ticks_per_update =
        (motion == 0) ? 1U : runtime_motion_normalize_service_ticks_per_update(motion->service_ticks_per_update);

    if (motion == 0)
    {
        return;
    }

    runtime_motion_zero_pose(&motion->current_pose);
    runtime_motion_zero_pose(&motion->target_pose);
    runtime_motion_zero_pose(&motion->pending_pose);
    motion->pending_request = false;
    motion->pending_home = false;
    motion->motion_active = false;
    motion->motion_home = false;
    motion->service_ticks_per_update = normalized_service_ticks_per_update;
    motion->service_ticks_until_update = normalized_service_ticks_per_update;
    motion->current_step_limit_rad = 0.0f;
}

bool runtime_motion_has_pending_request(const runtime_motion_t *motion)
{
    return (motion != 0) && motion->pending_request;
}

bool runtime_motion_has_active_motion(const runtime_motion_t *motion)
{
    return (motion != 0) && motion->motion_active;
}

bool runtime_motion_schedule_home(runtime_motion_t *motion)
{
    if ((motion == 0) || (motion->robot == 0) || motion->pending_request || motion->motion_active)
    {
        return false;
    }

    motion->pending_request = true;
    motion->pending_home = true;
    return true;
}

bool runtime_motion_schedule_pose(runtime_motion_t *motion, const robot_arm_pose_t *pose)
{
    if ((motion == 0) || (motion->robot == 0) || (pose == 0) || motion->pending_request || motion->motion_active)
    {
        return false;
    }

    motion->pending_pose = *pose;
    motion->pending_request = true;
    motion->pending_home = false;
    return true;
}

bool runtime_motion_service(runtime_motion_t *motion)
{
    robot_arm_status_t status;
    bool request_home;

    if (motion == 0)
    {
        return true;
    }

    if (!motion->pending_request && !motion->motion_active)
    {
        return true;
    }

    request_home = motion->pending_request ? motion->pending_home : motion->motion_home;

    if (motion->pending_request)
    {
        status = runtime_motion_begin_pending_request(motion);
        if (status == ROBOT_ARM_OK)
        {
            return true;
        }
    }
    else
    {
        if (!runtime_motion_active_step_due(motion))
        {
            return true;
        }

        status = runtime_motion_complete_active_motion(motion);
    }

    if ((status != ROBOT_ARM_OK) && (motion->recover_robot != 0)
        && motion->recover_robot(motion->recover_context, motion->robot))
    {
        status = motion->motion_active
            ? runtime_motion_complete_active_motion(motion)
            : runtime_motion_begin_pending_request(motion);
    }

    if (status == ROBOT_ARM_OK)
    {
        return true;
    }

    runtime_log_write_line(
        RUNTIME_LOG_LEVEL_ERROR,
        request_home ? "Scheduled HOME command failed." : "Scheduled POSE command failed.");
    return false;
}