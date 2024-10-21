#include "lcdTask.hpp"
#include "lcdMessage_t.h"

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
        if (!sysMess.createSprite(lcd.width(), lcd.height()))
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

    sysMess.pushSprite(0, 0);
}

void lcdTask(void *parameter)
{
    lcd.setColorDepth(lgfx::rgb565_2Byte);
    lcd.setBrightness(140);
    lcd.init();

    while (1)
    {
        lcdMessage_t msg;
        if (xQueueReceive(lcdQueue, &msg, 0))
        {
            switch (msg.type)
            {
            case lcdMessageType::LCD_SYSTEM_MESSAGE:
                showSystemMessage(msg.str);
                break;

            default:
                break;
            }
        }
        vTaskDelay(pdTICKS_TO_MS(10));
    }
}
