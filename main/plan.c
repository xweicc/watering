#include "main.h"

//自动
void plan_mode_1(int ch)
{
    if(getSysTime()-w.store[ch].last_time<1*T_Hour){
        return ;
    }
    
    int humi=get_humi(ch)/10;
    if(humi<0){
        Printf("get_humi error\n");
        return ;
    }

    Printf("humi:%d plan_humi:%d\n",humi,w.store[ch].plan_humi);
    if(humi<=w.store[ch].plan_humi){
        motor_run(ch);
    }
}

//定时
void plan_mode_2(int ch)
{
    time_t now;
    struct tm t;
    __u8 plan_week=w.store[ch].plan_week;
    __u8 plan_time=w.store[ch].plan_time;

    if(getSysTime()-w.store[ch].last_time<1*T_Hour){
        return ;
    }
    
    now=time(NULL);
    localtime_r(&now, &t);

    for(int i=0;i<7;i++){
        if(i==t.tm_wday && plan_week&(1<<i)){
            for(int i=0;i<24;i++){
                if(i==t.tm_hour && plan_time&(1<<i)){
                    motor_run(ch);
                }
            }
        }
    }
}

//间隔
void plan_mode_3(int ch)
{
    int interval=w.store[ch].plan_interval;
    if(w.store[ch].interval_unit){
        interval*=24;
    }
    
    if(getSysTime()-w.store[ch].last_time<interval*T_Hour){
        return ;
    }

    motor_run();
}

static void plan_timer_fun(unsigned long data)
{
    for(int ch=0;ch<ChannelMax;ch++){
        Printf("ch:%d plan_mode:%d\n",ch,w.store[ch].plan_mode);
        switch(w.store[ch].plan_mode){
            case 1:
                plan_mode_1(ch);
                break;
            case 2:
                plan_mode_2(ch);
                break;
            case 3:
                plan_mode_3(ch);
                break;
            default:
                break;
        }
        
        mod_timer(&w.plan_timer, jiffies+10*HZ);
    }
}

void plan_init(void)
{
    setup_timer(&w.plan_timer, plan_timer_fun, 0);
}

