#include <Arduino.h>

extern void displayTask(void *parameter);

void setup(void)
{
    Serial.begin(115200);
    auto result = xTaskCreate(displayTask,
                              NULL,
                              4096,
                              NULL,
                              tskIDLE_PRIORITY,
                              NULL);
    if (result != pdPASS)
        log_e("could not start displaytask");
}

void loop()
{
    // put your main code here, to run repeatedly:
}
