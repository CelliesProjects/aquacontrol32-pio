#include <sensorTask.hpp>

void sensorTask(void *parameter)
{
    log_i("Searching DS18B20 sensor");

    pinMode(ONE_WIRE_BUS, INPUT_PULLUP);

    sensor.begin();

    DeviceAddress sensorAddress;

    if (!sensor.getAddress(sensorAddress, 0))
    {
        log_i("No DS18B20 sensor found. Deleting task.");
        vTaskDelete(NULL);
    }

    sensor.setResolution(sensorAddress, 12);
    log_i("Sensor initialized with 12-bit resolution.");

    sensor.requestTemperatures();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(750));

        float temperatureC = sensor.getTempC(sensorAddress);

        if (temperatureC != DEVICE_DISCONNECTED_C)
            log_i("Temperature: %.2f°C", temperatureC);
        else
            log_i("Sensor disconnected or error reading temperature.");

        sensor.requestTemperatures();
    }
}

