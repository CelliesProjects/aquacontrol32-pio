#ifndef _HTTP_TASK_
#define _HTTP_TASK_

#include <Arduino.h>
#include <WiFi.h>
#include <FS.h>
#include <SD.h>
#include <mutex>
#include <optional>
#include <sstream> // For std::stringstrea
#include <time.h> 

#include <PsychicHttp.h>

#include "lightTimer.h"
#include "websocketMessage.h"
#include "build_epoch.h"

extern const char *WEBIF_USER;
extern const char *WEBIF_PASSWORD;

extern std::vector<lightTimer_t> channel[NUMBER_OF_CHANNELS];
extern std::mutex channelMutex;

extern bool saveDefaultTimers(String &result);
extern bool loadDefaultTimers();
extern void messageOnLcd(const char *str);

QueueHandle_t websocketQueue = xQueueCreate(6, sizeof(websocketMessage));

#endif

