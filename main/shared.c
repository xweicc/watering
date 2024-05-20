#include "shared.h"

__u32 getSysTime(void)
{
	time_t now;
	time(&now);
	return (__u32)now;
}

char *getTimeStr(void)
{
	time_t now;
    struct tm tm;
	static char str[32]={0};
	
    time(&now);
    localtime_r(&now, &tm);

	memset(&str, 0, sizeof(str));
	Snprintf(str, sizeof(str), "%d-%d-%d %d:%d:%d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	return str;
}

int split_string(char *str, char c, char **argv, int max)
{
	int i=0,j=0;
	char *cp = str;
	int is=0;

	for(j=0;j<max;j++)
		argv[j] = NULL;
	
	while(cp && *cp && i < max)
	{
		argv[i++] = cp;
		is=0;
		while(*cp && *cp != c)
		{
			cp++;
		}
		if(*cp)
		{
			is = 1;
			*cp++=0;
		}
	}
	if((is == 1) && (i < max))
		argv[i++] = cp;
	return i;
}

// 函数用于将十六进制数字字符转换为整数值
int hex2int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

// URL 解码函数
void urlDecode(char *src, char *dest) {
    char *p = src;
    char code[3] = {0};
    while (*p) {
        if (*p == '%') {
            if (p[1] && p[2]) {
                code[0] = p[1];
                code[1] = p[2];
                *dest++ = (hex2int(code[0]) << 4) | hex2int(code[1]);
                p += 3;
            }
        } else if (*p == '+') {
            *dest++ = ' ';
            p++;
        } else {
            *dest++ = *p++;
        }
    }
    *dest = '\0';
}

