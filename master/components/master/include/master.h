#ifndef MASTER_H_
#define MASTER_H_

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_err.h"

#define MASTER_SCL_IO   GPIO_NUM_2
#define MASTER_SDA_IO   GPIO_NUM_3
#define MASTER_PORT     I2C_NUM_0
#define SLAVE_ADDR      0x32
#define SPEED_HZ        400000      // 400 Hz

typedef struct {
    uint32_t voltages[64];
} __attribute__((packed)) display_packet_t;

esp_err_t app_master_init(i2c_master_dev_handle_t *out_dev_handle);

#endif  // master.h