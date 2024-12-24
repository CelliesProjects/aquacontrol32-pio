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
    sensor.requestTemperatures();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(750));

        const float temperatureC = sensor.getTempC(sensorAddress);

        if (temperatureC != DEVICE_DISCONNECTED_C)
            log_i("Temperature: %.2f°C", temperatureC);
        else
            log_i("Sensor disconnected or error reading temperature.");

        sensor.requestTemperatures();
    }
}
