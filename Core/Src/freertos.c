#include "freertos.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "stm32f4xx_hal.h" 
#include "bsp_gyro.h"

#include "bsp_gpio.h"
#include "bsp_gyro.h"
#include "bsp_uart.h"

typedef struct { 
    int16_t x; 
    int16_t y; 
    int16_t z; 
} GyroData_t;

static uint8_t current_mode = MODE_1;
static uint32_t period_ms = DEFAULT_PERIOD_MS;

QueueHandle_t GyroQueueHandle = NULL;
QueueHandle_t UartRxQueueHandle = NULL;
SemaphoreHandle_t SharedDataMutexHandle = NULL;
TaskHandle_t ButtonTaskHandle = NULL;

// Прототипы
void GyroTask_Entry(void *argument);
void UartTxTask_Entry(void *argument);
void UartRxTask_Entry(void *argument);
void ButtonTask_Entry(void *argument);
void ValidationTask_Entry(void *argument);
void LedTask_Entry(void *argument);
void Gyro_Init(void);
void Gyro_ReadXYZ(int16_t *x, int16_t *y, int16_t *z);

extern void GpioWrite(GPIO_TypeDef *port, uint8_t pin, uint8_t value);
extern uint8_t GpioRead(GPIO_TypeDef *port, uint8_t pin);
extern UART_HandleTypeDef huart1;
extern SPI_HandleTypeDef hspi5;


