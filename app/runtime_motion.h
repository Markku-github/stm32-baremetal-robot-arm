/**
 ******************************************************************************
 * @file    runtime_motion.h
 * @brief   Early periodic motion-request handoff for v0.3.x motion safety work
 ******************************************************************************
 */

#ifndef RUNTIME_MOTION_H
#define RUNTIME_MOTION_H

#include <stdbool.h>
#include <stdint.h>

#include "robot_arm.h"

typedef bool (*runtime_motion_recover_robot_fn)(void *context, robot_arm_t *robot);

typedef struct
{
    robot_arm_t *robot;
    runtime_motion_recover_robot_fn recover_robot;
    void *recover_context;
    robot_arm_pose_t current_pose;
    robot_arm_pose_t target_pose;
    robot_arm_pose_t pending_pose;
    bool pending_request;
    bool pending_home;
    bool motion_active;
    bool motion_home;
    uint16_t service_ticks_per_update;
    uint16_t service_ticks_until_update;
    float current_step_limit_rad;
} runtime_motion_t;

void runtime_motion_init(runtime_motion_t *motion);
void runtime_motion_configure(
    runtime_motion_t *motion,
    robot_arm_t *robot,
    runtime_motion_recover_robot_fn recover_robot,
    void *recover_context,
    uint16_t service_ticks_per_update);
void runtime_motion_clear(runtime_motion_t *motion);
bool runtime_motion_has_pending_request(const runtime_motion_t *motion);
bool runtime_motion_has_active_motion(const runtime_motion_t *motion);
bool runtime_motion_schedule_home(runtime_motion_t *motion);
bool runtime_motion_schedule_pose(runtime_motion_t *motion, const robot_arm_pose_t *pose);
bool runtime_motion_service(runtime_motion_t *motion);

#endif