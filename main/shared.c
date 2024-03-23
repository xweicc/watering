#include "shared.h"

__u32 getSysTime(void)
{
	time_t now;
	time(&now);
	return (__u32)now;
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

