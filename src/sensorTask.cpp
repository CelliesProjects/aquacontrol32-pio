#include <sensorTask.hpp>

void sensorTask(void *parameter)
{
    log_i("Starting DS18B20 sensor reading...");

    pinMode(ONE_WIRE_BUS, INPUT_PULLUP); // use internal pullup for when no hw at all is connected to the pin

    sensors.begin();

    const int sensorCount = sensors.getDeviceCount();
    if (sensorCount == 0)
    {
        log_i("No DS18B20 sensors found. Stopping task.");
        vTaskDelete(NULL);
    }

    log_i("Found %d DS18B20 sensors.", sensorCount);

    log_i("Setting resolution to 12-bit for all sensors...");
    for (int i = 0; i < sensorCount; i++)
    {
        DeviceAddress address;
        if (sensors.getAddress(address, i))
        {
            sensors.setResolution(address, 12);
            log_i("Sensor %d resolution set to 12-bit.", i);
        }
        else
            log_i("Sensor %d address not found.", i);
    }

    sensors.requestTemperatures();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(750));

        for (int i = 0; i < sensorCount; i++)
        {
            DeviceAddress address;
            if (sensors.getAddress(address, i))
            {
                float temperatureC = sensors.getTempC(address);
                if (temperatureC != DEVICE_DISCONNECTED_C)
                    log_i("Sensor %d: %.2f°C", i, temperatureC);
                else
                    log_i("Sensor %d: Disconnected or error reading temperature.", i);
            }
            else
                log_i("Sensor %d: Address not found.", i);
        }

        sensors.requestTemperatures();
    }
}
