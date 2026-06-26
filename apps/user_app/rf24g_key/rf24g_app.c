
#include "system/includes.h"
#include "task.h"
#include "event.h"
#include "btstack/btstack_typedef.h"
#include "app_config.h"
#include "rf24g_app.h"
#include "WS2812FX.H"
#include "led_strip_sys.h"
#include "led_strand_effect.h"
#include "breath_ic_control.h"

#if (TCFG_RF24GKEY_ENABLE)

/* ==============================================================

                2.4g遥控初始化

    备注：该2.4G遥控的代码逻辑，是根据遥控而定的，可能不适宜其他遥控
=================================================================*/

// 天奕2.4G遥控
#define HEADER1 0X55
#define HEADER2 0XAA

rf24g_ins_t rf24g_ins; // 2.4g遥控数据包格式

#define SUPPORT_LONG (0)

struct RF24G_PARA is_rf24g_ = {

    .clink_delay_up = 0,
    .long_press_cnt = 0,
    .last_key_v = NO_KEY,
    .last_dynamic_code = 0,
    .rf24g_rx_flag = 0,
    .hold_pess_cnt = 0,
    .is_long_time = 1000,
    .rf24g_key_state = 0,
#if SUPPORT_LONG
    .is_click = 35, // 因没有长按功能，缩短抬起判断时间
#else
    .is_click = 10, // 因没有长按功能，缩短抬起判断时间
#endif
    .is_long = 1,
    ._sacn_t = 10,
};

static u8 Get_24G_KeyValue()
{

    if (is_rf24g_.rf24g_rx_flag)
    {
        is_rf24g_.rf24g_rx_flag = 0;

        if ((is_rf24g_.last_dynamic_code != rf24g_ins.dynamic_code) && rf24g_ins.remoter_id == 0x00)
        {

            is_rf24g_.clink_delay_up = 0;
            is_rf24g_.hold_pess_cnt = 0;
            is_rf24g_.last_key_v = rf24g_ins.key_v;
            is_rf24g_.last_dynamic_code = rf24g_ins.dynamic_code;
            return rf24g_ins.key_v; // 当前2.4G架构无用
        }
#if SUPPORT_LONG
        else
#else
        else if (rf24g_ins.remoter_id == 0x01)
#endif
        {
            is_rf24g_.clink_delay_up = 0;
            if (is_rf24g_.hold_pess_cnt >= is_rf24g_.is_long_time)
            {
                is_rf24g_.long_press_cnt++;
                is_rf24g_.hold_pess_cnt = 0;
                is_rf24g_.rf24g_key_state = KEY_EVENT_LONG;
                is_rf24g_.last_key_v = rf24g_ins.key_v;
                return rf24g_ins.key_v; // 当前2.4G架构无用
            }
        }
    }
    return NO_KEY;
}

struct key_driver_para rf24g_scan_para = {
    .scan_time = 2,         // 按键扫描频率, 单位: ms
    .last_key = NO_KEY,     // 上一次get_value按键值, 初始化为NO_KEY;
    .filter_time = 0,       // 按键消抖延时;
    .long_time = 200,       // 按键判定长按数量
    .hold_time = 200,       // 按键判定HOLD数量
    .click_delay_time = 20, // 按键被抬起后等待连击延时数量
    .key_type = KEY_DRIVER_TYPE_RF24GKEY,
    .get_value = Get_24G_KeyValue,
};

/* 长按计数器 */
void RF24G_Key_Long_Scan(void)
{

    if (rf24g_ins.remoter_id == 0x01)
    {

        is_rf24g_.hold_pess_cnt += (is_rf24g_._sacn_t + 2);
    }
    else
    {
        is_rf24g_.hold_pess_cnt = 0;
    }
}

// 底层按键扫描，由__resolve_adv_report()调用
void rf24g_scan(u8 *pBuf)
{
    rf24g_ins_t *p = (rf24g_ins_t *)pBuf;
    if (p->header1 == HEADER1 && p->header2 == HEADER2)
    {
        // printf_buf(pBuf,sizeof(rf24g_ins_t));
        memcpy((u8 *)&rf24g_ins, pBuf, sizeof(rf24g_ins_t));
        is_rf24g_.rf24g_rx_flag = 1;
    }
}

/* ==============================================================

                2.4g遥控按键功能


=================================================================*/

