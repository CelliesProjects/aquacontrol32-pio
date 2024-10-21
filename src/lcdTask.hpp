#ifndef _DISPLAY_TASK_
#define _DISPLAY_TASK_

#define LGFX_ESP32_S3_BOX_LITE 
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>

#include "lcdMessage_t.h"

QueueHandle_t lcdQueue = xQueueCreate(6, sizeof(lcdMessage_t));

static LGFX lcd;

#endif