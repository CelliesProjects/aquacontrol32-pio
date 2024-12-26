#ifndef _SENSOR_TASK_
#define _SENSOR_TASK_

#include <OneWire.h>
#include <DallasTemperature.h>

#include "lcdMessage.h"

static constexpr int ONE_WIRE_PIN = 26;
static constexpr float TEMPERATURE_THRESHOLD = (0.05f);
static constexpr int MAX_ERROR_COUNT = 10;

extern QueueHandle_t lcdQueue;

#endif
