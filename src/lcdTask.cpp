#include "lcdTask.hpp"

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
        lightBars.setPaletteColor(1, 255, 255, 255);
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

static void showMoon(float percentlit, int angle)
{
    char buffer[64];
    lcd.setTextColor(lcd.color565(220, 220, 220), 0);
    snprintf(buffer, sizeof(buffer), "moon % 3.4f%% lit   angle %03i", percentlit, angle);
    lcd.drawCenterString(buffer, lcd.width() >> 1, 5, &DejaVu12);
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
    }
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%.2f°C", temperature);
    temp.drawCenterString(buffer, temp.width() >> 1, 6, &font);
    temp.pushSprite(0, 15);
}

void showIP(const char *ip)
{
    lcd.drawCenterString(ip, lcd.width() >> 1, 2, &DejaVu18);
}

void lcdTask(void *parameter)
{
    if (xSemaphoreTake(spiMutex, portMAX_DELAY) == pdTRUE)
    {
        lcd.init();
        xSemaphoreGive(spiMutex);
    }

    while (1)
    {
        if (xSemaphoreTake(spiMutex, 0) == pdTRUE)
        {
            lcdMessage_t msg;
            if (xQueueReceive(lcdQueue, &msg, pdTICKS_TO_MS(5)))
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
                    /*
                                case lcdMessageType::MOON_PHASE:
                                    showMoon(msg.float1, msg.int1);
                                    break;
                    */

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

            static time_t lastSecond = 0;
            if (time(NULL) != lastSecond)
            {
                struct tm timeinfo = {};
                if (getLocalTime(&timeinfo, 0))
                    updateClock(timeinfo);
                lastSecond = time(NULL);
            }
            xSemaphoreGive(spiMutex);
        }
    }
}
