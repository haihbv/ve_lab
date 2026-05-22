#include "FreeRTOS.h"
#include "driver_uart.h"
#include "projdefs.h"
#include "stdio.h"
#include "stm32f10x.h"
#include "task.h"

void vHelloTask(void* pvParameters)
{
    while (1) {
        printf("Hello, World!\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main()
{
    SystemInit();
    uart1.init(115200);

    BaseType_t res = xTaskCreate(vHelloTask, "HelloTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    if (res != pdPASS) {
        printf("Failed to create task\n");
        while (1);
    }

    vTaskStartScheduler();
    for (;;);
}
