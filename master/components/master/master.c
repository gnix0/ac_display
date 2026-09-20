#include "master.h"

static const char *TAG = "I2C Master Config Module";

esp_err_t app_master_init(i2c_master_dev_handle_t *out_dev_handle)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Initializing I2C Master bus on SCL:%d SDA:%d", MASTER_SCL_IO, MASTER_SDA_IO);

    i2c_master_bus_config_t master_config = {
        .scl_io_num                     = MASTER_SCL_IO,
        .sda_io_num                     = MASTER_SDA_IO,
        .i2c_port                       = MASTER_PORT,
        .clk_source                     = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt              = 7,
        .flags.enable_internal_pullup   = true,
    };

    i2c_master_bus_handle_t bus_handle = {0};
    ret = i2c_new_master_bus(&master_config, &bus_handle);
    if (ESP_OK != ret) {
        ESP_LOGE(TAG, "Failed to initialize bus: %s", esp_err_to_name(ret));
        return ret;
    }

    i2c_device_config_t device_config = {
        .dev_addr_length    = I2C_ADDR_BIT_LEN_7,
        .device_address     = SLAVE_ADDR,
        .scl_speed_hz       = SPEED_HZ,
    };
    ret = i2c_master_bus_add_device(bus_handle, &device_config, out_dev_handle);
    if (ESP_OK != ret)
        ESP_LOGE(TAG, "Failed to add device 0x%02X: %s", SLAVE_ADDR, esp_err_to_name(ret));
    
    return ret;
}