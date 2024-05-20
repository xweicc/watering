#ifndef __SHARED_H__
#define __SHARED_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

#include <sys/param.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <esp_sntp.h>

#include "llist.h"
#include "timer.h"

typedef unsigned long long __u64;
typedef unsigned int __u32;
typedef unsigned short __u16;
typedef unsigned char __u8;

#define T_Min 60
#define T_Hour T_Min*60
#define T_Day T_Hour*24

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define Snprintf(buff,size,fmt,args...) ({int __ret__; __ret__=snprintf((buff),(size),fmt,##args);__ret__<(size)?__ret__:(size)-1;})

__u32 getSysTime(void);
char *getTimeStr(void);
int split_string(char *str, char c, char **argv, int max);
void urlDecode(char *src, char *dest);

static inline char *ipstr(__u32 ip)
{
	static char str[20]={0};
	unsigned char *ip_dot=(unsigned char *)&ip;

	sprintf(str,"%d.%d.%d.%d",ip_dot[0],ip_dot[1],ip_dot[2],ip_dot[3]);
	return str;
}

#endif
