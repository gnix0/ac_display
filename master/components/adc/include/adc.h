#ifndef ADC_H_
#define ADC_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_log.h"

bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten,
                          adc_cali_handle_t *out_handle);
void adc_calibration_deinit(adc_cali_handle_t handle);

#endif  // adc.h