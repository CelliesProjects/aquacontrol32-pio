#ifndef _LIGHTTIMER_H_
#define _LIGHTTIMER_H_

struct lightTimer_t
{
    int time;        /* time in seconds since midnight so range is 0-86400 */
    int percentage; /* in percentage so range is 0-100 */
};

#endif