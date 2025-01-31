#ifndef _LCD_MESSAGE_T_
#define _LCD_MESSAGE_T_

enum lcdMessageType
{
    SET_BRIGHTNESS,
    LCD_SYSTEM_MESSAGE,
    UPDATE_LIGHTS,
    MOON_PHASE,
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