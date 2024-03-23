#include "main.h"

//自动
void plan_mode_1(void)
{
    if(getSysTime()-wvar.store.last_time<1*T_Hour){
        return ;
    }
    
    int humi=get_humi()/10;
    if(humi<0){
        Printf("get_humi error\n");
        return ;
    }

    Printf("humi:%d plan_humi:%d\n",humi,wvar.store.plan_humi);
    if(humi<=wvar.store.plan_humi){
        motor_run();
    }
}

//定时
void plan_mode_2(void)
{
    time_t now;
    struct tm t;
    __u8 plan_week=wvar.store.plan_week;
    __u8 plan_time=wvar.store.plan_time;

    if(getSysTime()-wvar.store.last_time<1*T_Hour){
        return ;
    }
    
    now=time(NULL);
    localtime_r(&now, &t);

    for(int i=0;i<7;i++){
        if(i==t.tm_wday && plan_week&(1<<i)){
            for(int i=0;i<24;i++){
                if(i==t.tm_hour && plan_time&(1<<i)){
                    motor_run();
                }
            }
        }
    }
}

//间隔
void plan_mode_3(void)
{
    int interval=wvar.store.plan_interval;
    if(wvar.store.interval_unit){
        interval*=24;
    }
    
    if(getSysTime()-wvar.store.last_time<interval*T_Hour){
        return ;
    }

    motor_run();
}

static void plan_timer_fun(unsigned long data)
{
    Printf("plan_mode:%d\n",wvar.store.plan_mode);
    switch(wvar.store.plan_mode){
        case 1:
            plan_mode_1();
            break;
        case 2:
            plan_mode_2();
            break;
        case 3:
            plan_mode_3();
            break;
        default:
            break;
    }
    
    mod_timer(&wvar.plan_timer, jiffies+10*HZ);
}

void plan_init(void)
{
    setup_timer(&wvar.plan_timer, plan_timer_fun, 0);
}

