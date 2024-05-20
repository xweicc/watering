#ifndef __WLAN_H__
#define __WLAN_H__

#define NTP_SYNC_CYCLE (60*60*HZ)	//1小时

struct gotIpHookInfo{
	struct list_head list;
	__u8 first;	//第一次获取IP才调用
	void (*fun)(__u32 ip);
};

void wlanInit(void);
void wlanInitSta(void);

#endif