void RF24G_Key_Handle(void)
{
    u8 key_value = 0;
    if (is_rf24g_.clink_delay_up < 0xFF)
        is_rf24g_.clink_delay_up++;
    // printf("is_rf24g_.clink_delay_up=%d",is_rf24g_.clink_delay_up);
    if ((is_rf24g_.clink_delay_up > is_rf24g_.is_click) && (is_rf24g_.long_press_cnt < is_rf24g_.is_long))
    {
        key_value = is_rf24g_.last_key_v;
        if (key_value == NO_KEY)
            return;
        is_rf24g_.last_key_v = NO_KEY;
        is_rf24g_.clink_delay_up = 0;
        is_rf24g_.long_press_cnt = 0;
        printf(" ------------------------duanna key_value = %x\n", (u16)key_value);

        /*  code  */

        // 开/关
        if (key_value == RF24_ON_OFF || key_value == RFKEY_ON_OFF)
        {
            ls_ledStrip_switch();
        }

        if (get_on_off_state())
        {

            if (
                key_value != RFKEY_PAUSE &&
                key_value != RFKEY_ON_OFF &&
                key_value != RF24_ON_OFF &&
                key_value != RFKEY_LIGHT_PLUS &&
                key_value != RFKEY_LIGHT_SUB &&
                key_value != RFKEY_SPEED_PLUS &&

                key_value != RFKEY_SPEED_SUB &&
                key_value != RFKEY_2H &&
                key_value != RFKEY_1H)
            {
                set_ir_auto(IS_PAUSE);
            }

            // 速度/亮度 -
            if (key_value == RF24_SPEED_BRIGHT_SUB ||
                key_value == RFKEY_LIGHT_SUB)
            {

                ls_sub_bright();
                ls_add_speed();
                ls_sub_sensitive();
            }

            // 速度/亮度 +
            if (key_value == RF24_SPEED_BRIGHT_PLUS ||
                key_value == RFKEY_LIGHT_PLUS)
            {
                ls_add_bright();
                ls_sub_speed();
                ls_add_sensitive();
            }

            if (key_value == RFKEY_SPEED_PLUS)
            {
                ls_add_speed();
            }
            if (key_value == RFKEY_SPEED_SUB)
            {
                ls_sub_speed();
            }

            // 红色
            if (key_value == RF24_RED || key_value == RFKEY_R)
            {
                set_static_mode((RED >> 16) & 0xff, (RED >> 8) & 0xff, (RED >> 0) & 0xff);
            }
            // 绿色
            if (key_value == RF24_GREEN || key_value == RFKEY_G)
            {
                set_static_mode((GREEN >> 16) & 0xff, (GREEN >> 8) & 0xff, (GREEN >> 0) & 0xff);
            }
            // 蓝色
            if (key_value == RF24_BLUE || key_value == RFKEY_B)
            {
                set_static_mode((BLUE >> 16) & 0xff, (BLUE >> 8) & 0xff, (BLUE >> 0) & 0xff);
            }
            // 黄色
            if (key_value == RF24_YELLOW || key_value == RFKEY_YELLOW)
            {
                set_static_mode((YELLOW >> 16) & 0xff, (YELLOW >> 8) & 0xff, (YELLOW >> 0) & 0xff);
            }
            // 青色
            if (key_value == RFKEY_CYAN)
            {
                set_static_mode((CYAN >> 16) & 0xff, (CYAN >> 8) & 0xff, (CYAN >> 0) & 0xff);
            }
            // 紫色
            if (key_value == RFKEY_PURPLE)
            {
                set_static_mode((PURPLE >> 16) & 0xff, (PURPLE >> 8) & 0xff, (PURPLE >> 0) & 0xff);
            }
            // 天蓝色  85 250 255
            if (key_value == RF24_AZURE)
            {
                set_static_mode(85, 250, 255);
            }
            // 玫红色  25 50 218
            if (key_value == RF24_ROSE_RED)
            {
                set_static_mode(255, 50, 218);
            }
            // 纯白色   w b
            if (key_value == RF24_WHITE || key_value == RFKEY_W)
            {
                set_static_mode((WHITE >> 16) & 0xff, (WHITE >> 8) & 0xff, (WHITE >> 0) & 0xff);

                set_static_mode(
                    ((RED | GREEN | BLUE | WHITE) >> 16) & 0xff,
                    ((RED | GREEN | BLUE | WHITE) >> 8) & 0xff,
                    ((RED | GREEN | BLUE | WHITE) >> 0) & 0xff);
            }
            // 模暖白光
            if (key_value == RF24_WARM_WHITE)
            {
                // set_static_mode(255, 180, 20);
                set_static_mode(
                    ((RED | GREEN | BLUE | WHITE) >> 16) & 0xff,
                    ((RED | GREEN | BLUE | WHITE) >> 8) & 0xff,
                    ((RED | GREEN | BLUE | WHITE) >> 0) & 0xff);
            }
            // 模式集合区
            if (key_value == RF24_ALL_MODE)
            {

                ls_chose_mode_InAPP(1, 0, 0x00, 0x1c);
            }
            // 7色渐变
            if (key_value == RF24_SEVEN_COLOR_GRADUAL || key_value == RFKEY_7COL_GRADUAL)
            {
                ls_chose_mode_InAPP(1, 3, 0x0a, 0x0a);
            }
            // 七色呼吸
            if (key_value == RF24_SEVEN_COLOR_BREATHE)
            {

                ls_chose_mode_InAPP(1, 0, 0x0B, 0x11);
            }
            // 7色跳变
            if (key_value == RF24_SEVEN_COLOR_JUMP || key_value == RFKEY_7COL_JUMP)
            {

                ls_chose_mode_InAPP(1, 3, 0x08, 0x08);
            }
            // 白色呼吸
            if (key_value == RFKEY_W_BREATH)
            {
                ls_chose_mode_InAPP(1, 3, 0x12, 0x12);
            }

            // 声控1    4音乐律动减
            if (key_value == RF24_SOUND_ONE)
            {
                ls_sub_music_effect();
            }

            // 声控2   4音乐律动加
            if (key_value == RF24_SOUND_TWO)
            {
                ls_add_music_effect();
            }

            if (key_value == RFKEY_MUSIC1)
            {
                app_set_music_mode(1);
            }
            if (key_value == RFKEY_MUSIC2)
            {
                app_set_music_mode(2);
            }
            if (key_value == RFKEY_MUSIC3)
            {
                app_set_music_mode(3);
            }
            if (key_value == RFKEY_2H)
            {
                ls_set_countdown(2);
            }
            if (key_value == RFKEY_1H)
            {
                ls_set_countdown(1);
            }
            if (key_value == RFKEY_MODE_ADD)
            {
                app_set_speed(90);
                ls_chose_mode_InAPP(1, 1, 0x07, 0x15);
            }
            if (key_value == RFKEY_MODE_DEC)
            {
                app_set_speed(90);
                ls_chose_mode_InAPP(1, 2, 0x07, 0x15);
            }

            if (key_value == RFKEY_AUTO)
            {
                auto_Function_Switch();
            }

            if (key_value == RFKEY_PAUSE)
            {

                ls_pause_and_play();
            }

            //  ======================= 流星，电机相关  ===================

            if (key_value == RF24_STEMPMOTOR_SPEED)
            {
                // 电机转速调节 5挡   8s 13s 18s 21s 26s 35s
                // ls_set_motor_speed();

                breath_ic_control.is_send_cmd_enable = 0;

                if (fc_effect.is_breath_ic_breathing_enable)
                {
                    fc_effect.is_breath_ic_breathing_enable = 0;
                    breath_ic_control.is_breathing_enable = 0;
                }
                else
                {
                    fc_effect.is_breath_ic_breathing_enable = 1;
                    breath_ic_control.is_breathing_enable = 1;
                }

                breath_ic_control.send_data_byte = breath_ic_control.is_breathing_enable;
                breath_ic_control.is_send_cmd_enable = 1;
            }

            // 单流星  双流星
            if (key_value == RF24_ONE_TOW_METEOR)
            {

                ls_set_star_one_two();
            }
            // 声控流星1 2 两个律动
            if (key_value == RF24_METEOR_SOUND_ONE_TWO)
            {

                ls_set_star_music();
            }
            // 方向控制   流星方向
            if (key_value == RF24_DIRECTION)
            {
                ls_set_star_dir();
            }

            // 流星速度调节
            if (key_value == RF24_METEOR_SPEED)
            {
                ls_set_star_speed();
            }
            // 流星频率调节 5挡  4s 8s 12s 16s 20s
            if (key_value == RF24_METEOR_FREQUENCY)
            {
                ls_set_star_pro();
            }

            // 流星拖尾长度调节 3 5 7
            if (key_value == RF24_METEOR_TAIL)
            {

                ls_set_star_tail();
            }
        }
        save_user_data_area3();
    }
    else if (is_rf24g_.clink_delay_up > is_rf24g_.is_click)
    {
        is_rf24g_.clink_delay_up = 0;
        is_rf24g_.long_press_cnt = 0;
        is_rf24g_.last_key_v = NO_KEY;
    }

    if (is_rf24g_.long_press_cnt > is_rf24g_.is_long)
    {

        key_value = is_rf24g_.last_key_v;
        if (key_value == NO_KEY)
            return;
        is_rf24g_.last_key_v = NO_KEY;
        /*  因为执行这里，清了键值，而长按时，1s才获取一次键值 */
        printf(" ================================longkey_value = %d", key_value);

#if SUPPORT_LONG

        /* code */

#endif
    }
}

#endif
