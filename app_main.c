#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "modbus_master.h"
#include "wifi.h"
#include "http_client.h"
#include "esp_log.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    wifi_init();

    vTaskDelay(5000 / portTICK_PERIOD_MS);      // Wait for Wi-Fi to connect

    modbus_init();          // Initialize RS485 Modbus communication

    meter_data_t meter;     // Structure to hold meter data

    while (1)
    {
        float voltage, current, power, energy;

        // Read all parameters from the meter
        if (read_all_parameters(&voltage, &current, &power, &energy) == 0)
        {
            /* Print values to serial monitor */
            ESP_LOGI(TAG, "Voltage: %.2f V", voltage);    
            ESP_LOGI(TAG, "Current: %.2f A", current);
            ESP_LOGI(TAG, "Power  : %.2f W", power);
            ESP_LOGI(TAG, "Energy : %.2f kWh", energy);

             send_to_http(voltage, current, power, energy);     // Send data to HTTP server
        }

        else
        {
            ESP_LOGE(TAG, "Read failed");           // Error handling if Modbus read fails

        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);      // Delay before next read (5 seconds)
    }
}