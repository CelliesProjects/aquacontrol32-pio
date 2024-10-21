#ifndef _LCD_MESSAGE_T_
#define _LCD_MESSAGE_T_

enum lcdMessageType
{
    LCD_SYSTEM_MESSAGE
};

struct lcdMessage_t
{
    lcdMessageType type;
    char str[64];
};

#endif