void FREERTOS_Init(void) {
  // 1. Создание Мьютекса
  SharedDataMutexHandle = xSemaphoreCreateMutex();
  if (SharedDataMutexHandle == NULL) Error_Handler();

  // 2. Создание Очередей
  GyroQueueHandle = xQueueCreate(GYRO_QUEUE_LENGTH, sizeof(GyroData_t));
  if (GyroQueueHandle == NULL) Error_Handler();

  UartRxQueueHandle = xQueueCreate(UART_QUEUE_LENGTH, UART_BUFFER_SIZE);
  if (UartRxQueueHandle == NULL) Error_Handler();

  // 3. Создание Задач (xTaskCreate)
  // Аргументы: функция, имя, размер стека (в словах!), параметр, приоритет, хэндл
  xTaskCreate(GyroTask_Entry, "GyroTask", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(UartTxTask_Entry, "UartTx", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(UartRxTask_Entry, "UartRx", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(ValidationTask_Entry, "Valid", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(LedTask_Entry, "Led", 128, NULL, tskIDLE_PRIORITY + 1, NULL);
  
  // Для кнопки сохраняем хэндл (последний аргумент), чтобы отправлять уведомления
  xTaskCreate(ButtonTask_Entry, "Button", 128, NULL, tskIDLE_PRIORITY + 3, &ButtonTaskHandle);
}

// Реализация задач (Native FreeRTOS API) -------------------------

// Чтение данных с гироскопа
void GyroTask_Entry(void *argument) {
  GyroData_t gyro_data;

  Gyro_Calibrate(); 

  // vTaskDelay принимает время в тиках (pdMS_TO_TICKS переводит мс в тики)
  vTaskDelay(pdMS_TO_TICKS(STARTUP_DELAY_MS)); 

  for(;;) {
    Gyro_ReadXYZ(&gyro_data.x, &gyro_data.y, &gyro_data.z);
    
    // xQueueSend отправляет данные. Таймаут 0 (не ждать, если очередь полна)
    xQueueSend(GyroQueueHandle, &gyro_data, 0); 
    
    vTaskDelay(pdMS_TO_TICKS(GYRO_TASK_DELAY_MS)); 
  }
}

// Отправка сообщений по UART
void UartTxTask_Entry(void *argument) {
  GyroData_t rx_data; 
  char tx_buffer[64];
  TickType_t last_print = 0; // Используем TickType_t для хранения тиков

  for(;;) {
    // xQueueReceive ожидает данные (portMAX_DELAY = ждать вечно)
    if (xQueueReceive(GyroQueueHandle, &rx_data, portMAX_DELAY) == pdTRUE) { 
      
      // xSemaphoreTake берет мьютекс
      xSemaphoreTake(SharedDataMutexHandle, portMAX_DELAY); 
      uint8_t mode = current_mode; 
      uint32_t period = period_ms;
      xSemaphoreGive(SharedDataMutexHandle); 
      
      // xTaskGetTickCount возвращает текущие тики
      if (mode == MODE_2 && (xTaskGetTickCount() - last_print >= pdMS_TO_TICKS(period))) { 
        last_print = xTaskGetTickCount();
        int len = snprintf(tx_buffer, sizeof(tx_buffer), "X:%d Y:%d Z:%d\r\n", rx_data.x, rx_data.y, rx_data.z);
        HAL_UART_Transmit(&huart1, (uint8_t*)tx_buffer, len, 100);
      }
    }
  }
}

// Прием сообщений по UART
void UartRxTask_Entry(void *argument) {
  // Запускаем асинхронный прием один раз при старте задачи
  BSP_UART_StartReceive(); 
  for(;;) {
    vTaskDelay(pdMS_TO_TICKS(RX_TASK_DELAY_MS));
  }
}



// Контроль за состоянием кнопки
void ButtonTask_Entry(void *argument) {
  for(;;) {
    // ulTaskNotifyTake ожидает уведомление (Task Notification). 
    // pdTRUE означает, что счетчик уведомлений сбрасывается в 0 после пробуждения
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  
    
    vTaskDelay(pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS));  // Антидребезг
    
    if (GpioRead(BUTTON_PORT, BUTTON_PIN) == LOW) {
      xSemaphoreTake(SharedDataMutexHandle, portMAX_DELAY);
      current_mode = (current_mode == MODE_1) ? MODE_2 : MODE_1; 
      xSemaphoreGive(SharedDataMutexHandle);
    }
  }
}

// Валидация введенных пользователем данных через UART
void ValidationTask_Entry(void *argument) {
  char rx_str[UART_BUFFER_SIZE]; 
  char tx_buffer[TX_BUFFER_SIZE]; 

  for(;;) {
    // Ждем, пока прерывание не положит строку в очередь
    if (xQueueReceive(UartRxQueueHandle, rx_str, portMAX_DELAY) == pdTRUE) { 
      
      xSemaphoreTake(SharedDataMutexHandle, portMAX_DELAY);
      uint8_t mode = current_mode; 
      xSemaphoreGive(SharedDataMutexHandle);
      
      if (mode == MODE_1) {
        char *endptr;
        long value = strtol(rx_str, &endptr, 10);

        if (endptr == rx_str) {
            const char *err = "Error: not a number\r\n";
            HAL_UART_Transmit(&huart1, (uint8_t *)err, strlen(err), 100);
        }
        else if (*endptr != '\0') {
            const char *err = "Error: trailing chars\r\n";
            HAL_UART_Transmit(&huart1, (uint8_t *)err, strlen(err), 100);
        }
        else if (value < MIN_PERIOD_MS || value > MAX_PERIOD_MS) {
            int len = snprintf(tx_buffer, sizeof(tx_buffer), "Error: %d..%d ms\r\n", MIN_PERIOD_MS, MAX_PERIOD_MS);
            HAL_UART_Transmit(&huart1, (uint8_t *)tx_buffer, len, 100);
        }
        else {
            xSemaphoreTake(SharedDataMutexHandle, portMAX_DELAY);
            period_ms = (uint32_t)value;
            xSemaphoreGive(SharedDataMutexHandle);
            
            int len = snprintf(tx_buffer, sizeof(tx_buffer), "Period set: %ld ms\r\n", value);
            HAL_UART_Transmit(&huart1, (uint8_t *)tx_buffer, len, 100);
        }
      }
    }
  }
}

// Управление светодиодами
void LedTask_Entry(void *argument) {
  uint8_t last_mode = 0;
  
  for(;;) {
    xSemaphoreTake(SharedDataMutexHandle, portMAX_DELAY);
    uint8_t mode = current_mode; 
    xSemaphoreGive(SharedDataMutexHandle);

    if (mode != last_mode) {
      if (mode == MODE_1) {
        GpioWrite(LED_RED_PORT, LED_RED_PIN, HIGH);
        GpioWrite(LED_GREEN_PORT, LED_GREEN_PIN, LOW);
      } else {
        GpioWrite(LED_RED_PORT, LED_RED_PIN, LOW);
        GpioWrite(LED_GREEN_PORT, LED_GREEN_PIN, HIGH);
      }
      last_mode = mode;
    }
    
    vTaskDelay(pdMS_TO_TICKS(LED_TASK_DELAY_MS));
  }
}

/* --- Выделение памяти для задачи Idle --- */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t  xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = xIdleStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* --- Выделение памяти для задачи Timer (если включены таймеры) --- */
#if (configUSE_TIMERS == 1)
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t  xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
    *ppxTimerTaskStackBuffer = xTimerStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
#endif
