#include "slave.h"
#include "display.h"

static const char *TAG = "I2C Slave Device";

static QueueHandle_t slave_rx_queue = NULL;

typedef struct {
    display_packet_t    packet;
    size_t              len;
} rx_msg_t;

static bool IRAM_ATTR slave_receive_cb(i2c_slave_dev_handle_t channel,
                                    const i2c_slave_rx_done_event_data_t *evt_data,
                                    void *user_data);

static void slave_process_task(void *arg);

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing I2C slave device...");

    display_init_ui();

    slave_rx_queue = xQueueCreate(1, sizeof(rx_msg_t));
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
        .send_buf_depth     = 1024,
        .receive_buf_depth  = 1024,
    };

    i2c_slave_dev_handle_t slave_handle = {0};
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

static bool slave_receive_cb(i2c_slave_dev_handle_t channel,
                             const i2c_slave_rx_done_event_data_t *evt_data,
                             void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;

    if (evt_data->length > 0 && NULL != slave_rx_queue) {
        static rx_msg_t msg; 

        size_t copy_len = evt_data->length;
        if (copy_len > sizeof(msg.packet))
            copy_len = sizeof(msg.packet);

        memcpy(&msg.packet, evt_data->buffer, copy_len);
        msg.len = copy_len;

        xQueueOverwriteFromISR(slave_rx_queue, &msg, &high_task_wakeup);
    }

    return (pdTRUE == high_task_wakeup);
}

static void slave_process_task(void *arg)
{
    rx_msg_t msg;

    for (;;) {
        if (xQueueReceive(slave_rx_queue, &msg, portMAX_DELAY) == pdTRUE) {
            if (msg.len == 256) {
                display_update_wave(&msg.packet);
                ESP_LOGI(TAG, "Chart updated successfully.");
            } else {
                ESP_LOGW(TAG, "Dropped partial packet: %zu bytes", msg.len);
            }
        }
    }
}