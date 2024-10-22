#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <esp_sntp.h>

#include "secrets.h"
#include "lcdMessage_t.h"
#include "lightTime_t.h"

extern QueueHandle_t lcdQueue;
extern void lcdTask(void *parameter);
extern void messageOnLcd(const char *str);

extern void dimmerTask(void *parameter);
extern std::vector<lightTimer_t> channel[5];

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
    sntp_set_time_sync_notification_cb(NULL);
    startDimmerTask();
    log_i("NTP synced");
}

static void parseTimerFile(File &file)
{
    log_i("parsing '%s'", file.path());

    String line = file.readStringUntil('\n');
    int currentLine = 1;
    while (file.available())
    {
        // first of every section line should be a [0-9]
        if (line[0] != '[' || !isdigit(line[1]) || line[2] != ']')
        {
            log_e("invalid section header at line %i", currentLine);
            return;
        }
        const int currentChannel = atoi(&line[1]);

        log_v("current channel: %i", currentChannel);

        // now parse lines until the line starts with something else than 0-9
        line = file.readStringUntil('\n');
        currentLine++;

        while (isdigit(line[0]))
        {
            // check if the comma is in a plausible place etc...
            int time = line.toInt();
            int percentage = line.substring(line.indexOf(",") + 1).toInt();
            log_v("time: %i, percent: %i", time, percentage);

            channel[currentChannel].push_back({time, percentage});

            line = file.readStringUntil('\n');
            currentLine++;
        }
    }
    log_i("read %i lines", currentLine);
    log_i("ch 0: %i timers", channel[0].size());
    log_i("ch 1: %i timers", channel[1].size());
    log_i("ch 2: %i timers", channel[2].size());
    log_i("ch 3: %i timers", channel[3].size());
    log_i("ch 4: %i timers", channel[4].size());
}

void setup(void)
{
    Serial.begin(115200);

    while (!Serial)
        delay(10);

    delay(1000);

    log_i("\n\naquacontrol v2\n\n\n");

    if (!lcdQueue)
    {
        log_e("no lcd queue. system halted!");
        while (1)
            delay(100);
    }

    SPI.begin(SCK, MISO, MOSI);
    SPI.setHwCs(true);

    if (!SD.begin(SS))
        log_w("could not mount SD");
    else
    {
        log_i("mounted SD");

        const char *DEFAULT_TIMERFILE = {"/default.aqu"};
        if (SD.exists(DEFAULT_TIMERFILE))
        {
            File file = SD.open(DEFAULT_TIMERFILE);
            if (file)
                parseTimerFile(file);
        }
    }

    WiFi.begin(SSID, PSK);
    WiFi.setSleep(false);

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

    messageOnLcd("AquaControl v2\nWifi connecting...");

    while (!WiFi.isConnected())
        delay(10);

    messageOnLcd("AquaControl v2\nWifi running\nSyncing clock...");

    log_i("syncing NTP");
    sntp_set_time_sync_notification_cb((sntp_sync_time_cb_t)ntpCb);
    configTzTime(TIMEZONE, NTP_POOL);
}

void loop()
{
    // put your main code here, to run repeatedly:
    delay(100);
}
