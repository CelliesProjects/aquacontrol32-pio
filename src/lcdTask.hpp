#ifndef _DISPLAY_TASK_
#define _DISPLAY_TASK_

#define LGFX_AUTODETECT
#include <LGFX_AUTODETECT.hpp>

#include "lcdMessage_t.h"
#include "fonts/DejaVu24-modded.h" /* contains percent sign and superscript 2*/

extern float currentPercentage[NUMBER_OF_CHANNELS];

QueueHandle_t lcdQueue = xQueueCreate(6, sizeof(lcdMessage_t));

static LGFX lcd;

#endif