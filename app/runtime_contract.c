/**
 ******************************************************************************
 * @file    runtime_contract.c
 * @brief   Current early-V1 runtime engineering-contract anchors
 ******************************************************************************
 */

#include "runtime_contract.h"

#include "runtime_log.h"

void runtime_contract_log_current_baseline(void)
{
    if (!runtime_log_begin_line(RUNTIME_LOG_LEVEL_INFO))
    {
        return;
    }

    runtime_log_write_raw(
        "Runtime contract: control_tick="
        RUNTIME_TICK_FREQUENCY_HZ_TEXT
        " Hz, periodic_service="
        RUNTIME_CONTRACT_MAIN_SERVICE_INTERVAL_MS_TEXT
        " ms, motion_update=direct-pose baseline, POSE_DELAY acknowledges before waiting.");
    runtime_log_end_line();
}