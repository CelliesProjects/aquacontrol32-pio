#ifndef _HTTP_TASK_
#define _HTTP_TASK_

#include <Arduino.h>
#include <WiFi.h>
#include <PsychicHttp.h>
#include <mutex>

#include "lightTimer.h"

extern std::vector<lightTimer_t> channel[NUMBER_OF_CHANNELS];
extern std::mutex channelMutex;

extern void messageOnLcd(const char *str);

#endif

