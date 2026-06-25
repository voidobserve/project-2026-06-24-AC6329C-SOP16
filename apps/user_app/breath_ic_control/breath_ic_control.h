#ifndef __BREATH_IC_CONTROL_H__
#define __BREATH_IC_CONTROL_H__

#include "typedef.h"

#include "gpio.h"

// 与呼吸灯控制ic的通信引脚
#define BREATH_IC_CONTROL_PIN (IO_PORT_DM)
// 呼吸灯控制ic使能呼吸时，通信引脚需要输出的电平：
#define BREATH_IC_BREATHING_ENABLE_PIN_LEVEL (0)
// 呼吸灯控制ic非呼吸时，通信引脚需要输出的电平：
#define BREATH_IC_BREATHING_DISABLE_PIN_LEVEL (1)


void breath_ic_control_init(void);
void breath_ic_breathing_enable(void);
void breath_ic_breathing_disable(void);


#endif
