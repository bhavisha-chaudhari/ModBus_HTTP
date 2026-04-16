#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H

#include <stdint.h>

// Define UART parameters
typedef struct {float voltage;
    float current;
    float power;
    float energy;
} meter_data_t;     // Structure to hold meter data

void modbus_init(void);     // Initialize UART for Modbus communication
int read_all_parameters(float *voltage, float *current, float *power, float *energy);   // Read all parameters from the meter

#endif