/*
MIT License

Copyright (c) 2025 Cellie https://github.com/CelliesProjects/aquacontrol32-pio/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <esp_sntp.h>
#include <NetworkEvents.h>
#include <freertos/semphr.h>
#include <vector>

#include "ScopedMutex.h"
#include "ScopedFile.h"
#include "secrets.h"
#include "lcdMessage.h"
#include "lightTimer.h"

SemaphoreHandle_t spiMutex;

extern const char *DEFAULT_TIMERFILE;

extern QueueHandle_t lcdQueue;
extern void messageOnLcd(const char *str);

extern void dimmerTask(void *parameter);
extern void httpTask(void *parameter);
extern void lcdTask(void *parameter);
extern void sensorTask(void *parameter);
extern bool loadMoonSettings(String &result);

extern std::vector<lightTimer_t> channel[NUMBER_OF_CHANNELS];
extern SemaphoreHandle_t channelMutex;
extern float fullMoonLevel[NUMBER_OF_CHANNELS];

bool sensorTaskRunning = false;
static TaskHandle_t sensorTaskHandle = nullptr;
static SemaphoreHandle_t sensorTaskMutex = xSemaphoreCreateMutex();

static void showIPonDisplay()
{
    lcdMessage_t msg;
    msg.type = SHOW_IP;
    snprintf(msg.str, sizeof(msg.str), "%s", WiFi.localIP().toString().c_str());
    xQueueSend(lcdQueue, &msg, portMAX_DELAY);
}

static void WiFiEvent(arduino_event_id_t event)
{
    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        showIPonDisplay();
        break;
    default:
        break;
    }
}

static void startDimmerTask()
{
    static TaskHandle_t dimmerTaskHandle = NULL;
    if (dimmerTaskHandle && (eTaskGetState(dimmerTaskHandle) == eRunning))
    {
        log_e("can not start dimmerTask - task already running");
        return;
    }

    const auto taskResult = xTaskCreatePinnedToCore(dimmerTask,
                                                    "dimmerTask",
                                                    1024 * 8,
                                                    NULL,
                                                    tskIDLE_PRIORITY + 5,
                                                    &dimmerTaskHandle,
                                                    APP_CPU_NUM);
    if (taskResult != pdPASS)
    {
        log_e("could not start dimmerTask. system halted!");
        while (1)
            delay(100);
    }
}

static void ntpCb(void *cb_arg)
{
    log_i("NTP synced");
    sntp_set_time_sync_notification_cb(NULL);

    const BaseType_t result = xTaskCreate(httpTask,
                                          "httpTask",
                                          4096,
                                          NULL,
                                          tskIDLE_PRIORITY,
                                          NULL);
    if (result != pdPASS)
    {
        log_e("could not start httpTask. system halted!");
        while (1)
            delay(100);
    }

    startDimmerTask();
}

static bool parseTimerFile(File &file, String &result)
{
    log_i("parsing '%s'", file.path());

    constexpr int MAX_TIME = 86400;
    constexpr int MAX_SECONDS_IN_A_DAY = 86399;
    constexpr int MAX_PERCENTAGE = 100;
    constexpr int MIN_PERCENTAGE = 0;
    constexpr int MIN_CHANNEL = 0;
    constexpr int MAX_CHANNEL = NUMBER_OF_CHANNELS - 1;

    {
        ScopedMutex lock(channelMutex, pdMS_TO_TICKS(1000));
        if (!lock.acquired())
        {
            result = "Mutex timeout";
            return false;
        }

        for (int i = 0; i < NUMBER_OF_CHANNELS;)
            channel[i++].clear();

        String line = file.readStringUntil('\n');
        int currentLine = 1;

        while (file.available())
        {
            if (line.isEmpty())
            {
                line = file.readStringUntil('\n');
                currentLine++;
                continue;
            }

            if (line.length() < 3 || line[0] != '[' || !isdigit(line[1]) || line[2] != ']')
            {
                result = "invalid section header at line " + String(currentLine);
                return false;
            }

            const int currentChannel = atoi(&line[1]);
            if (currentChannel > MAX_CHANNEL || currentChannel < MIN_CHANNEL)
            {
                result = "invalid channel number at line " + String(currentLine);
                return false;
            }

            log_v("current channel: %i", currentChannel);

            line = file.readStringUntil('\n');
            currentLine++;

            while ((line.length() && isdigit(line[0])) || line.isEmpty())
            {
                if (line.isEmpty())
                {
                    line = file.readStringUntil('\n');
                    currentLine++;
                    if (line.isEmpty() && !file.available())
                        break;
                    continue;
                }

                if (line.length() < 3)
                {
                    result = "invalid line " + String(currentLine);
                    return false;
                }

                if (line.indexOf(",") < 1)
                {
                    result = "invalid syntax in line " + String(currentLine) + " parsing channel " + String(currentChannel);
                    return false;
                }

                const int time = line.toInt();
                if (time > MAX_SECONDS_IN_A_DAY || time < 0)
                {
                    result = "invalid time value in line " + String(currentLine) + " parsing channel " + String(currentChannel);
                    return false;
                }

                const int percentage = line.substring(line.indexOf(",") + 1).toInt();
                if (percentage > MAX_PERCENTAGE || percentage < MIN_PERCENTAGE)
                {
                    result = "invalid percentage value in line " + String(currentLine) + " parsing channel " + String(currentChannel);
                    return false;
                }

                auto insertPos =
                    std::lower_bound(channel[currentChannel].begin(), channel[currentChannel].end(),
                                     lightTimer_t{time, percentage}, [](const lightTimer_t &a, const lightTimer_t &b)
                                     { return a.time < b.time; });

                if (insertPos != channel[currentChannel].end() && insertPos->time == time)
                {
                    result = "duplicate timer entry at line " + String(currentLine) + " for channel " + String(currentChannel) + " at time " + String(time);
                    return false;
                }

                log_v("adding timer for channel %i time: %i, percent: %i", currentChannel, time, percentage);
                channel[currentChannel].insert(insertPos, {time, percentage});

                line = file.readStringUntil('\n');
                currentLine++;
                if (line.isEmpty() && !file.available())
                    break;
            }
        }

        for (int index = 0; index < NUMBER_OF_CHANNELS; index++)
            if (channel[index].size())
                channel[index].push_back({MAX_TIME, channel[index][0].percentage});
            else
            {
                channel[index].push_back({0, 0});
                channel[index].push_back({MAX_TIME, 0});
            }
    }
    result = "Timers processed";
    return true;
}

bool saveDefaultTimers(String &result)
{
    ScopedMutex lock(spiMutex, pdMS_TO_TICKS(1000));
    if (!lock.acquired())
    {
        result = "Mutex timeout";
        log_w("%s", result.c_str());
        return false;
    }

    ScopedFile scopedFile(DEFAULT_TIMERFILE, FileMode::Write, SDCARD_SS, 20000000);
    if (!scopedFile.isValid())
    {
        result = "SD Card mount or file open failed";
        return false;
    }

    File &file = scopedFile.get();
    {
        ScopedMutex lock(channelMutex, pdMS_TO_TICKS(1000));
        if (!lock.acquired())
        {
            result = "Mutex timeout";
            log_w("%s", result.c_str());
            return false;
        }

        for (int i = 0; i < NUMBER_OF_CHANNELS; ++i)
        {
            file.printf("[%d]\n", i); // Write channel header
            for (const auto &timer : channel[i])
                if (timer.time != 86400)
                    file.printf("%d,%d\n", timer.time, timer.percentage);
        }
    }

    result = "Saved timers to ";
    result.concat(DEFAULT_TIMERFILE);
    log_i("%s", result.c_str());
    return true;
}

bool loadDefaultTimers(String &result)
{
    ScopedMutex lock(spiMutex, pdMS_TO_TICKS(1000));
    if (!lock.acquired())
    {
        result = "Mutex timeout";
        return false;
    }

    ScopedFile scopedFile(DEFAULT_TIMERFILE, FileMode::Read, SDCARD_SS, 20000000);
    if (!scopedFile.isValid())
    {
        result = "SD Card mount or file open failed";
        return false;
    }

    File &file = scopedFile.get();
    const bool success = parseTimerFile(file, result);
    return success;
}

bool startSensor()
{
    ScopedMutex lock(sensorTaskMutex);

    if (sensorTaskHandle == nullptr)
    {
        // Task never created — create it
        const BaseType_t result = xTaskCreate(
            sensorTask,
            "sensorTask",
            4096,
            NULL,
            tskIDLE_PRIORITY,
            &sensorTaskHandle);

        if (result != pdPASS)
        {
            log_e("Failed to create sensorTask");
            sensorTaskHandle = nullptr;
            return false;
        }

        log_i("sensorTask created");
        return true;
    }

    // If task already exists, resume it if suspended
    eTaskState state = eTaskGetState(sensorTaskHandle);
    if (state == eSuspended)
    {
        vTaskResume(sensorTaskHandle);
        log_i("sensorTask resumed");
        return true;
    }

    log_v("sensorTask already running (state: %d)", state);
    return false;
}

void setup(void)
{
    Serial.begin(115200);

#if defined(LGFX_M5STACK) || defined(LGFX_M5STACK_CORE2)
    pinMode(DAC1, OUTPUT);
    digitalWrite(DAC1, LOW); // prevents high pitched noise on the builtin speaker which is connected to the first DAC
#endif

    log_i("aquacontrol32-pio");

    if (!sensorTaskMutex)
    {
        log_e("Failed to create sensorTask Mutex! system halted!");
        while (1)
            delay(100);
    }
    xSemaphoreGive(sensorTaskMutex);

    spiMutex = xSemaphoreCreateMutex();
    if (!spiMutex)
    {
        log_e("Failed to create spi mutex! system halted!");
        while (1)
            delay(100);
    }
    xSemaphoreGive(spiMutex);

    channelMutex = xSemaphoreCreateMutex();
    if (!channelMutex)
    {
        log_e("Failed to create channel mutex! system halted!");
        while (1)
            delay(100);
    }
    xSemaphoreGive(channelMutex);

    for (int ch = 0; ch < NUMBER_OF_CHANNELS; ch++)
    {
        channel[ch].push_back({0, 0});
        channel[ch].push_back({86400, 0});
    }

    SPI.begin(SCK, MISO, MOSI);
    SPI.setHwCs(true);

    {
        String result;
        loadDefaultTimers(result);
        log_i("%s", result.c_str());
    }

    log_i("ch 0: %i timers", channel[0].size());
    log_i("ch 1: %i timers", channel[1].size());
    log_i("ch 2: %i timers", channel[2].size());
    log_i("ch 3: %i timers", channel[3].size());
    log_i("ch 4: %i timers", channel[4].size());

    {
        String result;
        loadMoonSettings(result);
        log_i("%s", result.c_str());
    }

    if (SET_STATIC_IP && !WiFi.config(STATIC_IP, GATEWAY, SUBNET, PRIMARY_DNS))
        log_i("Setting static IP failed");

    btStop();
    WiFi.onEvent(WiFiEvent);
    WiFi.setAutoReconnect(true);
    WiFi.begin(SSID, PSK);
    WiFi.setSleep(false);

    if (!ledcSetClockSource(LEDC_USE_APB_CLK))
    {
        log_e("could not set ledc clock source. system halted!");
        while (1)
            delay(100);
    }

#ifndef HEADLESS_BUILD
    if (!lcdQueue)
    {
        log_e("no lcd queue. system halted!");
        while (1)
            delay(100);
    }

    showIPonDisplay();

    BaseType_t result = xTaskCreatePinnedToCore(lcdTask,
                                                "lcdTask",
                                                1024 * 3,
                                                NULL,
                                                tskIDLE_PRIORITY + 1,
                                                NULL,
                                                APP_CPU_NUM);
    if (result != pdPASS)
    {
        log_e("could not start lcdTask. system halted!");
        while (1)
            delay(100);
    }

    if (!startSensor())
    {
        log_e("could not start sensorTask. system halted!");
        while (1)
            delay(100);
    }

    messageOnLcd("Wifi connecting...");
#endif

    while (!WiFi.isConnected())
        delay(10);

#ifndef HEADLESS_BUILD
    showIPonDisplay();
    messageOnLcd("Syncing clock...");
#endif

    log_i("syncing NTP");
    sntp_set_time_sync_notification_cb((sntp_sync_time_cb_t)ntpCb);
    configTzTime(TIMEZONE, NTP_POOL);

    vTaskDelete(NULL);
}

void loop() {}
