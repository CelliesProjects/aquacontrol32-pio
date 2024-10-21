#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <esp_sntp.h>

#include "secrets.h"
#include "lcdMessage_t.h"

extern QueueHandle_t lcdQueue;
extern void lcdTask(void *parameter);
extern void messageOnLcd(const char *str);

extern void dimmerTask(void *parameter);

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

static void ntpSynced(void *cb_arg)
{
    sntp_set_time_sync_notification_cb(NULL);
    startDimmerTask();
    log_i("NTP synced");
}

void setup(void)
{
    Serial.begin(115200);

    while (!Serial)
        delay(10);

    delay(1000);

    log_i("\n\naquacontrol v2\n\n\n");

    SPI.begin(SCK, MISO, MOSI);
    SPI.setHwCs(true);

    if (!SD.begin(SS))
        log_w("could not mount SD");
    else
        log_w("mounted SD");

    if (!lcdQueue)
    {
        log_e("no lcd queue. system halted!");
        while (1)
            delay(100);
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
    sntp_set_time_sync_notification_cb((sntp_sync_time_cb_t)ntpSynced);
    configTzTime(TIMEZONE, NTP_POOL);
}

void loop()
{
    // put your main code here, to run repeatedly:
    delay(100);
}
