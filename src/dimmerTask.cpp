#include "dimmerTask.hpp"

static unsigned long long msSinceMidnight()
{
    // Time should already be synced through SNTP!
    struct timeval now;
    gettimeofday(&now, NULL);

    struct tm currentTime;
    localtime_r(&now.tv_sec, &currentTime);

    int secondsSinceMidnight =
        currentTime.tm_hour * 3600 +
        currentTime.tm_min * 60 +
        currentTime.tm_sec;

    unsigned long long millisecondsSinceMidnight =
        (unsigned long long)secondsSinceMidnight * 1000ULL +
        now.tv_usec / 1000;

    return millisecondsSinceMidnight;
}

void dimmerTask(void *parameter)
{
    // https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html

    // setup ledc
    const int freq = 1200;
    const uint8_t ledPin[] = {38, 39, 40, 41, 42};

    for (auto pin = 0; pin < sizeof(ledPin); pin++)
        if (!ledcAttachChannel(ledPin[pin], freq, SOC_LEDC_TIMER_BIT_WIDTH, pin + 1)) // plus 1 because the tft backlight is channel 0
        {
            log_e("Error setting ledc channel %i. system halted", pin + 1);
            while (1)
                delay(1000);
        }

    constexpr const auto TICK_RATE_HZ = 1;
    constexpr const TickType_t ticksToWait = pdTICKS_TO_MS(1000 / TICK_RATE_HZ);
    static TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, ticksToWait);
        log_v("%llu milliseconds since midnight", msSinceMidnight());
    }
}