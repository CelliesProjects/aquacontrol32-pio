#ifndef SCOPEDMUTEX_H
#define SCOPEDMUTEX_H

#include <Arduino.h>
#include <freertos/semphr.h>

class ScopedMutex
{
private:
    SemaphoreHandle_t &mutex;
    bool locked;

public:
    ScopedMutex(SemaphoreHandle_t &m, TickType_t timeout = pdMS_TO_TICKS(1000)) : mutex(m), locked(xSemaphoreTake(mutex, timeout)) {}

    ~ScopedMutex()
    {
        if (locked)
            xSemaphoreGive(mutex);
    }

    bool acquired() const { return locked; }
};

#endif // SCOPEDMUTEX_H
