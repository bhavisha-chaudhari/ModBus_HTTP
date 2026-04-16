#include "wifi.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include <stdio.h>

#define WIFI_SSID "Galaxy J80173"
#define WIFI_PASS "bhavisha@1996"

//........Event Group for WiFi connection status...........//
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

//........Event Handler...........//
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    // When Wi-Fi starts, initiate connection
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        printf("WiFi started, connecting...\n");
        esp_wifi_connect();
    } 

    // If disconnected, retry connection 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("WiFi disconnected, retrying...\n");
        esp_wifi_connect();
    } 

   // When IP is received, connection is successful 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data; 
        printf("GOT IP: " IPSTR "\n", IP2STR(&event->ip_info.ip));  // Print the obtained IP address

        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);   // Set the bit to indicate WiFi is connected
    }
}

//..........WiFi Init..............//
void wifi_init(void)
{
    nvs_flash_init();                       // Initialize NVS flash for WiFi storage
    esp_netif_init();                       // Initialize network interface 
    esp_event_loop_create_default();        // Create default event loop
    esp_netif_create_default_wifi_sta();    // Create default WiFi station network interface

    wifi_event_group = xEventGroupCreate(); // Create event group for WiFi connection status

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();    // Get default WiFi configuration
    esp_wifi_init(&cfg);

    // Register event handlers for WiFi and IP events
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);

    // Register handler for IP events to know when we get an IP address
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    // WiFi config
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    // Start WiFi
    esp_wifi_set_mode(WIFI_MODE_STA);                     // Set WiFi to station mode
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);      // Set WiFi configuration for station interface
    esp_wifi_start();                                    // Start WiFi

    printf("WiFi init complete\n");                     // Print message indicating WiFi initialization is complete

    //.....WAIT until connected.....//
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    printf("WiFi Connected Successfully!\n");   // Print message indicating successful WiFi connection
}