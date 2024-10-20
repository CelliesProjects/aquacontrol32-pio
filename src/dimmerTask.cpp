#include "dimmerTask.hpp"

void dimmerTask(void *parameter)
{
    // https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html

    // setup ledc
    const int freq = 2400;
    const uint8_t ledPin[] = {36, 39, 40, 41, 42};

    for (auto pin = 0; pin < sizeof(ledPin); pin++)
        ledcAttachChannel(ledPin[pin], freq, SOC_LEDC_TIMER_BIT_WIDTH, pin + 1);

    log_i("done");

    while (1)
    {
        vTaskDelay(pdTICKS_TO_MS(100));
    }
}