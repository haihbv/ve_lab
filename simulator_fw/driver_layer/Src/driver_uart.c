/**
 * @file    driver_uart.c
 * @brief   UART driver implementation.
 */

#include "driver_uart.h"
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

/* Private macros ----------------------------------------------------------*/

#define RX_BUF_SIZE 512
#define RX_IDX_MASK (RX_BUF_SIZE - 1)

/* Private types -----------------------------------------------------------*/

typedef struct
{
    uint8_t  buf[RX_BUF_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} uart_rb_t;

/* Private function prototypes ---------------------------------------------*/

static void prv_rb_put(uart_rb_t *rb, uint8_t c);
static bool prv_rb_get(uart_rb_t *rb, uint8_t *c);
static bool prv_rb_available(uart_rb_t *rb);

/* Static objects ----------------------------------------------------------*/

static uart_rb_t g_uart1_rb = {0};
static uart_rb_t g_uart2_rb = {0};
static uart_rb_t g_uart3_rb = {0};

/* Public objects ----------------------------------------------------------*/

uart_t g_uart1 = { .instance = USART1, .rx_rb = &g_uart1_rb, .baud_rate = 115200 };
uart_t g_uart2 = { .instance = USART2, .rx_rb = &g_uart2_rb, .baud_rate = 115200 };
uart_t g_uart3 = { .instance = USART3, .rx_rb = &g_uart3_rb, .baud_rate = 115200 };

/* Public functions --------------------------------------------------------*/

driver_status_t uart_setup(uart_t *dev, USART_TypeDef *instance, uint32_t baud_rate)
{
    if (dev == NULL || instance == NULL)
    {
        return DRIVER_ERR_INVALID_ARG;
    }

    dev->instance  = instance;
    dev->baud_rate = baud_rate;

    if (instance == USART1)      dev->rx_rb = &g_uart1_rb;
    else if (instance == USART2) dev->rx_rb = &g_uart2_rb;
    else if (instance == USART3) dev->rx_rb = &g_uart3_rb;
    else return DRIVER_ERR_INVALID_ARG;

    return DRIVER_OK;
}

driver_status_t uart_enable(uart_t *dev)
{
    if (dev == NULL) return DRIVER_ERR_INVALID_ARG;

    GPIO_InitTypeDef  gpio_init  = {0};
    USART_InitTypeDef uart_init  = {0};
    NVIC_InitTypeDef  nvic_init  = {0};

    if (dev->instance == USART1)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
        /* TX: PA9, RX: PA10 */
        gpio_init.GPIO_Pin   = GPIO_Pin_9;
        gpio_init.GPIO_Mode  = GPIO_Mode_AF_PP;
        gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOA, &gpio_init);
        gpio_init.GPIO_Pin  = GPIO_Pin_10;
        gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOA, &gpio_init);
        nvic_init.NVIC_IRQChannel = USART1_IRQn;
    }
    else if (dev->instance == USART2)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
        /* TX: PA2, RX: PA3 */
        gpio_init.GPIO_Pin   = GPIO_Pin_2;
        gpio_init.GPIO_Mode  = GPIO_Mode_AF_PP;
        gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOA, &gpio_init);
        gpio_init.GPIO_Pin  = GPIO_Pin_3;
        gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOA, &gpio_init);
        nvic_init.NVIC_IRQChannel = USART2_IRQn;
    }
    else if (dev->instance == USART3)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
        /* TX: PB10, RX: PB11 */
        gpio_init.GPIO_Pin   = GPIO_Pin_10;
        gpio_init.GPIO_Mode  = GPIO_Mode_AF_PP;
        gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOB, &gpio_init);
        gpio_init.GPIO_Pin  = GPIO_Pin_11;
        gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOB, &gpio_init);
        nvic_init.NVIC_IRQChannel = USART3_IRQn;
    }

    uart_init.USART_BaudRate            = dev->baud_rate;
    uart_init.USART_WordLength          = USART_WordLength_8b;
    uart_init.USART_StopBits            = USART_StopBits_1;
    uart_init.USART_Parity              = USART_Parity_No;
    uart_init.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    uart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    USART_Init(dev->instance, &uart_init);
    USART_Cmd(dev->instance, ENABLE);
    USART_ITConfig(dev->instance, USART_IT_RXNE, ENABLE);

    nvic_init.NVIC_IRQChannelCmd                = ENABLE;
    nvic_init.NVIC_IRQChannelSubPriority        = 0;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_Init(&nvic_init);

    return DRIVER_OK;
}

