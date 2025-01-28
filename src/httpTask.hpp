#ifndef _HTTP_TASK_
#define _HTTP_TASK_

#include <Arduino.h>
#include <WiFi.h>
#include <PsychicHttp.h>

extern void messageOnLcd(const char *str);

PsychicHttpServer server;

#endif