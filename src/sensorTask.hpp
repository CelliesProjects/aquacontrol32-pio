#ifndef _SENSOR_TASK_
#define _SENSOR_TASK_

#include <OneWire.h>
#include <DallasTemperature.h>

#include "lcdMessage_t.h"

#define TEMPERATURE_THRESHOLD (0.05f)

extern QueueHandle_t lcdQueue;

static constexpr int ONE_WIRE_PIN = 26;

#endif
