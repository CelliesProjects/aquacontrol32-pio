#include "sensorTask.hpp"

static void updateDisplay(float temperatureC)
{
    lcdMessage_t msg;
    msg.type = TEMPERATURE;
    msg.float1 = temperatureC;
    xQueueSend(lcdQueue, &msg, portMAX_DELAY);
}

static void updateWebsocket(const float temp)
{
    websocketMessage msg;
    msg.type = TEMPERATURE_UPDATE;
    snprintf(msg.str, sizeof(msg.str), "TEMPERATURE\n%f\n", temp);
    xQueueSend(websocketQueue, &msg, portMAX_DELAY);
}

void sensorTask(void *parameter)
{
    pinMode(ONE_WIRE_PIN, INPUT_PULLUP);

    OneWire oneWire(ONE_WIRE_PIN);
    DallasTemperature sensor(&oneWire);

    sensor.begin();

    DeviceAddress sensorAddress;
    if (!sensor.getAddress(sensorAddress, 0))
    {
        log_i("No DS18B20 sensor found. Deleting task.");
        vTaskDelete(NULL);
    }
    sensor.setResolution(sensorAddress, 12);

    float lastTemperatureC = DEVICE_DISCONNECTED_C;
    int errorCount = 0;

    while (1)
    {
        sensor.requestTemperatures();

        vTaskDelay(pdMS_TO_TICKS(750));

        const float temperatureC = sensor.getTempC(sensorAddress);

        if (temperatureC == DEVICE_DISCONNECTED_C)
        {
            log_w("Sensor disconnected or error reading temperature.");

            if (++errorCount >= MAX_ERROR_COUNT)
            {               
                updateDisplay(DEVICE_DISCONNECTED_C);
                updateWebsocket(DEVICE_DISCONNECTED_C);

                log_e("Persistent sensor error after %d retries. Deleting task.", MAX_ERROR_COUNT);

                vTaskDelete(NULL);
            }
        }
        else
        {
            errorCount = 0;

            if (fabs(temperatureC - lastTemperatureC) > TEMPERATURE_THRESHOLD)
            {
                updateDisplay(temperatureC);
                updateWebsocket(temperatureC);
            }
        }
    }
}
