#include "main.h"

adc_cali_handle_t adc1_cali_handle = NULL;
adc_oneshot_unit_handle_t adc1_handle;

#define VoltMax 1000
#define VoltMin 100

int get_humi(void)
{
    int adc_raw[10]={0};
    int max_idx=0,min_idx=0;
    int sum=0, adc;
    int voltage;

    for(int c=0;c<10;c++){
        esp_err_t ret=adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw[c]);
        if(ret){
            Printf("adc_oneshot_read error\n");
            return -1;
        }
    }
    for(int c=0;c<10;c++){
        if(adc_raw[c]>adc_raw[max_idx]){
            max_idx=c;
        }else if(adc_raw[c]<adc_raw[min_idx]){
            min_idx=c;
        }
    }
    for(int c=0;c<10;c++){
        if (c!=max_idx && c!=min_idx){
            sum+=adc_raw[c];
        }
    }

    if(max_idx==min_idx){
        adc=sum/9;
    }else{
        adc=sum/8;
    }

    if(adc_cali_raw_to_voltage(adc1_cali_handle, adc, &voltage)){
        Printf("adc_cali_raw_to_voltage error\n");
        return -1;
    }

    Printf("voltage:%d\n",voltage);
    if(voltage>VoltMax){
        voltage=VoltMax;
    }

    if(voltage<VoltMin){
        voltage=VoltMin;
    }

    return 1000-((voltage-VoltMin)*1000/(VoltMax-VoltMin));
}

static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        Printf("calibration scheme version is %s", "Curve Fitting\n");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        Printf("calibration scheme version is %s", "Line Fitting\n");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        Printf("Calibration Success\n");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        Printf("eFuse not burnt, skip software calibration\n");
        esp_restart();
    } else {
        Printf("Invalid arg or no memory\n");
        esp_restart();
    }

    return calibrated;
}

int sensor_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));

    adc_calibration_init(ADC_UNIT_1, ADC_ATTEN_DB_11, &adc1_cali_handle);

    return 0;
}


