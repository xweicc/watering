#include "main.h"

static void wifiEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	if(event_base==WIFI_EVENT){
		switch(event_id){
			case WIFI_EVENT_STA_START:
				xEventGroupSetBits(w.wifiEventGroup, BIT0);
				break;
			case WIFI_EVENT_STA_DISCONNECTED:
				esp_wifi_connect();
				break;
            case WIFI_EVENT_AP_STACONNECTED:{
                wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
                Printf("station "MACSTR" join, AID=%d\n", MAC2STR(event->mac), event->aid);
                break;
            }
            case WIFI_EVENT_AP_STADISCONNECTED:{
                wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
                Printf("station "MACSTR" leave, AID=%d\n", MAC2STR(event->mac), event->aid);
                break;
            }
			default:
				break;
		}
	}
    else if(event_base==IP_EVENT){
		switch(event_id){
			case IP_EVENT_STA_GOT_IP:{
				static __u8 first=1;
				ip_event_got_ip_t *event=(void*)event_data;
                w.ip=event->ip_info.ip.addr;
				Printf("Got ip:%s\n",ipstr(w.ip));
                w.net_state=2;
				struct gotIpHookInfo *info;
				list_for_each_entry(info, &w.gotIpHead, list){
					if(!info->first || first){
						info->fun(w.ip);
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
	strncpy((char *)cfg.sta.ssid,w.wlan.ssid,sizeof(cfg.sta.ssid));
	strncpy((char *)cfg.sta.password,w.wlan.pwd,sizeof(cfg.sta.password));
	cfg.sta.bssid_set=false;
	esp_netif_set_hostname(w.netif, "Watering");

    ESP_ERROR_CHECK(esp_wifi_disconnect());
	Printf("Connect ssid:%s pwd:%s\n",cfg.sta.ssid,cfg.sta.password);
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, NULL) );
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));
	ESP_ERROR_CHECK(esp_wifi_connect());
    w.net_state=1;
}

void gotIpRegister(struct gotIpHookInfo *info)
{
	list_add(&info->list, &w.gotIpHead);
}

void ntpTask(void *data)
{
    int init=0;
    
	Printf("Start ntp task...\n");
	while(1){
        __u32 tm=getSysTime();
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
		esp_sntp_setservername(0, "cn.ntp.org.cn");
		esp_sntp_setservername(1, "ntp1.aliyun.com");
		esp_sntp_init();
        int count=0;
		while (getSysTime()<1700000000){
			vTaskDelay(HZ);
            if(++count>60){
                Printf("ntp failed\n");
                esp_restart();
            }
		}
		w.sysTimeRun=getSysTime()-(tm-w.sysTimeRun);
		esp_sntp_stop();

        if(!init){
            init=1;
            Printf("Timestamp:%u jiffies:%lu\n",getSysTime(),jiffies);
            mod_timer(&w.plan_timer, jiffies+10*HZ);
        }
        w.net_state=3;
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


void wlanInitSta(void)
{
    if(w.netif){
        esp_wifi_disconnect();
        esp_wifi_stop();
    }
    
    w.wifiEventGroup = xEventGroupCreate();
    w.netif=esp_netif_create_default_wifi_sta();
    
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

	xEventGroupWaitBits(w.wifiEventGroup, BIT0, false, true, portMAX_DELAY);

    INIT_LIST_HEAD(&w.gotIpHead);
	ntpInit();
    wifiStartConnect();
}


void wlanInitAp(void)
{
    w.netif=esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "Watering",
            .ssid_len = strlen("Watering"),
            .password = "12345678",
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = true,
            },
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    Printf("finished\n");
}


void wlanInit(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, NULL));

    if(!strlen(w.wlan.ssid)){
        wlanInitAp();
    }else{
        wlanInitSta();
    }
}

