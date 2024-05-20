
#include "main.h"

watering_t w;

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

void reset(void)
{
    oled_clear();
    oled_show_text(0, 0, "reseting");
    vTaskDelay(HZ);
    nvram_unset("store");
    nvram_unset("wlan");
    esp_restart();
}

void key_handler_fun(int event)
{
    switch(event){
        case keySetShort:
            w.sleep=0;
            mod_timer(&w.sleep_timer, jiffies+60*HZ);
            break;
        case keySetLonger:
            reset();
            break;
        default:
            break;
    }
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

int nvram_unset(char *name)
{
	int ret=-1;
	nvs_handle_t nvram;
	esp_err_t err=nvs_open("nvram", NVS_READWRITE, &nvram);
	if(err!=ERR_OK){
		Printf("nvs_open:%s(%X)\n",esp_err_to_name(err),err);
		return -1;
	}
	err=nvs_erase_key(nvram, name);
	if(err!=ERR_OK){
		Printf("nvs_erase_key %s:%s(%X)\n",name,esp_err_to_name(err),err);
		goto out;
	}
	
	err=nvs_commit(nvram);
	if(err!=ERR_OK){
		Printf("nvs_commit:%s(%X)\n",esp_err_to_name(err),err);
		goto out;
	}
	ret=0;
	
out:
	nvs_close(nvram);
	return ret;
}

void save(void)
{
    nvram_set_data("store", &w.store, sizeof(w.store));
}

__u32 getRunTime(void)
{
	return getSysTime()-w.sysTimeRun;
}


void sysTimeInit(void)
{
	struct timeval tv={0};
	tv.tv_sec=0;
	settimeofday(&tv, NULL);

    setenv("TZ", "CST-8", 1);
	tzset();
}

void view_timer_fun(unsigned long data)
{
    w.view();
    mod_timer(&w.view_timer, jiffies+100);
}

void sleep_timer_fun(unsigned long data)
{
    w.sleep=1;
}

void view_show_init(void)
{
    oled_clear();
    if(w.sleep){
        return ;
    }
    switch(w.net_state){
        case 1:{
            oled_show_text(0, 0, "connecting");
            oled_show_string(64, 0, "WIFI...", FontSize_8x16);
        }break;
        case 2:{
            oled_show_text(0, 0, "connect");
            oled_show_string(32, 0, "WIFI", FontSize_8x16);
            oled_show_text(64, 0, "success");
            oled_show_text(0, 16, "ntping");
        }break;
        case 3:{
            oled_show_text(0, 0, "network");
            oled_show_text(32, 0, "success");
            oled_show_text(0, 16, "conf_addr");
            oled_show_string(0, 32, ipstr(w.ip), FontSize_8x16);
        }break;
        default:{
            oled_show_string(0, 0, "WIFI: Watering", FontSize_8x16);
            oled_show_text(0, 16, "pwd");
            oled_show_string(32, 16, ": 12345678", FontSize_8x16);
            oled_show_text(0, 32, "conf_addr");
            oled_show_string(0, 48, "192.168.4.1", FontSize_8x16);
        }break;
    }
}


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    sysTimeInit();
    init_timers_cpu();

    int size=sizeof(w.store);
    if(nvram_get_data("store", &w.store, &size)!=ESP_OK || size!=sizeof(w.store)){
        Printf("set defaults\n");
        for(int i=0;i<ChannelMax;i++){
            w.store[i]=defaults;
        }
    }
    size=sizeof(w.wlan);
    nvram_get_data("wlan", &w.wlan, &size);

    motor_init();
    oled_init();
    key_init();
    sensor_init();
    http_init();
    wlanInit();
    plan_init();

    key_register_fun(keyNumSet, key_handler_fun);
    xTaskCreate(oled_task, "oled_task", 2048, NULL, 3, NULL);
    setup_timer(&w.view_timer, view_timer_fun, 0);
    mod_timer(&w.view_timer, jiffies+100);
    w.view=view_show_init;
    setup_timer(&w.sleep_timer, sleep_timer_fun, 0);
    mod_timer(&w.sleep_timer, jiffies+60*HZ);

    Printf("init ok\n");
    while(1){
        run_timers();
        vTaskDelay(1);
    }
}

