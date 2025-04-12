/*
MIT License

Copyright (c) 2025 Cellie https://github.com/CelliesProjects/aquacontrol32-pio/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
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
        updateDisplay(DEVICE_DISCONNECTED_C);
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
                lastTemperatureC = temperatureC;
            }
        }
    }
}
