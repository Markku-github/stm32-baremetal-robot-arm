/**
 ******************************************************************************
 * @file    runtime_motion.c
 * @brief   Early periodic motion-request handoff for v0.3.x motion safety work
 ******************************************************************************
 */

#include "runtime_motion.h"

#include "runtime_log.h"

static robot_arm_status_t runtime_motion_apply_pending_request(runtime_motion_t *motion)
{
    if ((motion == 0) || (motion->robot == 0) || !motion->pending_request)
    {
        return ROBOT_ARM_ERR_INVALID_ARGUMENT;
    }

    if (motion->pending_home)
    {
        return robot_arm_home(motion->robot);
    }

    return robot_arm_set_pose_immediate(motion->robot, &motion->pending_pose);
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
    motion->pending_request = false;
    motion->pending_home = false;
    motion->pending_pose.base_rad = 0.0f;
    motion->pending_pose.shoulder_rad = 0.0f;
    motion->pending_pose.elbow_rad = 0.0f;
    motion->pending_pose.wrist_tilt_rad = 0.0f;
    motion->pending_pose.wrist_rotate_rad = 0.0f;
    motion->pending_pose.gripper_rad = 0.0f;
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

    motion->pending_request = false;
    motion->pending_home = false;
}

bool runtime_motion_has_pending_request(const runtime_motion_t *motion)
{
    return (motion != 0) && motion->pending_request;
}

bool runtime_motion_schedule_home(runtime_motion_t *motion)
{
    if ((motion == 0) || (motion->robot == 0) || motion->pending_request)
    {
        return false;
    }

    motion->pending_request = true;
    motion->pending_home = true;
    return true;
}

bool runtime_motion_schedule_pose(runtime_motion_t *motion, const robot_arm_pose_t *pose)
{
    if ((motion == 0) || (motion->robot == 0) || (pose == 0) || motion->pending_request)
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
    const bool pending_home = (motion != 0) && motion->pending_home;

    if ((motion == 0) || !motion->pending_request)
    {
        return true;
    }

    status = runtime_motion_apply_pending_request(motion);
    if ((status != ROBOT_ARM_OK) && (motion->recover_robot != 0)
        && motion->recover_robot(motion->recover_context, motion->robot))
    {
        status = runtime_motion_apply_pending_request(motion);
    }

    motion->pending_request = false;
    motion->pending_home = false;

    if (status == ROBOT_ARM_OK)
    {
        return true;
    }

    runtime_log_write_line(
        RUNTIME_LOG_LEVEL_ERROR,
        pending_home ? "Scheduled HOME command failed." : "Scheduled POSE command failed.");
    return false;
}