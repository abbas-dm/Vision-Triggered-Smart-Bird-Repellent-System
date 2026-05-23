#ifndef THERMAL_H
#define THERMAL_H

#include "esp_err.h"
#include <stdbool.h>

// Initialize I2C + sensor
esp_err_t thermal_init(void);

// Read temperatures
float thermal_read_ambient(void);
float thermal_read_object(void);

// Check if object hotter than threshold
bool thermal_detect_presence(float threshold_deg);

#endif