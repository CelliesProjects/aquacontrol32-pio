#ifndef _WEBSOCKET_MESSAGE_
#define _WEBSOCKET_MESSAGE_

#include <stdint.h>

enum websocketMessageType
{
    LIGHT_UPDATE,
    TEMPERATURE_UPDATE,
    // MOON_UPDATE
};

struct websocketMessage
{
    websocketMessageType type;
    char str[64];
    int32_t int1;
};

#endif