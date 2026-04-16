#include "modbus_master.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define TAG "MODBUS"

/* ---------------- UART RS485 Configuration ---------------- */ 
#define UART_PORT   UART_NUM_1          // Use UART1 for RS485 communication
#define TXD_PIN     17                  // GPIO17 for TX
#define RXD_PIN     18                  // GPIO18 for RX
#define DE_RE_PIN   5                   // Direction control pin (1=Transmit, 0=Receive)          
#define BAUD_RATE   19200               // Modbus standard baud rate

/* Modbus Slave ID */
#define SLAVE_ID 2

/* ---------------- CRC Calculation ---------------- */
static uint16_t modbus_crc(uint8_t *buf, int len)   
{
    uint16_t crc = 0xFFFF;                      // Initial value
    for (int pos = 0; pos < len; pos++) {
        crc ^= buf[pos];                        // XOR byte with crc
        for (int i = 0; i < 8; i++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;      // Apply Polynomial
            else
                crc >>= 1;                      
        }
    }
    return crc;
}

/* ---------------- UART Initialization ---------------- */
void modbus_init(void)
{
    uart_config_t config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
    };

    // Install UART driver and configure parameters (RX buffer = 512 bytes)
    uart_driver_install(UART_PORT, 512, 0, 0, NULL, 0);

    // Configure UART parameters and pins
    uart_param_config(UART_PORT, &config);
    uart_set_pin(UART_PORT, TXD_PIN, RXD_PIN, DE_RE_PIN, UART_PIN_NO_CHANGE);
    uart_set_rx_timeout(UART_PORT, 20);                 // Set RX timeout to 20ms

    gpio_set_direction(DE_RE_PIN, GPIO_MODE_OUTPUT);    // Set DE/RE pin as output  
    gpio_set_level(DE_RE_PIN, 0);                      // Start in receive mode

    ESP_LOGI(TAG, "UART RS485 Initialized");
}


/* ---------------- Float Parsing ---------------- */
static float parse_float(uint8_t *data)
{
    uint16_t reg1 = (data[0] << 8) | data[1];   // First register
    uint16_t reg2 = (data[2] << 8) | data[3];   // Second register

    uint32_t combined = (reg2 << 16) | reg1;     // Combine registers

    float value;
    memcpy(&value, &combined, sizeof(value));    //Convert to float

    return value;
}

/* ---------------- Send Modbus Request ---------------- */
static void modbus_send_range(uint16_t start_addr, uint16_t count)
{
    uint8_t request[8];

    request[0] = SLAVE_ID;          // Slave ID
    request[1] = 0x04;              // Function Code (Read Input Registers)
    request[2] = start_addr >> 8;   // Start address (High byte)
    request[3] = start_addr & 0xFF; // Start address (Low byte)
    request[4] = count >> 8;        // Register count (High byte)
    request[5] = count & 0xFF;      // Register count (Low byte)

    // Append CRC
    uint16_t crc = modbus_crc(request, 6);
    request[6] = crc & 0xFF;        // CRC Low byte
    request[7] = crc >> 8;          // CRC High byte

    // Log transmitted frame
    ESP_LOG_BUFFER_HEX("TX", request, 8);

    // Enable transmission mode
    gpio_set_level(DE_RE_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(2));

    // Send request
    uart_write_bytes(UART_PORT, (const char *)request, 8);

    // Wait for transmission complete
    uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100));

    // Switch back to receive mode
    gpio_set_level(DE_RE_PIN, 0);
}

/* ---------------------- READ ALL PARAMETERS ---------------------- */
int read_all_parameters(float *voltage, float *current, float *power, float *energy)
{
    uint8_t response[128];

    modbus_send_range(0x0000, 28);   // Read 28 registers starting from address 0x0000 (14 registers for each parameter)
    vTaskDelay(pdMS_TO_TICKS(300));  // Wait for response (adjust as needed based on device response time)
    
    // Read response from UART
    int len = uart_read_bytes(UART_PORT, response, sizeof(response), pdMS_TO_TICKS(1000));

    // Validate response length
    if (len < 10) {
        return -1;   // Error condition
    }

    // Log received data
    ESP_LOG_BUFFER_HEX("RX", response, len);

    // Skip header (Slave ID + Function + Byte count)
    uint8_t *data = &response[3];

    // Extract values from specific register positions
    *energy  = parse_float(&data[0]);    // Energy
    *power   = parse_float(&data[28]);   // Power
    *voltage = parse_float(&data[40]);   // Voltage
    *current = parse_float(&data[44]);   // Current

    return 0;   // Success
}
