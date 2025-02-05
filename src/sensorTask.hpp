#ifndef _SENSOR_TASK_HPP_
#define _SENSOR_TASK_HPP_

#include <OneWire.h>
#include <DallasTemperature.h>

#include "lcdMessage.h"
#include "websocketMessage.h"

static constexpr float TEMPERATURE_THRESHOLD = (0.05f);
static constexpr int MAX_ERROR_COUNT = 10;

extern QueueHandle_t lcdQueue;
extern QueueHandle_t websocketQueue;

#endif
