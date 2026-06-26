#include "breath_ic_control.h"
#include "power_interface.h" // AT_VOLATILE_RAM_CODE
#include "mcpwm.h"           //

volatile breath_ic_control_t breath_ic_control;

void breath_ic_control_init(void)
{
    // gpio_set_direction(BREATH_IC_CONTROL_PIN, 0); // 输出

    // 默认不使能呼吸灯控制ic的呼吸效果
    // breath_ic_breathing_disable();

    // gpio_set_direction(BREATH_IC_CONTROL_PIN, 0);    // 输出模式
    // gpio_direction_output(BREATH_IC_CONTROL_PIN, 1); // 输出高电平（空闲电平）

    // 测试的时候使用
    // gpio_set_direction(IO_PORT_DM, 0);    // 输出模式
    // gpio_direction_output(IO_PORT_DM, 1); // 输出高电平（空闲电平）
}

// void breath_ic_breathing_enable(void)
// {
//     gpio_direction_output(BREATH_IC_CONTROL_PIN, BREATH_IC_BREATHING_ENABLE_PIN_LEVEL);
// }

// void breath_ic_breathing_disable(void)
// {
//     gpio_direction_output(BREATH_IC_CONTROL_PIN, BREATH_IC_BREATHING_DISABLE_PIN_LEVEL);
// }

/**
 * @brief 与呼吸灯控制ic通信的处理函数
 *
 *      先将 breath_ic_control.is_breathing_enable 赋值给 breath_ic_control.send_data_byte
 *      再将 breath_ic_control.is_send_cmd_enable 置一
 *
 */

