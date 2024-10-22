#ifndef _LIGHT_TIMER_T_
#define _LIGHT_TIMER_T_

#include <Arduino.h>

struct lightTimer_t
{
    int time;        /* time in seconds since midnight so range is 0-86400 */
    int percentage; /* in percentage so range is 0-100 */
};

#endif