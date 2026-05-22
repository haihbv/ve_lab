#include "stm32f10x.h"

void delay_ms(int time)
{
    while (time) {
        SysTick->CTRL = 5;
        SysTick->VAL = 0;
        SysTick->LOAD = 72000 - 1;
        while (!(SysTick->CTRL & (1 << 16)));
        --time;
    }
}

int main(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    while(1)
    {
        GPIOA->ODR ^= GPIO_Pin_5;
        delay_ms(500);
    }
}