AT_VOLATILE_RAM_CODE
void breath_ic_control_125us_isr(void)
{
    const static u8 dest_repeat_cnt = 3;   // 控制重复发送控制命令的次数
    static volatile u8 cur_repeat_cnt = 0; // 当前重复发送控制命令的计数

    // 控制单线发送数据的状态机
    static volatile one_wire_send_state_e one_wire_send_state = ONE_WIRE_SEND_STATE_IDLE;
    static volatile u8 one_wire_send_step = 0; // 控制各个子状态的步骤
    static volatile u8 one_wire_send_cnt = 0;  // 控制各个子状态的计时值

    struct pwm_platform_data pwm_arg;

    // static volatile u8 dir = 0;
    // if (0 == dir)
    // {
    //     gpio_direction_output(IO_PORT_DM, 0);
    //     dir = 1;
    // }
    // else
    // {
    //     gpio_direction_output(IO_PORT_DM, 1);
    //     dir = 0;
    // }

#if 1
    if (0 == breath_ic_control.is_send_cmd_enable)
    {
        // 不需要发送控制命令，直接返回
        cur_repeat_cnt = 0;
        one_wire_send_cnt = 0;
        one_wire_send_step = 0;
        one_wire_send_state = ONE_WIRE_SEND_STATE_IDLE;
        return;
    }

    switch (one_wire_send_state)
    {
    case ONE_WIRE_SEND_STATE_IDLE:
        // 取消复用功能:
        gpio_disable_fun_output_port(BREATH_IC_CONTROL_PIN);

        gpio_set_direction(BREATH_IC_CONTROL_PIN, 0);    // 输出模式
        gpio_direction_output(BREATH_IC_CONTROL_PIN, 1); // 输出高电平（空闲电平）

        one_wire_send_state = ONE_WIRE_SEND_STATE_SEND_START_SIG;
        one_wire_send_cnt = 0;
        // printf("sig start\n");
        break;

    case ONE_WIRE_SEND_STATE_SEND_START_SIG:
        // 起始信号，1ms 低电平 + 1ms 高电平
        one_wire_send_cnt++;
        if (one_wire_send_cnt <= (u8)(ONE_WIRE_START_SIG_LOW_LEVEL_DURATION / 125)) // 1000us / 125us定时调用周期
        {
            gpio_direction_output(BREATH_IC_CONTROL_PIN, 0);
        }
        else if (one_wire_send_cnt <
                 (u8)((ONE_WIRE_START_SIG_LOW_LEVEL_DURATION +
                       ONE_WIRE_START_SIG_HIGH_LEVEL_DURATION) /
                      125)) // 2000us / 125us定时调用周期
        {
            gpio_direction_output(BREATH_IC_CONTROL_PIN, 1);
        }
        else
        {
            one_wire_send_cnt = 0;
            one_wire_send_step = 0;
            one_wire_send_state = ONE_WIRE_SEND_STATE_SEND_DATA_SIG;
            // one_wire_send_state = ONE_WIRE_SEND_STATE_SEND_START_SIG; // USER_TO_DO 测试时使用
            // printf("sig data\n");
        }
        break;

    case ONE_WIRE_SEND_STATE_SEND_DATA_SIG:
        // 发送8个数据位，LSB （小端，低位优先）

        one_wire_send_cnt++;

        // 逻辑1 和 逻辑0 都是 500us 的低电平开始
        if (one_wire_send_cnt <= (500 / 125))
        {
            gpio_direction_output(BREATH_IC_CONTROL_PIN, 0);
        }
        else
        {
            // USER_TO_DO 不是LSB发送，需要修改：
            // if (breath_ic_control.send_data_byte & ((u8)1 << (7 - one_wire_send_step)))
            if ((breath_ic_control.send_data_byte >> one_wire_send_step) & ((u8)0x01))
            {
                // 发送逻辑1
                if (one_wire_send_cnt <
                    (u8)((ONE_WIRE_DATA_SIG_LOGIC_1_LOW_LEVEL_DURATION +
                          ONE_WIRE_DATA_SIG_LOGIC_1_HIGH_LEVEL_DURATION) /
                         125))
                {
                    gpio_direction_output(BREATH_IC_CONTROL_PIN, 1);
                }
                else
                {
                    // 刚发送完成 1 个数据位
                    one_wire_send_cnt = 0;
                    one_wire_send_step++;
                    // USER_TO_DO
                }
            }
            else
            {
                // 发送逻辑0
                if (one_wire_send_cnt <
                    (u8)((ONE_WIRE_DATA_SIG_LOGIC_0_LOW_LEVEL_DURATION +
                          ONE_WIRE_DATA_SIG_LOGIC_0_HIGH_LEVEL_DURATION) /
                         125))
                {
                    gpio_direction_output(BREATH_IC_CONTROL_PIN, 1);
                }
                else
                {
                    // 刚发送完成 1 个数据位
                    one_wire_send_cnt = 0;
                    one_wire_send_step++;
                    // USER_TO_DO
                }
            }
        }

        if (one_wire_send_step >= 8)
        {
            // 刚发送完成 8 个数据位
            one_wire_send_step = 0;
            one_wire_send_cnt = 0;
            one_wire_send_state = ONE_WIRE_SEND_STATE_SEND_STOP_SIG;

            // 测试时使用：
            // one_wire_send_cnt = 0;
            // one_wire_send_step = 0;
            // one_wire_send_state = ONE_WIRE_SEND_STATE_IDLE;
            // breath_ic_control.is_send_cmd_enable = 0;
            // printf("sig end\n");
        }
        break;

    case ONE_WIRE_SEND_STATE_SEND_STOP_SIG:
        // 结束信号，2ms 低电平 + 高电平（空闲）

        one_wire_send_cnt++;
        if (one_wire_send_cnt <= (ONE_WIRE_STIO_SIG_LOW_LEVEL_DURATION / 125))
        {
            gpio_direction_output(BREATH_IC_CONTROL_PIN, 0);
        }
        else if (one_wire_send_cnt <
                 (u8)((ONE_WIRE_STIO_SIG_LOW_LEVEL_DURATION +
                       ONE_WIRE_STOP_SIG_HIGH_LEVEL_DURATION) /
                      125))
        {
            gpio_direction_output(BREATH_IC_CONTROL_PIN, 1);
        }
        else
        {
            one_wire_send_cnt = 0;
            cur_repeat_cnt++;
            if (cur_repeat_cnt >= dest_repeat_cnt)
            {
                cur_repeat_cnt = 0;

                // 恢复驱动白灯的引脚为输出模式，并打开驱动白灯的PWM
                pwm_arg.pwm_aligned_mode = pwm_edge_aligned; // 边沿对齐
                pwm_arg.pwm_ch_num = pwm_ch3;                // 通道号
                pwm_arg.frequency = 1000;                    // 1KHz
                pwm_arg.duty = 0;                            // 上电输出0%占空比
                pwm_arg.h_pin = BREATH_IC_CONTROL_PIN;       // 任意引脚
                pwm_arg.l_pin = -1;                          // 任意引脚,不需要就填-1
                pwm_arg.complementary_en = 0;                // 两个引脚的波形, 0: 同步,  1: 互补，互补波形的占空比体现在H引脚上
                mcpwm_init(&pwm_arg);

                breath_ic_control.is_send_cmd_enable = 0;
            }

            one_wire_send_state = ONE_WIRE_SEND_STATE_IDLE;
        }
        break;
    }
#endif
}
