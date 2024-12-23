#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <esp_sntp.h>
#include <vector>
#include <mutex>

#include "secrets.h"
#include "lcdMessage_t.h"
#include "lightTime_t.h"

extern QueueHandle_t lcdQueue;
extern void lcdTask(void *parameter);
extern void messageOnLcd(const char *str);

extern void sensorTask(void *parameter);

extern void dimmerTask(void *parameter);
extern std::vector<lightTimer_t> channel[NUMBER_OF_CHANNELS];
extern std::mutex channelMutex;

static void startDimmerTask()
{
    static TaskHandle_t dimmerTaskHandle = NULL;
    if (dimmerTaskHandle && (eTaskGetState(dimmerTaskHandle) == eRunning))
    {
        log_e("can not start dimmerTask - task already running");
        return;
    }
    const auto taskResult = xTaskCreate(dimmerTask,
                                        NULL,
                                        4096 * 2,
                                        NULL,
                                        tskIDLE_PRIORITY + 2,
                                        &dimmerTaskHandle);
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
    startDimmerTask();
}
static void parseTimerFile(File &file)
{
    log_i("parsing '%s'", file.path());

    constexpr int MAX_TIME = 86400;
    constexpr int MAX_SECONDS_IN_A_DAY = 86399;
    constexpr int MAX_PERCENTAGE = 100;
    constexpr int MIN_PERCENTAGE = 0;
    constexpr int MIN_CHANNEL = 0;
    constexpr int MAX_CHANNEL = NUMBER_OF_CHANNELS - 1;

    {
        std::lock_guard<std::mutex> lock(channelMutex);

        for (int i = 0; i < NUMBER_OF_CHANNELS;)
            channel[i++].clear();

        String line = file.readStringUntil('\n');
        int currentLine = 1;
        bool error = false;

        while (file.available() && !error)
        {
            if (line.isEmpty()) // Skip empty lines
            {
                line = file.readStringUntil('\n');
                currentLine++;
                continue;
            }

            if (line.length() < 3 || line[0] != '[' || !isdigit(line[1]) || line[2] != ']')
            {
                log_e("invalid section header at line %i", currentLine);
                break;
            }

            const int currentChannel = atoi(&line[1]);
            if (currentChannel > MAX_CHANNEL || currentChannel < MIN_CHANNEL)
            {
                log_e("invalid channel number at line %i", currentLine);
                break;
            }

            log_v("current channel: %i", currentChannel);

            line = file.readStringUntil('\n');
            currentLine++;

            while ((line.length() && isdigit(line[0])) || line.isEmpty())
            {
                if (line.isEmpty()) // Skip empty lines
                {
                    line = file.readStringUntil('\n');
                    currentLine++;
                    continue;
                }

                if (line.length() < 3)
                {
                    log_e("invalid line %i", currentLine);
                    error = true;
                    break;
                }

                if (line.indexOf(",") < 1)
                {
                    log_e("invalid syntax in line %i parsing channel %i", currentLine, currentChannel);
                    error = true;
                    break;
                }

                const int time = line.toInt();
                if (time > MAX_SECONDS_IN_A_DAY || time < 0)
                {
                    log_e("invalid time value in line %i parsing channel %i", currentLine, currentChannel);
                    error = true;
                    break;
                }

                const int percentage = line.substring(line.indexOf(",") + 1).toInt();
                if (percentage > MAX_PERCENTAGE || percentage < MIN_PERCENTAGE)
                {
                    log_e("invalid percentage value in line %i parsing channel %i", currentLine, currentChannel);
                    error = true;
                    break;
                }

                auto insertPos =
                    std::lower_bound(channel[currentChannel].begin(), channel[currentChannel].end(),
                                     lightTimer_t{time, percentage}, [](const lightTimer_t &a, const lightTimer_t &b)
                                     { return a.time < b.time; });

                if (insertPos != channel[currentChannel].end() && insertPos->time == time)
                {
                    log_e("duplicate timer entry at line %i for channel %i at time %i", currentLine, currentChannel, time);
                    error = true;
                    break;
                }

                log_v("adding timer for channel %i time: %i, percent: %i", currentChannel, time, percentage);
                channel[currentChannel].insert(insertPos, {time, percentage});

                line = file.readStringUntil('\n');
                currentLine++;
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

    log_v("read %i lines", currentLine);
}

void setup(void)
{
    Serial.begin(115200);

#ifdef LGFX_M5STACK
    pinMode(DAC1, OUTPUT); // prevents high pitched noise on the builtin speaker which is connected to the first DAC
#endif

    log_i("aquacontrol v2");

    if (!lcdQueue)
    {
        log_e("no lcd queue. system halted!");
        while (1)
            delay(100);
    }

    for (int ch = 0; ch < NUMBER_OF_CHANNELS; ch++)
    {
        channel[ch].push_back({0, 0});
        channel[ch].push_back({86400, 0});
    }

    SPI.begin(SCK, MISO, MOSI);
    SPI.setHwCs(true);

    if (!SD.begin(SDCARD_SS))
        log_w("could not mount SD");
    else
    {
        log_i("mounted SD");

        const char *DEFAULT_TIMERFILE = {"/default.aqu"};
        if (SD.exists(DEFAULT_TIMERFILE))
        {
            File file = SD.open(DEFAULT_TIMERFILE);
            if (file)
            {
                parseTimerFile(file);
                file.close();
            }
        }
    }

    log_i("ch 0: %i timers", channel[0].size());
    log_i("ch 1: %i timers", channel[1].size());
    log_i("ch 2: %i timers", channel[2].size());
    log_i("ch 3: %i timers", channel[3].size());
    log_i("ch 4: %i timers", channel[4].size());

    WiFi.begin(SSID, PSK);
    WiFi.setSleep(false);

    if (!ledcSetClockSource(LEDC_USE_APB_CLK))
    {
        log_e("could not set ledc clock source. system halted!");
        while (1)
            delay(100);
    }

    BaseType_t result = xTaskCreate(lcdTask,
                                    NULL,
                                    4096,
                                    NULL,
                                    tskIDLE_PRIORITY,
                                    NULL);
    if (result != pdPASS)
    {
        log_e("could not start lcdTask. system halted!");
        while (1)
            delay(100);
    }

    result = xTaskCreate(sensorTask,
                                    NULL,
                                    4096,
                                    NULL,
                                    tskIDLE_PRIORITY,
                                    NULL);
    if (result != pdPASS)
        log_e("could not start sensorTask. system halted!");

    messageOnLcd("Wifi connecting...");

    while (!WiFi.isConnected())
        delay(10);

    messageOnLcd("Syncing clock...");

    log_i("syncing NTP");
    sntp_set_time_sync_notification_cb((sntp_sync_time_cb_t)ntpCb);
    configTzTime(TIMEZONE, NTP_POOL);

    vTaskDelete(NULL);
}

void loop() {}