driver_status_t uart_disable(uart_t *dev)
{
    if (dev == NULL) return DRIVER_ERR_INVALID_ARG;
    USART_Cmd(dev->instance, DISABLE);
    return DRIVER_OK;
}

driver_status_t uart_write_char(uart_t *dev, char c)
{
    if (dev == NULL) return DRIVER_ERR_INVALID_ARG;
    while (USART_GetFlagStatus(dev->instance, USART_FLAG_TXE) == RESET);
    USART_SendData(dev->instance, (uint16_t)c);
    return DRIVER_OK;
}

driver_status_t uart_write_string(uart_t *dev, const char *str)
{
    if (dev == NULL || str == NULL) return DRIVER_ERR_INVALID_ARG;
    while (*str)
    {
        uart_write_char(dev, *str++);
    }
    return DRIVER_OK;
}

bool uart_read_available(uart_t *dev)
{
    if (dev == NULL || dev->rx_rb == NULL) return false;
    return prv_rb_available((uart_rb_t *)dev->rx_rb);
}

driver_status_t uart_read_char(uart_t *dev, char *c)
{
    if (dev == NULL || c == NULL || dev->rx_rb == NULL) return DRIVER_ERR_INVALID_ARG;
    if (prv_rb_get((uart_rb_t *)dev->rx_rb, (uint8_t *)c))
    {
        return DRIVER_OK;
    }
    return DRIVER_ERR_EMPTY;
}

void uart_handle_irq(uart_t *dev)
{
    if (dev == NULL || dev->rx_rb == NULL) return;
    if (USART_GetITStatus(dev->instance, USART_IT_RXNE) != RESET)
    {
        uint8_t c = (uint8_t)(USART_ReceiveData(dev->instance) & 0xFF);
        prv_rb_put((uart_rb_t *)dev->rx_rb, c);
        USART_ClearITPendingBit(dev->instance, USART_IT_RXNE);
    }
}

/* IRQ Handlers ------------------------------------------------------------*/

void USART1_IRQHandler(void)
{
    uart_handle_irq(&g_uart1);
}

void USART2_IRQHandler(void)
{
    uart_handle_irq(&g_uart2);
}

void USART3_IRQHandler(void)
{
    uart_handle_irq(&g_uart3);
}

/* Private functions -------------------------------------------------------*/

static void prv_rb_put(uart_rb_t *rb, uint8_t c)
{
    uint16_t next_head = (rb->head + 1) & RX_IDX_MASK;
    if (next_head == rb->tail)
    {
        rb->tail = (rb->tail + 1) & RX_IDX_MASK;
    }
    rb->buf[rb->head] = c;
    rb->head          = next_head;
}

static bool prv_rb_get(uart_rb_t *rb, uint8_t *c)
{
    if (rb->head == rb->tail) return false;
    *c       = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1) & RX_IDX_MASK;
    return true;
}

static bool prv_rb_available(uart_rb_t *rb)
{
    return (rb->head != rb->tail);
}

/* Retarget printf ---------------------------------------------------------*/

static uart_t *g_debug_uart = NULL;

int fputc(int ch, FILE *f)
{
    (void)f;
    if (g_debug_uart)
    {
        if (ch == '\n') uart_write_char(g_debug_uart, '\r');
        uart_write_char(g_debug_uart, (char)ch);
    }
    return ch;
}

void uart_set_debug_output(uart_t *dev)
{
    g_debug_uart = dev;
}
