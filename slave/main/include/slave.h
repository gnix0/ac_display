#ifndef SLAVE_H_
#define SLAVE_H_

#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c_slave.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define SLAVE_SCL_IO    GPIO_NUM_1
#define SLAVE_SDA_IO    GPIO_NUM_2
#define SLAVE_PORT      I2C_NUM_0
#define SLAVE_ADDR      0x32
#define SLAVE_MODE      (400000U)   // 400 kbits/s

#endif  // slave.h