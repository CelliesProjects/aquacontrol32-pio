/*
MIT License

Copyright (c) 2025 Cellie https://github.com/CelliesProjects/aquacontrol32-pio/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef _DIMMERTASK_HPP_
#define _DIMMERTASK_HPP_

#include <esp32-hal.h>
#include <hal/ledc_types.h>
#include <vector>
#include <MoonPhase.hpp>

#include "ScopedMutex.h"
#include "lightTimer.h"
#include "lcdMessage.h"
#include "websocketMessage.h"

extern float mapf(const float x, const float in_min, const float in_max, const float out_min, const float out_max);

extern QueueHandle_t lcdQueue;
extern QueueHandle_t websocketQueue;

std::vector<lightTimer_t> channel[NUMBER_OF_CHANNELS];
SemaphoreHandle_t channelMutex;

float currentPercentage[NUMBER_OF_CHANNELS] = {0, 0, 0, 0, 0};
float fullMoonLevel[NUMBER_OF_CHANNELS] = {0, 0, 0, 0, 0};

#endif
