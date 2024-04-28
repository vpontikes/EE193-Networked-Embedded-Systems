#include <stdio.h>
#include "driver/i2c.h"

#ifndef DIGITAL_SENSOR_H
#define DIGITAL_SENSOR_H

esp_err_t i2c_master_init();
float pct2075_read_temp(uint8_t * sensor_data); 

#endif