#ifndef _HTTP_TASK_
#define _HTTP_TASK_

#include <Arduino.h>
#include <WiFi.h>
#include <FS.h>
#include <SD.h>
#include <mutex>
#include <optional>

#include <PsychicHttp.h>

#include "lightTimer.h"
#include "websocketMessage.h"

extern std::vector<lightTimer_t> channel[NUMBER_OF_CHANNELS];
extern std::mutex channelMutex;

extern bool saveDefaultTimers();
extern bool loadDefaultTimers();
extern void messageOnLcd(const char *str);

QueueHandle_t websocketQueue = xQueueCreate(6, sizeof(websocketMessage));

#endif

