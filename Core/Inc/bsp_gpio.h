#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include "stm32f4xx_hal.h"

// Режимы пинов
#define PIN_MODE_INPUT   0b00
#define PIN_MODE_OUTPUT  0b01
#define PIN_MODE_AF      0b10
#define PIN_MODE_ANALOG  0b11

// Типы выхода
#define PIN_OTYPE_PP     0b0
#define PIN_OTYPE_OD     0b1

// Скорость
#define PIN_SPEED_LOW    0b00
#define PIN_SPEED_MED    0b01
#define PIN_SPEED_HIGH   0b10
#define PIN_SPEED_VHIGH  0b11

// Подтяжка
#define PIN_PU_NONE      0b00
#define PIN_PU_UP        0b01
#define PIN_PU_DOWN      0b10

// Пины светодиодов и кнопки
#define LED_GREEN_PORT   GPIOG
#define LED_GREEN_PIN    13
#define LED_RED_PORT     GPIOG
#define LED_RED_PIN      14
#define BUTTON_PORT      GPIOA
#define BUTTON_PIN       0

#define BUTTON_IRQ_PRIO  5

#define LOW   0
#define HIGH  1

void BSP_GPIO_Init(void);
void GpioInit(GPIO_TypeDef *port, uint8_t pin, uint8_t mode, uint8_t otype, uint8_t speed, uint8_t pull);
void GpioWrite(GPIO_TypeDef *port, uint8_t pin, uint8_t value);
uint8_t GpioRead(GPIO_TypeDef *port, uint8_t pin);

#endif