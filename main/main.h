#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_log.h>
#include <time.h>
#include <driver/i2c.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/spi_master.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include <lwip/sys.h>
#include <esp_wifi.h>
#include <esp_http_server.h>
#include <esp_tls_crypto.h>
#include <sys/param.h>
#include <esp_netif.h>
#include <esp_http_server.h>
#include <soc/soc_caps.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <hal/gpio_types.h>
#include <driver/gpio.h>

#include "llist.h"
#include "timer.h"
#include "shared.h"
#include "motor.h"
#include "key.h"
#include "wlan.h"
#include "webfile.h"

#define Printf(format,args...) do{\
        printf("[%s:%d]:"format,__ASSERT_FUNC,__LINE__,##args);\
    }while(0)

#define PrintOnce(format,args...) do{\
	static bool __print_once=0;\
	if(!__print_once){\
		__print_once=true;\
		Printf(format, ##args);\
	}\
	}while(0)

typedef struct {
    __u8 pump_flow;
    __u8 pump_time;
    __u8 plan_mode;
    __u8 plan_humi;
        
    __u8 plan_week;
    __u8 plan_interval;
    __u8 interval_unit;
    __u8 reserve1;
    
    __u32 plan_time;
    __u32 last_time;
    __u32 reserve[1];
}store_t;

enum{
    LedStateConnecting,
    LedStateWaitNtp,
    LedStateOK,
};

typedef struct {
    motor_var_t motor;
    key_trigger_t keys[keyNumMax];
    struct timer_list key_timer;
    struct timer_list led_timer;
    struct list_head gotIpHead;
    EventGroupHandle_t wifiEventGroup;
	esp_netif_t *netif;
	char hostname[32];
	char ssid[32];
	char pwd[64];
	__u8 bssid[6];
    int led_state;
    httpd_handle_t server;
    store_t store;
    struct timer_list plan_timer;
} watering_var_t;

extern watering_var_t wvar;

void http_init(void);
void plan_init(void);
void save(void);
int sensor_init(void);
int get_humi(void);

#endif

