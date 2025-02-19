#ifndef _HTTPTASK_HPP_
#define _HTTPTASK_HPP_

#include <WiFi.h>
#include <FS.h>
#include <SD.h>
#include <optional>
#include <freertos/semphr.h>

#include <PsychicHttp.h>

#include "ScopedMutex.h"
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

QueueHandle_t websocketQueue = xQueueCreate(6, sizeof(websocketMessage));

constexpr char *MOON_SETTINGS_FILE = "/default.mnl";
const char *DEFAULT_TIMERFILE = "/default.aqu";

#endif

