#include <Arduino.h>

extern void displayTask(void *parameter);
extern void dimmerTask(void *parameter);

void setup(void)
{
    Serial.begin(115200);

    while (!Serial)
        delay(10);

    delay(1000);

    BaseType_t result = xTaskCreate(displayTask,
                                    NULL,
                                    4096,
                                    NULL,
                                    tskIDLE_PRIORITY,
                                    NULL);
    if (result != pdPASS)
        log_e("could not start displayTask");

    result = xTaskCreate(dimmerTask,
                         NULL,
                         4096,
                         NULL,
                         tskIDLE_PRIORITY,
                         NULL);
    if (result != pdPASS)
        log_e("could not start dimmerTask");
}

void loop()
{
    // put your main code here, to run repeatedly:
}
