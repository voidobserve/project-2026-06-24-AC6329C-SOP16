#include "breath_ic_control.h"

void breath_ic_control_init(void)
{
    gpio_set_direction(BREATH_IC_CONTROL_PIN, 0); // 输出

    // 默认不使能呼吸灯控制ic的呼吸效果
    breath_ic_breathing_disable();
}

void breath_ic_breathing_enable(void)
{
    gpio_direction_output(BREATH_IC_CONTROL_PIN, BREATH_IC_BREATHING_ENABLE_PIN_LEVEL);
}

void breath_ic_breathing_disable(void)
{
    gpio_direction_output(BREATH_IC_CONTROL_PIN, BREATH_IC_BREATHING_DISABLE_PIN_LEVEL);
}