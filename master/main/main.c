#include <stdio.h>
#include <stdint.h>

#include "adc.h"
#include "master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "I2C Master Device";

#define ADC_UNIT    ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_0
#define ADC_ATTEN   ADC_ATTEN_DB_12

typedef struct {
    adc_oneshot_unit_handle_t   adc_handle;
    adc_cali_handle_t           cali_handle;
    bool                        do_calibration;
    i2c_master_dev_handle_t     i2c_dev_handle;
} master_task_args_t;

static void adc_and_i2c_task(void *arg);

void app_main(void)
{
    static master_task_args_t task_args = {0};

    adc_oneshot_unit_init_cfg_t init_config = { .unit_id = ADC_UNIT };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &task_args.adc_handle));

    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth   = ADC_BITWIDTH_DEFAULT,
        .atten      = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(task_args.adc_handle, ADC_CHANNEL, &channel_config));
    task_args.do_calibration = adc_calibration_init(ADC_UNIT, ADC_CHANNEL, ADC_ATTEN, &task_args.cali_handle);

    ESP_ERROR_CHECK(app_master_init(&task_args.i2c_dev_handle));

    xTaskCreate(adc_and_i2c_task, "adc_and_i2c_task", 4096, &task_args, 5, NULL);
}

static void adc_and_i2c_task(void *arg)
{
    master_task_args_t *args    = (master_task_args_t *)arg;
    display_packet_t packet     = {0};
    memset(&packet, 0, sizeof(packet));

    int raw_val         = 0;
    int voltage         = 0;
    uint8_t sample_idx  = 0;

    for (;;) {
        ESP_ERROR_CHECK(adc_oneshot_read(args->adc_handle, ADC_CHANNEL, &raw_val));

        if (args->do_calibration)
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(args->cali_handle, raw_val, &voltage));
        else
            voltage = raw_val; 

        packet.voltages[sample_idx++] = (uint32_t)voltage;

        if (sample_idx >= 64) {
            ESP_LOGI(TAG, "Transmitting 256 bytes to Slave...");
            
            esp_err_t err = i2c_master_transmit(args->i2c_dev_handle, 
                                                (const uint8_t *)&packet, 
                                                sizeof(packet), 
                                                pdMS_TO_TICKS(100));
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "I2C Transmit failed: %s", esp_err_to_name(err));
            }

            sample_idx = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}