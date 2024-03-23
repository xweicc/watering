#include "main.h"


int key_is_down(key_trigger_t *key)
{
	int val=gpio_get_level(key->io);

	if(!val){
		if(key->state==keyStateNone){
			key->state=keyStateDowning;
			key->tm=jiffies;
		}else if(key->state==keyStateDowning && time_after(jiffies,key->tm+keyDelayTime)){
			key->state=keyStateDowned;
			key->tm=jiffies;
		}else if(key->state==keyStateDowned){
			if(time_after(jiffies,key->tm+keyLongTime)){
				key->state=keyStateDownedLong;
                return keyEventLong;
			}
		}
	}else{
		if(key->state==keyStateDowned){
            key->state=keyStateUping;
            key->tm=jiffies;
        }else if(key->state==keyStateUping && time_after(jiffies,key->tm+keyDelayTime)){
            key->state=keyStateNone;
            return keyEventShort;
        }else if(key->state==keyStateDownedLong && time_after(jiffies,key->tm+keyDelayTime)){
            key->state=keyStateNone;
        }
	}

    return keyEventNone;
}

void key_check_timer_fun(unsigned long data)
{
    for(int i=0;i<keyNumMax;i++){
        int event=key_is_down(&wvar.keys[i]);
        if(event==keyEventNone){
            continue;
        }else if(event==keyEventShort){
            if(i==keyNumSet){
                wvar.keys[i].fun(keySetShort);
            }
            break;
        }else if(event==keyEventLong){
            if(i==keyNumSet){
                wvar.keys[i].fun(keySetLong);
            }
            break;
        }
    };

    mod_timer(&wvar.key_timer, jiffies+keyCheckTime);
}


int key_register_fun(int key, key_fun_t fun)
{
    if(key>=keyNumMax){
        return -1;
    }
    wvar.keys[key].fun=fun;

    return 0;
}

int key_init(void)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL<<keyIoSet);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = true;
    gpio_config(&io_conf);

    wvar.keys[0].io=keyIoSet;

    setup_timer(&wvar.key_timer, key_check_timer_fun, 0);
    mod_timer(&wvar.key_timer, jiffies+keyCheckTime);
    
    return 0;
}

