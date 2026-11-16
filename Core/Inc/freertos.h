#ifndef FREERTOS_H
#define FREERTOS_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#define UART_BUFFER_SIZE    32
#define BUTTON_PRESS_FLAG   0x01

#define MODE_1                  1
#define MODE_2                  2

#define MIN_PERIOD_MS           10
#define MAX_PERIOD_MS           10000
#define DEFAULT_PERIOD_MS       500

#define GYRO_TASK_DELAY_MS      10
#define LED_TASK_DELAY_MS       50
#define BUTTON_DEBOUNCE_MS      50
#define RX_TASK_DELAY_MS        1000
#define STARTUP_DELAY_MS        100
#define TX_BUFFER_SIZE          64

#define GYRO_QUEUE_LENGTH       10
#define UART_QUEUE_LENGTH       5
#define QUEUE_NO_WAIT           0

#define STACK_SIZE_NORMAL       (256 * 4)
#define STACK_SIZE_SMALL        (128 * 4)

#define BUTTON_PRESS_FLAG       0x01

// Хэндлы 
extern TaskHandle_t ButtonTaskHandle;
extern QueueHandle_t UartRxQueueHandle;
extern QueueHandle_t GyroQueueHandle;
extern SemaphoreHandle_t SharedDataMutexHandle;

void FREERTOS_Init(void);

#endif