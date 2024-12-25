#include <sensorTask.hpp>

void sensorTask(void *parameter)
{
    pinMode(ONE_WIRE_PIN, INPUT_PULLUP);

    OneWire oneWire(ONE_WIRE_PIN);
    DallasTemperature sensor(&oneWire);

    sensor.begin();

    DeviceAddress sensorAddress;

    if (!sensor.getAddress(sensorAddress, 0))
    {
        log_d("No DS18B20 sensor found. Deleting task.");
        vTaskDelete(NULL);
    }
    sensor.setResolution(sensorAddress, 12);

    float lastTemperatureC = DEVICE_DISCONNECTED_C;
    static int errorCount = 0;

    while (1)
    {
        sensor.requestTemperatures();

        vTaskDelay(pdMS_TO_TICKS(750));

        const float temperatureC = sensor.getTempC(sensorAddress);

        if (temperatureC == DEVICE_DISCONNECTED_C)
        {
            log_w("Sensor disconnected or error reading temperature.");
            if (++errorCount >= 10)
            {
                log_e("Persistent sensor error after 10 retries. Deleting task.");
                vTaskDelete(NULL);
            }
        }
        else
        {
            errorCount = 0;

            if (fabs(temperatureC - lastTemperatureC) > TEMPERATURE_THRESHOLD)
            {
                lcdMessage_t msg;
                msg.type = TEMPERATURE;
                msg.float1 = temperatureC;

                const BaseType_t result = xQueueSend(lcdQueue, &msg, 0);

                log_d("%s lcd temperature %.2f°C", result ? "Updated" : "Could not update", temperatureC);
                lastTemperatureC = result ? temperatureC : lastTemperatureC;
            }
        }
    }
}
