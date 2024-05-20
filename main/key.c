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
		}else if(key->state==keyStateDownedLong){
			if(time_after(jiffies,key->tm+keyLongerTime)){
				key->state=keyStateDownedLonger;
                return keyEventLonger;
			}
		}
	}else{
		if(key->state==keyStateDowned){
            key->state=keyStateUping;
            key->tm=jiffies;
        }else if(key->state==keyStateUping && time_after(jiffies,key->tm+keyDelayTime)){
            key->state=keyStateNone;
            return keyEventShort;
        }else if((key->state==keyStateDownedLong || key->state==keyStateDownedLonger) && time_after(jiffies,key->tm+keyDelayTime)){
            key->state=keyStateNone;
        }
	}

    return keyEventNone;
}

void key_check_timer_fun(unsigned long data)
{
    for(int i=0;i<keyNumMax;i++){
        int event=key_is_down(&w.keys[i]);
        if(event==keyEventNone){
            continue;
        }else if(event==keyEventShort){
            if(i==keyNumSet){
                w.keys[i].fun(keySetShort);
            }
            break;
        }else if(event==keyEventLong){
            if(i==keyNumSet){
                w.keys[i].fun(keySetLong);
            }
            break;
        }else if(event==keyEventLonger){
            if(i==keyNumSet){
                w.keys[i].fun(keySetLonger);
            }
            break;
        }
    };

    mod_timer(&w.key_timer, jiffies+keyCheckTime);
}


int key_register_fun(int key, key_fun_t fun)
{
    if(key>=keyNumMax){
        return -1;
    }
    w.keys[key].fun=fun;

    return 0;
}

int key_init(void)
{
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL<<keyIoSet);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = true;
    gpio_config(&io_conf);

    w.keys[0].io=keyIoSet;

    setup_timer(&w.key_timer, key_check_timer_fun, 0);
    mod_timer(&w.key_timer, jiffies+keyCheckTime);
    
    return 0;
}

