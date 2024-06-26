#ifndef __KEY_H__
#define __KEY_H__

#define keyDelayTime 50	    //消抖时间
#define keyCheckTime 10     //检测间隔
#define keyLongTime 500    //长按时间
#define keyLongerTime 3000  //更长按时间

enum{
    keyIoSet=9,
};

enum{
    keyNumSet,
    keyNumMax,
};

enum{
	keyStateNone,
	keyStateDowning,
	keyStateDowned,
	keyStateDownedLong,
	keyStateDownedLonger,
	keyStateUping,
};

enum{
    keySetShort,
    keySetLong,
    keySetLonger,
};

enum{
    keyEventNone,
    keyEventShort,  //短按
    keyEventLong,   //长按
    keyEventLonger, //更长按
};

typedef void (*key_fun_t)(int);

typedef struct {
	unsigned long tm;	//上一次检测时间
	uint8_t io;			//IO口
	uint8_t state;			//状态
    key_fun_t fun;
} key_trigger_t;


int key_init(void);
int key_register_fun(int key, key_fun_t fun);

#endif
