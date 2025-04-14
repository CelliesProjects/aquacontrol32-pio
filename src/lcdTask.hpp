/*
MIT License

Copyright (c) 2025 Cellie https://github.com/CelliesProjects/aquacontrol32-pio/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef _LCDTASK_HPP_
#define _LCDTASK_HPP_

#define LGFX_AUTODETECT
#include <LGFX_AUTODETECT.hpp>

#include <WiFi.h>

#include "lcdMessage.h"
#include "fonts/DejaVu24-modded.h" /* contains percent sign and a modified superscript 2 - to subscript*/
                                   /* modded with https://tchapi.github.io/Adafruit-GFX-Font-Customiser/ */

extern float currentPercentage[NUMBER_OF_CHANNELS];
extern SemaphoreHandle_t spiMutex;

QueueHandle_t lcdQueue = xQueueCreate(6, sizeof(lcdMessage_t));

static LGFX lcd;

#endif
