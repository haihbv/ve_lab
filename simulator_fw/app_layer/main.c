#include "FreeRTOS.h"
#include "app_cfg.h"
#include "stm32f10x.h"
#include "task.h"

#if (ENABLE_DS1307 == 1)
#include "sim_ds1307.h"
#endif

#if (ENABLE_DHT11 == 1)
#include "sim_dht11.h"
#endif

#if (ENABLE_MQ2 == 1)
#include "sim_mq2.h"
#endif

#if (ENABLE_HC595 == 1)
#include "sim_74hc595.h"
#endif

#if (ENABLE_DHT22 == 1)
#include "sim_dht22.h"
#endif

void prvSetupHardware(void);
void prvSetupLabs(void);

int main(void)
{
    prvSetupHardware();
    prvSetupLabs();
    vTaskStartScheduler();

    for (;;);
}

void prvSetupHardware(void)
{
    SystemInit();
    SystemCoreClockUpdate();
    NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);
}

void prvSetupLabs(void)
{
#if (ENABLE_DS1307 == 1)
    sim_ds1307_init();
#endif

#if (ENABLE_DHT11 == 1)
    sim_dht11_init();
#endif

#if (ENABLE_MQ2 == 1)
    sim_mq2_init();
#endif

#if (ENABLE_HC595 == 1)
    sim_74hc595_init();
#endif

#if (ENABLE_DHT22 == 1)
    sim_dht22_init();
#endif

#if (ENABLE_BLINK_LED == 1)
    sim_blink_led_init();
#endif
}
