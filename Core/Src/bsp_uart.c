#include "bsp_uart.h"
#include "freertos.h" 
#include "FreeRTOS.h"
#include "queue.h"

static uint8_t rx_byte;
static char rx_buffer[32];
static uint8_t rx_index = 0;

void BSP_UART_StartReceive(void) { 
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1); 
}

UART_HandleTypeDef huart1;

void BSP_UART_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin       = (1U << UART_TX_PIN) | (1U << UART_RX_PIN);
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = UART_AF;
    HAL_GPIO_Init(UART_TX_PORT, &GPIO_InitStruct);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = UART_BAUDRATE;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);

    HAL_NVIC_SetPriority(USART1_IRQn, UART_IRQ_PRIO, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        char c = (char)rx_byte;
        if (c == '\n' || c == '\r') { // Проверка на конец строки
            if (rx_index > 0) {
                rx_buffer[rx_index] = '\0';
                BaseType_t xHigherPriorityTaskWoken = pdFALSE; // переменная флаг
                // Отправляем в очередь (функция для прерываний)
                xQueueSendFromISR(UartRxQueueHandle, rx_buffer, &xHigherPriorityTaskWoken); 
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // если флаг pdTRUE, то сразу переключаемся
                rx_index = 0; 
            }
        } else {
            if (rx_index < 31) rx_buffer[rx_index++] = c;
            else rx_index = 0; 
        }
        HAL_UART_Receive_IT(huart, &rx_byte, 1); // Перезапускаем прием
    }
}