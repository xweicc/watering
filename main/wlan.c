#include "main.h"

static void wifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	if(event_base==WIFI_EVENT){
		switch(event_id){
			case WIFI_EVENT_STA_START:
				xEventGroupSetBits(wvar.wifiEventGroup, BIT0);
				break;
			case WIFI_EVENT_STA_DISCONNECTED:
				esp_wifi_connect();
				break;
			default:
				break;
		}
	}else if(event_base==IP_EVENT){
		switch(event_id){
			case IP_EVENT_STA_GOT_IP:{
				static __u8 first=1;
				ip_event_got_ip_t *event=(void*)event_data;
				Printf("Got ip:%s\n",ipstr(event->ip_info.ip.addr));
				struct gotIpHookInfo *info;
				list_for_each_entry(info, &wvar.gotIpHead, list){
					if(!info->first || first){
						info->fun(event->ip_info.ip.addr);
					}
				}
				first=0;
				break;
			}
			default:
				break;
		}
	}
}

void wifiStartConnect(void)
{
	wifi_config_t cfg;
	
	bzero(&cfg, sizeof(wifi_config_t));
	strncpy((char *)cfg.sta.ssid,WIFI_SSID,sizeof(cfg.sta.ssid));
	strncpy((char *)cfg.sta.password,WIFI_PWD,sizeof(cfg.sta.password));
	cfg.sta.bssid_set=false;
	esp_netif_set_hostname(wvar.netif, "Watering");

    ESP_ERROR_CHECK(esp_wifi_disconnect());
	Printf("Connect ssid:%s pwd:%s\n",cfg.sta.ssid,cfg.sta.password);
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, NULL) );
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));
	ESP_ERROR_CHECK(esp_wifi_connect());
}

void gotIpRegister(struct gotIpHookInfo *info)
{
	list_add(&info->list, &wvar.gotIpHead);
}

void ntpTask(void *data)
{
    int init=0;
    
	Printf("Start ntp task...\n");
    wvar.led_state=LedStateWaitNtp;
	while(1){
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
		esp_sntp_setservername(0, "cn.ntp.org.cn");
		esp_sntp_setservername(1, "ntp1.aliyun.com");
		esp_sntp_init();
		while (getSysTime()<1700000000){
			vTaskDelay(HZ);
		}
		esp_sntp_stop();

        if(!init){
            init=1;
            Printf("Timestamp:%u jiffies:%lu\n",getSysTime(),jiffies);
            mod_timer(&wvar.plan_timer, jiffies+10*HZ);
            wvar.led_state=LedStateOK;
        }

		vTaskDelay(NTP_SYNC_CYCLE);
	}
}

void ntpStart(__u32 ip)
{
	xTaskCreate(ntpTask, "ntpTask", 2048, NULL, ESP_TASK_MAIN_PRIO, NULL);
}

void ntpInit(void)
{
	static struct gotIpHookInfo hook;
	hook.first=1;
	hook.fun=ntpStart;
	gotIpRegister(&hook);
}

void wlanInit(void)
{
    wvar.wifiEventGroup = xEventGroupCreate();
    wvar.netif=esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, NULL));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

	xEventGroupWaitBits(wvar.wifiEventGroup, BIT0, false, true, portMAX_DELAY);

    INIT_LIST_HEAD(&wvar.gotIpHead);
	ntpInit();
    wifiStartConnect();
}

