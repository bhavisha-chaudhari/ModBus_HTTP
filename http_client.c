#include "http_client.h"
#include "esp_http_client.h"
#include <string.h>
#include "esp_log.h"

#define TAG "HTTP"

// Replace this with your actual token
#define ACCESS_TOKEN "t3eqIZUFHI2CMfpUvcCY"

// Function to send data to ThingsBoard
void send_to_http(float voltage, float current, float power, float energy)
{
    char url[200];              // Buffer to hold the URL
    char post_data[128];        // Buffer to hold the JSON data

    // Build correct URL
    sprintf(url, "http://eu.thingsboard.cloud/api/v1/%s/telemetry", "t3eqIZUFHI2CMfpUvcCY");

    // JSON data
    sprintf(post_data,
        "{\"voltage\": %.2f, \"current\": %.2f, \"power\": %.2f, \"energy\": %.2f}",
        voltage, current, power, energy);

    ESP_LOGI(TAG, "URL: %s", url);              // Log the URL being used
    ESP_LOGI(TAG, "DATA: %s", post_data);       // Log the JSON data being sent

    /* ---------------- HTTP Client Configuration ---------------- */
    esp_http_client_config_t config = {
        .url = url,                     // Server URL to send the POST request to
        .method = HTTP_METHOD_POST,     // Use POST method to send data to ThingsBoard
        .timeout_ms = 8000,             // Set a timeout for the HTTP request (8 seconds)
        .transport_type = HTTP_TRANSPORT_OVER_TCP,   // Use TCP transport for HTTP communication
    };

    /* Initialize HTTP client */
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Set necessary HTTP headers for ThingsBoard
    esp_http_client_set_header(client, "Content-Type", "application/json");   
    esp_http_client_set_header(client, "Accept", "*/*");

    // Set the POST data (JSON payload) to be sent to ThingsBoard
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);   // Perform the HTTP request and get the result

    /* ---------------- Handle Response ---------------- */
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);   // Get the HTTP status code from the response
        ESP_LOGI(TAG, "HTTP Status = %d", status);              // Log the HTTP status code received from the server

        // Check if the status code indicates success (200 OK)
        if (status == 200) {
            ESP_LOGI(TAG, "Data sent successfully to ThingsBoard");
        } else {
            ESP_LOGW(TAG, "Server responded but not OK");  
        }

    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err)); 
    }

    esp_http_client_cleanup(client);   // Clean up the HTTP client 
}