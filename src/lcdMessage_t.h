#ifndef _LCD_MESSAGE_T_
#define _LCD_MESSAGE_T_

enum lcdMessageType
{
    LCD_SYSTEM_MESSAGE,
    UPDATE_LIGHTS,
    MOON_PHASE
};

struct lcdMessage_t
{
    lcdMessageType type;
    char str[64];
    int32_t int1;
    float float1;
};

#endif