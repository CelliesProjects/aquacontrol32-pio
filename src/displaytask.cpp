#include "displaytask.hpp"

void displayTask(void *parameter)
{
    lcd.init();
    lcd.setTextDatum(CC_DATUM);
    lcd.drawString("Hello world!", lcd.width() >> 1, lcd.height() >> 1, &Font4);    

    while (1)
    {
        vTaskDelay(pdTICKS_TO_MS(100));
    }
}
