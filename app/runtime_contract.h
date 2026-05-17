/**
 ******************************************************************************
 * @file    runtime_contract.h
 * @brief   Current early-V1 runtime engineering-contract anchors
 ******************************************************************************
 */

#ifndef RUNTIME_CONTRACT_H
#define RUNTIME_CONTRACT_H

#include "runtime_tick.h"

#define RUNTIME_CONTRACT_MAIN_SERVICE_INTERVAL_MS 1U
#define RUNTIME_CONTRACT_MAIN_SERVICE_INTERVAL_MS_TEXT "1"

void runtime_contract_log_current_baseline(void);

#endif