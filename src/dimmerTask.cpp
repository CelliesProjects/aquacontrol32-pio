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

static inline float mapFloat(const float &x, const float &in_min, const float &in_max, const float &out_min, const float &out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void dimmerTask(void *parameter)
{
    // https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html

    const uint8_t ledPin[NUMBER_OF_CHANNELS] = {38, 39, 40, 41, 42};
    const float fullMoonLevel[NUMBER_OF_CHANNELS] = {0, 0, 0, 0, 0.06};

    const int freq = 1220;
    for (auto current = 0; current < sizeof(ledPin); current++)
        if (!ledcAttach(ledPin[current], freq, SOC_LEDC_TIMER_BIT_WIDTH))
        {
            log_e("Error setting ledc pin %i. system halted", current);
            while (1)
                delay(1000);
        }

    moonPhase moonPhase;
    moonData_t moon = moonPhase.getPhase();

    constexpr const auto TICK_RATE_HZ = 100;
    constexpr const TickType_t ticksToWait = pdTICKS_TO_MS(1000 / TICK_RATE_HZ);
    static TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, ticksToWait);

        const suseconds_t msElapsedToday = msSinceMidnight();

        if (msElapsedToday) /* to solve flashing at 00:00:000 due to the fact that the first timer has no predecessor at this time*/
        {
            for (uint8_t num = 0; num < NUMBER_OF_CHANNELS; num++)
            {
                uint8_t thisTimer = 0;
                while (channel[num][thisTimer].time * 1000U < msElapsedToday)
                    thisTimer++;

                float newPercentage;
                if (channel[num][thisTimer].percentage != channel[num][thisTimer - 1].percentage)
                {
                    newPercentage = mapFloat(msElapsedToday,
                                             channel[num][thisTimer - 1].time * 1000U,
                                             channel[num][thisTimer].time * 1000U,
                                             channel[num][thisTimer - 1].percentage,
                                             channel[num][thisTimer].percentage);
                }
                else
                    newPercentage = channel[num][thisTimer].percentage;

                if (newPercentage < fullMoonLevel[num] * moon.percentLit)
                    newPercentage = fullMoonLevel[num] * moon.percentLit;

                currentPercentage[num] = newPercentage;

                constexpr const int LEDC_MAX_VALUE = (1 << SOC_LEDC_TIMER_BIT_WIDTH) - 1;

                const bool success = ledcWrite(ledPin[num], mapFloat(currentPercentage[num],
                                                                     0,
                                                                     100,
                                                                     0,
                                                                     LEDC_MAX_VALUE));

                if (!success)
                    log_e("error setting duty cycle");
            }
        }

        constexpr const int REFRESHRATE_LCD_HZ = 5;
        constexpr const int LCD_WAIT_TIME = 1000 / REFRESHRATE_LCD_HZ;
        static unsigned long lastLcdRefresh = 0;
        if (millis() - lastLcdRefresh >= LCD_WAIT_TIME)
        {
            lcdMessage_t msg;
            msg.type = lcdMessageType::UPDATE_LIGHTS;
            xQueueSend(lcdQueue, &msg, portMAX_DELAY);
            lastLcdRefresh = millis();
        }

        constexpr const int MOON_UPDATE_INTERVAL_SECONDS = 30;
        static time_t lastMoonUpdate = time(NULL);
        if (time(NULL) - lastMoonUpdate >= MOON_UPDATE_INTERVAL_SECONDS)
        {
            moon = moonPhase.getPhase();
            lastMoonUpdate = time(NULL);
            log_i("moon visible %.1f angle %i", moon.percentLit * 100, moon.angle);
        }
        /*
                static int lps = 0;
                lps++;
                static time_t lastlps = time(NULL);
                if (time(NULL) != lastlps)
                {
                    log_i("loops per second: %i", lps);
                    lastlps++;
                    lps = 0;
                }
        */
    }
}
