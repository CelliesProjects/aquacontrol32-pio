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
#include "lcdTask.hpp"
#include "ScopedMutex.h"

float mapf(const float x, const float in_min, const float in_max, const float out_min, const float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void messageOnLcd(const char *str)
{
    lcdMessage_t msg;
    snprintf(msg.str, sizeof(msg.str), "%s", str);
    msg.type = lcdMessageType::LCD_SYSTEM_MESSAGE;
    xQueueSend(lcdQueue, &msg, portMAX_DELAY);
}

void pushSpriteLocked(LGFX_Sprite &sprite, int32_t y)
{
    ScopedMutex lock(spiMutex, pdMS_TO_TICKS(5));
    if (lock.acquired())
        sprite.pushSprite(0, y);
    else
        log_w("Failed to acquire SPI mutex for pushSprite()");
}

static void showSystemMessage(char *str)
{
    const GFXfont &font = lgfx::fonts::DejaVu18;
    static LGFX_Sprite sysMess(&lcd);

    if (sysMess.width() == 0 || sysMess.height() == 0)
    {
        sysMess.setColorDepth(lgfx::palette_2bit);
        if (!sysMess.createSprite(lcd.width(), 3 * font.yAdvance))
        {
            log_e("could not create sprite");
            return;
        }
        sysMess.setPaletteColor(1, 255, 255, 255);
        sysMess.setTextDatum(CC_DATUM);
        sysMess.setTextColor(1, 0);
    }
    sysMess.clear();

    auto cnt = 0;
    char *pch = strtok(str, "\n");
    while (pch)
    {
        sysMess.drawCenterString(pch, sysMess.width() >> 1, cnt++ * font.yAdvance, &font);
        pch = strtok(NULL, "\n");
    }

    pushSpriteLocked(sysMess, 96);
}

static void updateLights()
{
    constexpr int yPos = 60;
    static LGFX_Sprite lightBars(&lcd);

    const GFXfont &font = lgfx::fonts::DejaVu12;

    if (lightBars.width() == 0 || lightBars.height() == 0)
    {
        lightBars.setColorDepth(lgfx::palette_2bit);
        if (!lightBars.createSprite(lcd.width(), lcd.height() - yPos))
        {
            log_e("could not create sprite");
            return;
        }
        lightBars.setPaletteColor(1, 255, 255, 255);
        lightBars.setPaletteColor(2, 10, 10, 200);
        lightBars.setPaletteColor(3, 10, 200, 10);
        lightBars.setTextDatum(CC_DATUM);
        lightBars.setTextColor(1);
    }
    lightBars.clear();

    const int BAR_HEIGHT = lightBars.height() - font.yAdvance - 3;
    constexpr const int BAR_WIDTH = 38;
    const int DISTANCE = lightBars.width() / NUMBER_OF_CHANNELS;
    const int HALF_DISTANCE = DISTANCE / 2;
    for (int ch = 0; ch < NUMBER_OF_CHANNELS; ch++)
    {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%03.2f%%", currentPercentage[ch]);

        int w = 0;
        lightBars.textLength(buffer, w);
        const int THIS_OFFSET = HALF_DISTANCE + (ch * DISTANCE);
        lightBars.drawCenterString(buffer, THIS_OFFSET - (w / 2), lightBars.height() - font.yAdvance, &font);

        const int filledHeight = mapf(currentPercentage[ch], 0, 100, 0, BAR_HEIGHT);
        lightBars.drawRect(THIS_OFFSET - (BAR_WIDTH / 2), BAR_HEIGHT, BAR_WIDTH, -BAR_HEIGHT, 1);
        lightBars.fillRect(THIS_OFFSET - (BAR_WIDTH / 2), BAR_HEIGHT, BAR_WIDTH, -filledHeight, 1);
    }

    pushSpriteLocked(lightBars, yPos);
}

static void showTemp(const float temperature)
{
    const GFXfont &font = DejaVu24Modded;
    static LGFX_Sprite temp(&lcd);
    if (temp.width() == 0 || temp.height() == 0)
    {
        temp.setColorDepth(lgfx::palette_2bit);
        if (!temp.createSprite(lcd.width(), font.yAdvance))
        {
            log_e("could not create sprite");
            return;
        }
        temp.setPaletteColor(1, TFT_WHITE);
        temp.setPaletteColor(2, 0x00FF00U);
        temp.setPaletteColor(3, TFT_ORANGE);
    }
    temp.clear(3);
    temp.setTextColor(0, 3);
    temp.setTextDatum(CC_DATUM);

    const size_t bgColor = (temperature == -127.0) ? 3 : 2;
    temp.clear(bgColor);
    temp.setTextColor(0, bgColor);

    if (temperature != -127.0)
    {
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%.2f°C", temperature);
        temp.drawString(buffer, temp.width() >> 1, 3 + (font.yAdvance >> 1), &font);
    }
    else
        temp.drawString("NO SENSOR", temp.width() >> 1, 3 + (font.yAdvance >> 1), &font);

    pushSpriteLocked(temp, 25);
}

void showIP(const char *ip)
{
    const GFXfont &font = DejaVu18;
    static LGFX_Sprite ipAddress(&lcd);
    if (ipAddress.width() == 0 || ipAddress.height() == 0)
    {
        ipAddress.setColorDepth(lgfx::palette_2bit);
        if (!ipAddress.createSprite(lcd.width(), font.yAdvance))
        {
            log_e("could not create sprite");
            return;
        }
        ipAddress.setPaletteColor(1, TFT_WHITE);
        ipAddress.setPaletteColor(2, 0x00FF00U);
        ipAddress.setPaletteColor(3, TFT_ORANGE);
    }
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%s", WiFi.STA.hasIP() ? ip : "NO WIFI");
    const size_t bgColor = WiFi.STA.hasIP() ? 2 : 3;
    ipAddress.clear(bgColor);
    ipAddress.setTextColor(0, bgColor);
    ipAddress.setTextDatum(CC_DATUM);
    ipAddress.drawString(buffer, ipAddress.width() >> 1, 2 + (font.yAdvance >> 1), &font);

    pushSpriteLocked(ipAddress, 0);
}

void handleNextMessage()
{
    lcdMessage_t msg;
    if (xQueueReceive(lcdQueue, &msg, 0))
    {
        switch (msg.type)
        {
        case lcdMessageType::SET_BRIGHTNESS:
            lcd.setBrightness(msg.int1);
            break;

        case lcdMessageType::LCD_SYSTEM_MESSAGE:
            showSystemMessage(msg.str);
            break;

        case lcdMessageType::UPDATE_LIGHTS:
            updateLights();
            break;

        case lcdMessageType::SHOW_IP:
            showIP(msg.str);
            break;

        case lcdMessageType::TEMPERATURE:
            showTemp(msg.float1);
            break;

        default:
            break;
        }
    }
}

void lcdTask(void *parameter)
{
    {
        ScopedMutex scopedMutex(spiMutex);
        lcd.init();
    }

    log_i("lcd init done");

    while (1)
    {
        lcdMessage_t dummy;
        if (xQueuePeek(lcdQueue, &dummy, portMAX_DELAY))
            handleNextMessage();
    }
}
