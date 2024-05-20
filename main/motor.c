#include "main.h"

void motor_run(int ch)
{
    int flow=w.store[ch].pump_flow;
    int time=w.store[ch].pump_time;
    
    if(!flow || !time){
        return ;
    }
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, ch, 27+flow));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, ch));
    mod_timer(&w.motor.timer[ch], jiffies+time*HZ);
    
    w.store[ch].last_time=getSysTime();
    save();
    Printf("Start ch:%d duty:%d tm:%d\n",ch,flow*255/100,time);
}

void motor_timer_fun(unsigned long data)
{
    int ch=(int)data;
    Printf("Stop ch:%d\n",ch);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, ch, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, ch));
}

void motor_init(void)
{
    esp_efuse_write_field_bit(ESP_EFUSE_DIS_PAD_JTAG);
    
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_7_BIT,
        .freq_hz          = 40000,  // Set output frequency at 40 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel[ChannelMax] = {
        {
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = LEDC_CHANNEL_0,
            .timer_sel      = LEDC_TIMER_0,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = 4,
            .duty           = 0,
            .hpoint         = 0
        },
        {
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = LEDC_CHANNEL_1,
            .timer_sel      = LEDC_TIMER_0,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = 5,
            .duty           = 0,
            .hpoint         = 0
        },
        {
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = LEDC_CHANNEL_2,
            .timer_sel      = LEDC_TIMER_0,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = 6,
            .duty           = 0,
            .hpoint         = 0
        },
        {
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = LEDC_CHANNEL_3,
            .timer_sel      = LEDC_TIMER_0,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = 7,
            .duty           = 0,
            .hpoint         = 0
        }
    };

    for(int i=0;i<ChannelMax;i++){
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel[i]));
        setup_timer(&w.motor.timer[i], motor_timer_fun, i);
    }
}


