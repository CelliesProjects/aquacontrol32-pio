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
    explicit ScopedMutex(SemaphoreHandle_t &m) : mutex(m), locked(xSemaphoreTake(mutex, pdMS_TO_TICKS(1000))) {}

    ~ScopedMutex()
    {
        if (locked)
        {
            xSemaphoreGive(mutex);
        }
    }

    bool acquired() const { return locked; }
};

#endif // SCOPEDMUTEX_H
