#ifndef __MOTOR_H__
#define __MOTOR_H__

typedef struct {
    struct timer_list timer[ChannelMax];
} motor_var_t;

void motor_init(void);
void motor_run();

#endif
