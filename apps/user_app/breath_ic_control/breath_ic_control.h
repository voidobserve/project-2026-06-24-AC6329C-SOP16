#ifndef __BREATH_IC_CONTROL_H__
#define __BREATH_IC_CONTROL_H__

#include "typedef.h"

#include "gpio.h"

typedef struct
{
    u8 is_breathing_enable; // 是否使能呼吸灯控制ic的呼吸效果
    u8 is_send_cmd_enable;  // 是否要发送控制命令
    u8 send_data_byte;      // 待发送的数据字节
} breath_ic_control_t;

// 与呼吸灯控制ic的通信引脚
// #define BREATH_IC_CONTROL_PIN (IO_PORT_DM) // 测试的时候使用
#define BREATH_IC_CONTROL_PIN (IO_PORTB_06)
// 呼吸灯控制ic使能呼吸时，通信引脚需要输出的电平：
#define BREATH_IC_BREATHING_ENABLE_PIN_LEVEL (0)
// 呼吸灯控制ic非呼吸时，通信引脚需要输出的电平：
#define BREATH_IC_BREATHING_DISABLE_PIN_LEVEL (1)

enum
{
    ONE_WIRE_SEND_STATE_IDLE = 0,
    ONE_WIRE_SEND_STATE_SEND_START_SIG,
    ONE_WIRE_SEND_STATE_SEND_DATA_SIG,
    ONE_WIRE_SEND_STATE_SEND_STOP_SIG,
};
typedef u8 one_wire_send_state_e;

// 单线发送时，起始信号的高电平时长，单位：us
#define ONE_WIRE_START_SIG_HIGH_LEVEL_DURATION ((u16)1000)
// 单线发送时，起始信号的低电平时长，单位：us
#define ONE_WIRE_START_SIG_LOW_LEVEL_DURATION ((u16)1000)

// 单线发送时，一个二进制数据位，"逻辑1" 信号的高电平时长，单位：us
#define ONE_WIRE_DATA_SIG_LOGIC_1_HIGH_LEVEL_DURATION ((u16)1000)
// 单线发送时，一个二进制数据位，"逻辑1" 信号的低电平时长，单位：us
#define ONE_WIRE_DATA_SIG_LOGIC_1_LOW_LEVEL_DURATION ((u16)500)

// 单线发送时，一个二进制数据位，"逻辑0" 信号的高电平时长，单位：us
#define ONE_WIRE_DATA_SIG_LOGIC_0_HIGH_LEVEL_DURATION ((u16)500)
// 单线发送时，一个二进制数据位，"逻辑0" 信号的低电平时长，单位：us
#define ONE_WIRE_DATA_SIG_LOGIC_0_LOW_LEVEL_DURATION ((u16)500)

// 单线发送时，结束信号的高电平时长，单位：us
#define ONE_WIRE_STOP_SIG_HIGH_LEVEL_DURATION ((u16)2000)
// 单线发送时，结束信号的低电平时长，单位：us
#define ONE_WIRE_STIO_SIG_LOW_LEVEL_DURATION ((u16)2000)

extern volatile breath_ic_control_t breath_ic_control;

void breath_ic_control_init(void);
// void breath_ic_breathing_enable(void);
// void breath_ic_breathing_disable(void);

void breath_ic_control_125us_isr(void);

#endif
