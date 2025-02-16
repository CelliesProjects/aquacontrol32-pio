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

    ScopedMutex scopedMutex(spiMutex);
    if (scopedMutex.acquired())
        sysMess.pushSprite(0, 96);
    else
        log_w("Failed to acquire SPI mutex for pushSprite()");
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

    ScopedMutex scopedMutex(spiMutex);
    if (scopedMutex.acquired())
        lightBars.pushSprite(0, yPos);
    else
        log_w("Failed to acquire SPI mutex for pushSprite()");
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
        temp.setPaletteColor(3, TFT_GREEN);
    }
    temp.clear(3);
    temp.setTextColor(0, 3);
    temp.setTextDatum(CC_DATUM);
    if (temperature != -127.0)
    {
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%.2f°C", temperature);
        temp.drawString(buffer, temp.width() >> 1, 3 + (font.yAdvance >> 1), &font);
    }
    else
        temp.drawString("NO SENSOR", temp.width() >> 1, 3 + (font.yAdvance >> 1), &font);

    ScopedMutex scopedMutex(spiMutex);
    if (scopedMutex.acquired())
        temp.pushSprite(0, 25);
    else
        log_w("Failed to acquire SPI mutex for pushSprite()");
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
        ipAddress.setPaletteColor(3, TFT_DARKCYAN);
    }
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%s", ip);
    ipAddress.clear(3);
    ipAddress.setTextColor(0, 3);
    ipAddress.setTextDatum(CC_DATUM);
    ipAddress.drawString(buffer, ipAddress.width() >> 1, 2 + (font.yAdvance >> 1), &font);

    ScopedMutex scopedMutex(spiMutex);
    if (scopedMutex.acquired())
        ipAddress.pushSprite(0, 0);
    else
        log_w("Failed to acquire SPI mutex for pushSprite()");
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
    if (!spiMutex)
    {
        log_e("SPI mutex not initialized");
        vTaskDelete(NULL);
    }

    {
        ScopedMutex scopedMutex(spiMutex, portMAX_DELAY);
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
