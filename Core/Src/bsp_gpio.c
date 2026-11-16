#include "bsp_gpio.h"

void GpioInit(GPIO_TypeDef *port, uint8_t pin, uint8_t mode, uint8_t otype, uint8_t speed, uint8_t pull)
{
    port->MODER &= ~(3U << (pin * 2));
    port->MODER |=  ((uint32_t)mode << (pin * 2));
    
    port->OTYPER &= ~(1U << pin);
    port->OTYPER |=  ((uint32_t)otype << pin);
    
    port->OSPEEDR &= ~(3U << (pin * 2));
    port->OSPEEDR |=  ((uint32_t)speed << (pin * 2));
    
    port->PUPDR &= ~(3U << (pin * 2));
    port->PUPDR |=  ((uint32_t)pull << (pin * 2));
}

void GpioWrite(GPIO_TypeDef *port, uint8_t pin, uint8_t value) {
    if (value == 0) port->BSRR = (1U << (pin + 16));
    else port->BSRR = (1U << pin);
}

uint8_t GpioRead(GPIO_TypeDef *port, uint8_t pin) {
    return (uint8_t)((port->IDR >> pin) & 1U);
}

void BSP_GPIO_Init(void)
{
    // Используем макросы CMSIS для включения тактирования
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOGEN;
    
    // Настройка светодиодов
    GpioInit(LED_GREEN_PORT, LED_GREEN_PIN, PIN_MODE_OUTPUT, PIN_OTYPE_PP, PIN_SPEED_HIGH, PIN_PU_NONE);
    GpioInit(LED_RED_PORT,   LED_RED_PIN,   PIN_MODE_OUTPUT, PIN_OTYPE_PP, PIN_SPEED_HIGH, PIN_PU_NONE);
    
    // Настройка кнопки
    GpioInit(BUTTON_PORT, BUTTON_PIN, PIN_MODE_INPUT, PIN_OTYPE_PP, PIN_SPEED_LOW, PIN_PU_NONE);

    // EXTI0 для кнопки
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0_Msk;
    SYSCFG->EXTICR[0] |=  (0U << SYSCFG_EXTICR1_EXTI0_Pos); // Соединяем с PA0
    EXTI->FTSR |= EXTI_FTSR_TR0;
    EXTI->RTSR |= EXTI_RTSR_TR0;
    EXTI->IMR  |= EXTI_IMR_IM0;
    
    NVIC_SetPriority(EXTI0_IRQn, BUTTON_IRQ_PRIO);
    NVIC_EnableIRQ(EXTI0_IRQn);
}