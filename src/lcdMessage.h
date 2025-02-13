#ifndef _LCDMESSAGE_H_
#define _LCDMESSAGE_H_

enum lcdMessageType
{
    SET_BRIGHTNESS,
    LCD_SYSTEM_MESSAGE,
    UPDATE_LIGHTS,
    TEMPERATURE,
    SHOW_IP
};

struct lcdMessage_t
{
    lcdMessageType type;
    char str[64];
    int32_t int1;
    float float1;
};

#endif