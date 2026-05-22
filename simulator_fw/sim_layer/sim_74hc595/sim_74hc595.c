/**
 * @file    sim_74hc595.c
 * @brief   74HC595 Simulation Module implementation.
 */

#include "sim_74hc595.h"
#include <string.h>

/* Private macros ----------------------------------------------------------*/

#define PORT          GPIOB
#define DS_PIN        GPIO_Pin_10
#define SHCP_PIN      GPIO_Pin_11
#define STCP_PIN      GPIO_Pin_12

#define SHCP_EXTI     EXTI_Line11
#define STCP_EXTI     EXTI_Line12
#define IRQN          EXTI15_10_IRQn

/* Static objects ----------------------------------------------------------*/

static sim_74hc595_t g_74hc595_sim;

/* Public functions --------------------------------------------------------*/

void sim_74hc595_init(void)
{
    memset(&g_74hc595_sim, 0, sizeof(sim_74hc595_t));

    /* 1. Logic reset */
    hc595_reset(&g_74hc595_sim.logic);

    /* 2. Driver setup */
    gpio_setup(&g_74hc595_sim.ds,   PORT, DS_PIN,   GPIO_Mode_IN_FLOATING, GPIO_Speed_50MHz);
    gpio_setup(&g_74hc595_sim.shcp, PORT, SHCP_PIN, GPIO_Mode_IN_FLOATING, GPIO_Speed_50MHz);
    gpio_setup(&g_74hc595_sim.stcp, PORT, STCP_PIN, GPIO_Mode_IN_FLOATING, GPIO_Speed_50MHz);

    gpio_enable(&g_74hc595_sim.ds);
    gpio_enable(&g_74hc595_sim.shcp);
    gpio_enable(&g_74hc595_sim.stcp);

    /* 3. EXTI Config for Clocks */
    EXTI_InitTypeDef exti_init;
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource11);
    exti_init.EXTI_Line    = SHCP_EXTI;
    exti_init.EXTI_Mode    = EXTI_Mode_Interrupt;
    exti_init.EXTI_Trigger = EXTI_Trigger_Rising;
    exti_init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_init);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource12);
    exti_init.EXTI_Line    = STCP_EXTI;
    EXTI_Init(&exti_init);

    NVIC_EnableIRQ(IRQN);

    /* 4. RTOS Sync */
    g_74hc595_sim.mutex = xSemaphoreCreateMutex();
}

/* IRQ Handlers ------------------------------------------------------------*/

void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(SHCP_EXTI) != RESET)
    {
        uint8_t bit = 0;
        gpio_read(&g_74hc595_sim.ds, &bit);
        
        /* Inside ISR, we don't take mutex but rely on atomic operations 
           if necessary. Here it's simple enough. */
        hc595_shift(&g_74hc595_sim.logic, bit);
        
        EXTI_ClearITPendingBit(SHCP_EXTI);
    }
    
    if (EXTI_GetITStatus(STCP_EXTI) != RESET)
    {
        hc595_latch(&g_74hc595_sim.logic);
        EXTI_ClearITPendingBit(STCP_EXTI);
    }
}
