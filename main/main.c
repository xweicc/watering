
#include "main.h"

watering_var_t wvar;

store_t defaults={
    .pump_flow=50,
    .pump_time=3,
    .plan_mode=0,
    .plan_humi=10,
    .plan_week=1,
    .plan_time=1,
    .plan_interval=1,
    .interval_unit=1,
};

void key_handler_fun(int event)
{
    motor_run();
}

void led_timer_fun(unsigned long data)
{
    static int val=0;
    val=!val;
    gpio_set_level(2,val);

    if(wvar.led_state==LedStateConnecting){
        mod_timer(&wvar.led_timer, jiffies+100);
    }else if(wvar.led_state==LedStateWaitNtp){
        mod_timer(&wvar.led_timer, jiffies+500);
    }else if(wvar.led_state==LedStateOK){
        if(val){
            mod_timer(&wvar.led_timer, jiffies+100);
        }else{
            mod_timer(&wvar.led_timer, jiffies+2000);
        }
    }
}

void led_init(void)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL<<2);
    io_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io_conf);
    gpio_set_level(2,1);
    setup_timer(&wvar.led_timer, led_timer_fun, 0);
    mod_timer(&wvar.led_timer, jiffies+200);
}


int nvram_set_data(char *name, void *data, int len)
{
	int ret=-1;
	nvs_handle_t nvram;
	esp_err_t err=nvs_open("nvram", NVS_READWRITE, &nvram);
	if(err!=ESP_OK){
		Printf("nvs_open:%s(%X)\n", esp_err_to_name(err),err);
		return -1;
	}

	err=nvs_set_blob(nvram, name, data, len);
	if(err!=ESP_OK){
		Printf("nvs_set_blob %s:%s(%X)\n", name,esp_err_to_name(err),err);
		goto out;
	}
	
	err=nvs_commit(nvram);
	if(err!=ESP_OK){
		Printf("nvs_commit:%s(%X)\n", esp_err_to_name(err),err);
		goto out;
	}
	ret=0;
	
out:
	nvs_close(nvram);
	return ret;
}

int nvram_get_data(char *name, void *data, int *size)
{
	int ret=-1;
	nvs_handle_t nvram;
	esp_err_t err=nvs_open("nvram", NVS_READONLY, &nvram);
	if(err!=ESP_OK){
		Printf("nvs_open:%s(%X)\n", esp_err_to_name(err),err);
		return -1;
	}
	err=nvs_get_blob(nvram, name, data, (size_t *)size);
	if(err!=ESP_OK){
		Printf("nvs_get_blob %s:%s(%X)\n", name,esp_err_to_name(err),err);
		goto out;
	}
	ret=0;
	
out:
	nvs_close(nvram);
	return ret;
}

void save(void)
{
    nvram_set_data("store", &wvar.store, sizeof(wvar.store));
}


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    struct timeval tv={0};
	tv.tv_sec=0;
	settimeofday(&tv, NULL);
    init_timers_cpu();

    int size=sizeof(wvar.store);
    if(nvram_get_data("store", &wvar.store, &size)!=ESP_OK
        || size!=sizeof(wvar.store)){
        Printf("set defaults\n");
        wvar.store=defaults;
    }

    led_init();
    key_init();
    motor_init();
    sensor_init();
    http_init();
    wlanInit();
    plan_init();

    key_register_fun(keyNumSet, key_handler_fun);

    Printf("init ok\n");
    while(1){
        run_timers();
        vTaskDelay(1);
    }
}

