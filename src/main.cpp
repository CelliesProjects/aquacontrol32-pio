#include <Arduino.h>
#include <WiFi.h>

#include "secrets.h"
#include "lcdMessage_t.h"

extern QueueHandle_t lcdQueue;
extern void lcdTask(void *parameter);
extern void messageOnLcd(const char *str);

extern void dimmerTask(void *parameter);

void setup(void)
{
    Serial.begin(115200);

    while (!Serial)
        delay(10);

    delay(1000);

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

    messageOnLcd("AquaControl v2\nWifi running\nStarting dimmerTask");

    log_i("Wifi running, starting dimmerTask");

    result = xTaskCreate(dimmerTask,
                         NULL,
                         4096,
                         NULL,
                         tskIDLE_PRIORITY,
                         NULL);
    if (result != pdPASS)
    {
        log_e("could not start dimmerTask. system halted!");
        while (1)
            delay(100);
    }
}

void loop()
{
    // put your main code here, to run repeatedly:
    delay(100);
}
