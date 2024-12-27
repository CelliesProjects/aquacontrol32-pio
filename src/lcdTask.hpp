#ifndef _DISPLAY_TASK_
#define _DISPLAY_TASK_

#define LGFX_AUTODETECT
#include <LGFX_AUTODETECT.hpp>

#include "lcdMessage.h"
#include "fonts/DejaVu24-modded.h" /* contains percent sign and a modified superscript 2 - to subscript*/
                                   /* modded with https://tchapi.github.io/Adafruit-GFX-Font-Customiser/ */

extern float currentPercentage[NUMBER_OF_CHANNELS];

QueueHandle_t lcdQueue = xQueueCreate(6, sizeof(lcdMessage_t));

static LGFX lcd;

#endif