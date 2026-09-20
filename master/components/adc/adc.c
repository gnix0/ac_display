#include "adc.h"

static const char *TAG = "ADC Calibration Routine";

bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten,
                          adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle    = NULL;
    esp_err_t ret               = ESP_FAIL;
    bool calibrated             = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id    = unit,
            .chan       = channel,
            .atten      = atten,
            .bitwidth   = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);

        if (ESP_OK == ret)
            calibrated = true;
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id  = unit,
            .atten    = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);

        if (ret == ESP_OK)
            calibrated = true;
    }
#endif

    *out_handle = handle;
    if (ESP_OK == ret)
        ESP_LOGI(TAG, "Calibration successful");
    else if (ESP_ERR_NOT_SUPPORTED == ret || !calibrated)
        ESP_LOGW(TAG, "eFuse not burnt, skipping software calibration");
    else
        ESP_LOGE(TAG, "Invalid argument or no memory for calibration routine");

    return calibrated;
}

void adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "unregistering %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}