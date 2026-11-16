#include "main.h"
#include "stm32f4xx_it.h"
#include "freertos.h"
#include "FreeRTOS.h"
#include "task.h"

extern TIM_HandleTypeDef htim6;
extern UART_HandleTypeDef huart1;

void HardFault_Handler(void)
{
  while (1) {}
}

void TIM6_DAC_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim6);
}

void EXTI0_IRQHandler(void)
{
  if (EXTI->PR & EXTI_PR_PR0) {
      EXTI->PR = EXTI_PR_PR0; // Очищаем флаг прерывания
      
      // Переменная для проверки, нужно ли переключить задачу
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      // Отправляем уведомление (увеличивает счетчик уведомлений задачи на 1)
      vTaskNotifyGiveFromISR(ButtonTaskHandle, &xHigherPriorityTaskWoken);
      // Переключаем задачу, если ее приоритет выше текущей
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart1);
}
