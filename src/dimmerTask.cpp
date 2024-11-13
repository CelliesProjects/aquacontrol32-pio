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
    constexpr const uint8_t ledPin[NUMBER_OF_CHANNELS] = {38, 39, 40, 41, 42};
    constexpr const float fullMoonLevel[NUMBER_OF_CHANNELS] = {0, 0, 0, 0, 0.06};

    const int freq = 1200;
    for (int index = 0; index < NUMBER_OF_CHANNELS; index++)
        if (!ledcAttachChannel(ledPin[index], freq, SOC_LEDC_TIMER_BIT_WIDTH, index + 2))
        {
            log_e("Error setting ledc pin %i. system halted", index);
            while (1)
                delay(1000);
        }

    moonPhase moonPhase;
    moonData_t moon = moonPhase.getPhase();

    {
        lcdMessage_t msg;
        msg.type = lcdMessageType::MOON_PHASE;
        msg.float1 = moon.percentLit * 100;
        msg.int1 = moon.angle;
        xQueueSend(lcdQueue, &msg, portMAX_DELAY);
    }

    constexpr const int TICK_RATE_HZ = 100;
    const TickType_t ticksToWait = pdTICKS_TO_MS(1000 / TICK_RATE_HZ);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        vTaskDelayUntil(&xLastWakeTime, ticksToWait);
        const suseconds_t msElapsedToday = msSinceMidnight();

        if (msElapsedToday) /* to solve flashing at 00:00:000 due to the fact that the first timer has no predecessor at this time */
        {
            std::lock_guard<std::mutex> lock(channelMutex);

            for (int index = 0; index < NUMBER_OF_CHANNELS; index++)
            {
                int currentTimer = 0;
                while (channel[index][currentTimer].time * 1000U < msElapsedToday)
                    currentTimer++;

                currentTimer = (currentTimer >= channel[index].size()) ? channel[index].size() - 1 : currentTimer;

                const float newPercentage =
                    (currentTimer > 0 && channel[index][currentTimer].percentage != channel[index][currentTimer - 1].percentage)
                        ? mapf(msElapsedToday,
                               channel[index][currentTimer - 1].time * 1000U,
                               channel[index][currentTimer].time * 1000U,
                               channel[index][currentTimer - 1].percentage,
                               channel[index][currentTimer].percentage)
                        : channel[index][currentTimer].percentage;

                const float currentMoonLevel = fullMoonLevel[index] * moon.percentLit;

                currentPercentage[index] = newPercentage < currentMoonLevel ? currentMoonLevel : newPercentage;

                constexpr const int LEDC_MAX_VALUE = (1 << SOC_LEDC_TIMER_BIT_WIDTH) - 1;
                const int dutyCycle = mapf(currentPercentage[index], 0, 100, 0, LEDC_MAX_VALUE);

                if (!ledcWrite(ledPin[index], dutyCycle))
                    log_w("Error setting duty cycle %i on pin %i", dutyCycle, ledPin[index]);
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

        constexpr const int MOON_UPDATE_INTERVAL_SECONDS = 1;
        static time_t lastMoonUpdate = time(NULL);
        if (time(NULL) - lastMoonUpdate >= MOON_UPDATE_INTERVAL_SECONDS)
        {
            moon = moonPhase.getPhase();
            lastMoonUpdate = time(NULL);
            log_d("moon visible %f angle %i", moon.percentLit * 100, moon.angle);

            lcdMessage_t msg;
            msg.type = lcdMessageType::MOON_PHASE;
            msg.float1 = moon.percentLit * 100;
            msg.int1 = moon.angle;
            xQueueSend(lcdQueue, &msg, portMAX_DELAY);
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
