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
#ifndef _HTTPTASK_HPP_
#define _HTTPTASK_HPP_

#include <WiFi.h>
#include <FS.h>
#include <SD.h>
#include <optional>
#include <freertos/semphr.h>

#include <PsychicHttp.h>

#include "ScopedMutex.h"
#include "ScopedFile.h"
#include "lightTimer.h"
#include "websocketMessage.h"

extern const char *WEBIF_USER;
extern const char *WEBIF_PASSWORD;

extern std::vector<lightTimer_t> channel[NUMBER_OF_CHANNELS];
extern float fullMoonLevel[NUMBER_OF_CHANNELS];
extern SemaphoreHandle_t channelMutex;
extern SemaphoreHandle_t spiMutex;

extern bool saveDefaultTimers(String &result);
extern bool loadDefaultTimers(String &result);
extern void messageOnLcd(const char *str);
extern bool startSensor();

QueueHandle_t websocketQueue = xQueueCreate(6, sizeof(websocketMessage));

constexpr char *MOON_SETTINGS_FILE = "/default.mnl";
const char *DEFAULT_TIMERFILE = "/default.aqu";

#endif
