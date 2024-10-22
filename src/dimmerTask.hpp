#ifndef _DIMMER_TASK_
#define _DIMMER_TASK_

#include <Arduino.h>
#include <esp32-hal.h>
#include <hal/ledc_types.h>
#include <vector>
#include <moonPhase.h>

#include "lightTime_t.h"
#include "lcdMessage_t.h"

extern QueueHandle_t lcdQueue;

std::vector<lightTimer_t> channel[NUMBER_OF_CHANNELS];

float currentPercentage[NUMBER_OF_CHANNELS];

#endif