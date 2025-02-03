#ifndef _HTTP_TASK_
#define _HTTP_TASK_

#include <Arduino.h>
#include <WiFi.h>
#include <PsychicHttp.h>
#include <mutex>

#include "lightTimer.h"

extern void messageOnLcd(const char *str);

static char lastModified[30];

#endif

