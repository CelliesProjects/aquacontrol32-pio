#ifndef _DIMMERTASK_HPP_
#define _DIMMERTASK_HPP_

#include <Arduino.h>
#include <esp32-hal.h>
#include <hal/ledc_types.h>
#include <vector>
#include <mutex>
#include <moonPhase.h>

#include "lightTimer.h"
#include "lcdMessage.h"
#include "websocketMessage.h"

extern float mapf(const float x, const float in_min, const float in_max, const float out_min, const float out_max);

extern QueueHandle_t lcdQueue;
extern QueueHandle_t websocketQueue;

std::vector<lightTimer_t> channel[NUMBER_OF_CHANNELS];
std::mutex channelMutex;

float currentPercentage[NUMBER_OF_CHANNELS];

#endif