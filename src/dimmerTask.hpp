#ifndef _DIMMER_TASK_
#define _DIMMER_TASK_

#include <Arduino.h>
#include <esp32-hal.h>
#include <hal/ledc_types.h>
#include <vector>

#include "lightTime_t.h"

//https://www.geeksforgeeks.org/array-of-vectors-in-c-stl/

std::vector<lightTimer_t> channel[5];

#endif