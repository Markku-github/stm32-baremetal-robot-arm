/**
 ******************************************************************************
 * @file    runtime_motion.c
 * @brief   Early periodic motion-request handoff for v0.3.x motion safety work
 ******************************************************************************
 */

#include "runtime_motion.h"

#include "runtime_log.h"

#define RUNTIME_MOTION_INCREMENTAL_STEP_RAD 0.00087266462599716477f

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

static float runtime_motion_step_joint_toward_target(float current_angle_rad, float target_angle_rad)
{
    const float delta = target_angle_rad - current_angle_rad;

    if (delta > RUNTIME_MOTION_INCREMENTAL_STEP_RAD)
    {
        return current_angle_rad + RUNTIME_MOTION_INCREMENTAL_STEP_RAD;
    }

    if (delta < -RUNTIME_MOTION_INCREMENTAL_STEP_RAD)
    {
        return current_angle_rad - RUNTIME_MOTION_INCREMENTAL_STEP_RAD;
    }

    return target_angle_rad;
}

static void runtime_motion_build_next_pose(
    const robot_arm_pose_t *current_pose,
    const robot_arm_pose_t *target_pose,
    robot_arm_pose_t *next_pose)
{
    if ((current_pose == 0) || (target_pose == 0) || (next_pose == 0))
    {
        return;
    }

    next_pose->base_rad = runtime_motion_step_joint_toward_target(current_pose->base_rad, target_pose->base_rad);
    next_pose->shoulder_rad = runtime_motion_step_joint_toward_target(current_pose->shoulder_rad, target_pose->shoulder_rad);
    next_pose->elbow_rad = runtime_motion_step_joint_toward_target(current_pose->elbow_rad, target_pose->elbow_rad);
    next_pose->wrist_tilt_rad = runtime_motion_step_joint_toward_target(current_pose->wrist_tilt_rad, target_pose->wrist_tilt_rad);
    next_pose->wrist_rotate_rad = runtime_motion_step_joint_toward_target(current_pose->wrist_rotate_rad, target_pose->wrist_rotate_rad);
    next_pose->gripper_rad = runtime_motion_step_joint_toward_target(current_pose->gripper_rad, target_pose->gripper_rad);
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
    return ROBOT_ARM_OK;
}

static robot_arm_status_t runtime_motion_complete_active_motion(runtime_motion_t *motion)
{
    robot_arm_status_t status;
    robot_arm_pose_t next_pose;

    if ((motion == 0) || (motion->robot == 0) || !motion->motion_active)
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    runtime_motion_build_next_pose(&motion->current_pose, &motion->target_pose, &next_pose);

    status = robot_arm_set_pose_immediate(motion->robot, &next_pose);
    if (status != ROBOT_ARM_OK)
    {
        return status;
    }

    motion->current_pose = next_pose;

    if (runtime_motion_pose_matches(&motion->current_pose, &motion->target_pose))
    {
        motion->motion_active = false;
        motion->motion_home = false;
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
}

void runtime_motion_configure(
    runtime_motion_t *motion,
    robot_arm_t *robot,
    runtime_motion_recover_robot_fn recover_robot,
    void *recover_context)
{
    if (motion == 0)
    {
        return;
    }

    motion->robot = robot;
    motion->recover_robot = recover_robot;
    motion->recover_context = recover_context;

    if (robot == 0)
    {
        runtime_motion_clear(motion);
    }
}

void runtime_motion_clear(runtime_motion_t *motion)
{
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