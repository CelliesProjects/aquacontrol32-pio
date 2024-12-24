#ifndef _SENSOR_TASK_
#define _SENSOR_TASK_

#include <OneWire.h>
#include <DallasTemperature.h>

constexpr int ONE_WIRE_BUS = 35;

// these should become local to the sensortask once we got the code running nicely.
static OneWire oneWire(ONE_WIRE_BUS);
static DallasTemperature sensor(&oneWire);

#endif