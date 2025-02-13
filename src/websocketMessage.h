#ifndef _WEBSOCKETMESSAGE_H_
#define _WEBSOCKETMESSAGE_H_

enum websocketMessageType
{
    LIGHT_UPDATE,
    TEMPERATURE_UPDATE,
};

struct websocketMessage
{
    websocketMessageType type;
    char str[64];
    int32_t int1;
};

#endif