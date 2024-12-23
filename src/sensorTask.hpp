#ifndef _SENSOR_TASK_
#define _SENSOR_TASK_

#include <OneWire.h>
#include <DallasTemperature.h>

constexpr int ONE_WIRE_BUS = 35;

static OneWire oneWire(ONE_WIRE_BUS);
static DallasTemperature sensors(&oneWire);

#endif