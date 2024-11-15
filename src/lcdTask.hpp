#ifndef _DISPLAY_TASK_
#define _DISPLAY_TASK_

#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>

#include "lcdMessage_t.h"

extern float currentPercentage[NUMBER_OF_CHANNELS];

QueueHandle_t lcdQueue = xQueueCreate(6, sizeof(lcdMessage_t));

static LGFX lcd;

#endif