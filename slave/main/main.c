#include "slave.h"

static const char *TAG = "I2C Slave Device";

static QueueHandle_t slave_rx_queue = NULL;

typedef struct {
    uint8_t data[128];
    size_t  len;
} rx_msg_t;

static bool IRAM_ATTR slave_receive_cb(i2c_slave_dev_handle_t channel,
                                    const i2c_slave_rx_done_event_data_t *evt_data,
                                    void *user_data);

static void slave_process_task(void *arg);

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing I2C slave device...");

    slave_rx_queue = xQueueCreate(10, sizeof(rx_msg_t));
    if (NULL == slave_rx_queue) {
        ESP_LOGE(TAG, "slave_rx_queue failed");
        return;
    }

    i2c_slave_config_t slave_config = {
        .addr_bit_len       = I2C_ADDR_BIT_LEN_7,
        .clk_source         = I2C_CLK_SRC_DEFAULT,
        .i2c_port           = SLAVE_PORT,
        .scl_io_num         = SLAVE_SCL_IO,
        .sda_io_num         = SLAVE_SDA_IO,
        .slave_addr         = SLAVE_ADDR,
        .send_buf_depth     = 256,
        .receive_buf_depth  = 256,
    };

    i2c_slave_dev_handle_t slave_handle;
    ESP_ERROR_CHECK(i2c_new_slave_device(&slave_config, &slave_handle));

    i2c_slave_event_callbacks_t cbs = {
        .on_receive = slave_receive_cb,
    };
    ESP_ERROR_CHECK(i2c_slave_register_event_callbacks(slave_handle, &cbs, NULL));

    xTaskCreate(slave_process_task,
                "slave_process",
                4096,
                slave_handle,
                5,
                NULL);

    ESP_LOGI(TAG, "I2C slave device initialized and listening on address 0x%02X", SLAVE_ADDR);
}

static bool IRAM_ATTR slave_receive_cb(i2c_slave_dev_handle_t channel,
                                    const i2c_slave_rx_done_event_data_t *evt_data,
                                    void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;

    if (evt_data->length > 0 && NULL != slave_rx_queue) {
        rx_msg_t msg = {0};

        size_t copy_len = evt_data->length;
        if (copy_len > sizeof(msg.data))
            copy_len = sizeof(msg.data);

        memcpy(msg.data, evt_data->buffer, copy_len);
        msg.len = copy_len;

        xQueueSendFromISR(slave_rx_queue, &msg, &high_task_wakeup);
    }

    return (pdTRUE == high_task_wakeup);
}

static void slave_process_task(void *arg)
{
    i2c_slave_dev_handle_t handle = (i2c_slave_dev_handle_t)arg;
    rx_msg_t msg = {0};

    for (;;) {
        if (xQueueReceive(slave_rx_queue, &msg, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Received %zu bytes from I2C master", msg.len);
            ESP_LOG_BUFFER_HEX(TAG, msg.data, msg.len);
        }
    }
}