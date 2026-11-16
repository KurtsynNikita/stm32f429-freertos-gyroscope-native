#ifndef BSP_UART_H
#define BSP_UART_H

#include "stm32f4xx_hal.h"

// Параметры UART
#define UART_BAUDRATE    115200
#define UART_TX_PORT     GPIOA
#define UART_TX_PIN      9
#define UART_RX_PORT     GPIOA
#define UART_RX_PIN      10
#define UART_AF          GPIO_AF7_USART1
#define UART_IRQ_PRIO    5
#define UART_BUFFER_SIZE 32 

extern UART_HandleTypeDef huart1;

void BSP_UART_Init(void);
void BSP_UART_StartReceive(void);

#endif