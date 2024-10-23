#include "lcdTask.hpp"
#include "lcdMessage_t.h"

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
        sysMess.setPaletteColor(1, 200, 200, 200);
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

    sysMess.pushSprite(0, 96);
}

static void updateLights()
{
    static LGFX_Sprite lightBars(&lcd);

    if (lightBars.width() == 0 || lightBars.height() == 0)
    {
        lightBars.setColorDepth(lgfx::palette_2bit);
        if (!lightBars.createSprite(lcd.width(), 150))
        {
            log_e("could not create sprite");
            return;
        }
        lightBars.setPaletteColor(1, 200, 200, 200);
        lightBars.setPaletteColor(2, 10, 10, 200);
        lightBars.setPaletteColor(3, 10, 200, 10);
        lightBars.setTextDatum(CC_DATUM);
        lightBars.setTextColor(1);
    }
    lightBars.clear();

    const int BAR_HEIGHT = lightBars.height() - DejaVu12.yAdvance - 3;
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
        lightBars.drawCenterString(buffer, THIS_OFFSET - (w / 2), lightBars.height() - DejaVu12.yAdvance, &DejaVu12);

        const int filledHeight = mapf(currentPercentage[ch], 0, 100, 0, BAR_HEIGHT);
        lightBars.drawRect(THIS_OFFSET - (BAR_WIDTH / 2), BAR_HEIGHT, BAR_WIDTH, -BAR_HEIGHT, 1);
        lightBars.fillRect(THIS_OFFSET - (BAR_WIDTH / 2), BAR_HEIGHT, BAR_WIDTH, -filledHeight, 1);
    }
    lightBars.pushSprite(0, 60);
}

static void updateClock(const struct tm &timeinfo)
{
    const GFXfont &font = lgfx::fonts::DejaVu18;
    static LGFX_Sprite clock(&lcd);

    if (clock.width() == 0 || clock.height() == 0)
    {
        clock.setColorDepth(lgfx::palette_2bit);
        if (!clock.createSprite(lcd.width(), font.yAdvance))
        {
            log_e("could not create sprite");
            return;
        }
    }
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%c", &timeinfo);
    clock.drawCenterString(timestr, clock.width() >> 1, 0, &font);
    clock.pushSprite(0, lcd.height() - font.yAdvance);
}

void lcdTask(void *parameter)
{
    lcd.setColorDepth(lgfx::rgb565_2Byte);
    lcd.setBrightness(80);
    lcd.init();

    while (1)
    {
        lcdMessage_t msg;
        if (xQueueReceive(lcdQueue, &msg, pdTICKS_TO_MS(5)))
        {
            switch (msg.type)
            {
            case lcdMessageType::LCD_SYSTEM_MESSAGE:
                showSystemMessage(msg.str);
                break;

            case lcdMessageType::UPDATE_LIGHTS:
                updateLights();
                break;

            default:
                break;
            }
        }
        static time_t lastSecond = 0;
        if (time(NULL) != lastSecond)
        {
            struct tm timeinfo = {};
            if (getLocalTime(&timeinfo, 0))
                updateClock(timeinfo);
            lastSecond = time(NULL);
        }
    }
}
