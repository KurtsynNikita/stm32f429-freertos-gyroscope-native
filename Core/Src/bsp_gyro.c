#include "bsp_gyro.h"
#include "bsp_uart.h" 
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h" 

static void SPI5_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN | RCC_AHB1ENR_GPIOCEN;
    
    // SCK, MISO, MOSI
    SPI_PORT->MODER &= ~((3U << (SPI_SCK_PIN*2)) | (3U << (SPI_MISO_PIN*2)) | (3U << (SPI_MOSI_PIN*2)));
    SPI_PORT->MODER |=  ((2U << (SPI_SCK_PIN*2)) | (2U << (SPI_MISO_PIN*2)) | (2U << (SPI_MOSI_PIN*2)));
    SPI_PORT->AFR[0] &= ~(0xFU << (SPI_SCK_PIN*4));
    SPI_PORT->AFR[0] |=  (5U  << (SPI_SCK_PIN*4));
    SPI_PORT->AFR[1] &= ~((0xFU << ((SPI_MISO_PIN-8)*4)) | (0xFU << ((SPI_MOSI_PIN-8)*4)));
    SPI_PORT->AFR[1] |=  ((5U << ((SPI_MISO_PIN-8)*4)) | (5U << ((SPI_MOSI_PIN-8)*4)));

    // CS
    GYRO_CS_PORT->MODER &= ~(3U << (GYRO_CS_PIN*2));
    GYRO_CS_PORT->MODER |=  (1U << (GYRO_CS_PIN*2));
    GYRO_CS_PORT->OTYPER &= ~(1U << GYRO_CS_PIN);
    GYRO_CS_PORT->OSPEEDR |= (2U << (GYRO_CS_PIN*2));
    GYRO_CS_PORT->BSRR = (1U << GYRO_CS_PIN); // CS HIGH

    RCC->APB2ENR |= RCC_APB2ENR_SPI5EN;
    GYRO_SPI->CR1 = 0;
    GYRO_SPI->CR2 = 0;
    GYRO_SPI->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | GYRO_SPI_BR_DIV_64 | SPI_CR1_SPE;
}

static uint8_t SPI5_Transfer(uint8_t data)
{
    while (!(GYRO_SPI->SR & SPI_SR_TXE)) { }
    *(volatile uint8_t *)&GYRO_SPI->DR = data;
    while (!(GYRO_SPI->SR & SPI_SR_RXNE)) { }
    return *(volatile uint8_t *)&GYRO_SPI->DR;
}

static void Gyro_Write(uint8_t reg, uint8_t value)
{
    GYRO_CS_PORT->BSRR = (1U << (GYRO_CS_PIN + 16)); // CS LOW
    SPI5_Transfer(reg & ~SPI_READ_CMD);              // Адрес + W
    SPI5_Transfer(value);                            // Значение
    GYRO_CS_PORT->BSRR = (1U << GYRO_CS_PIN);        // CS HIGH
}

static uint8_t Gyro_Read(uint8_t reg)
{
    uint8_t value;
    GYRO_CS_PORT->BSRR = (1U << (GYRO_CS_PIN + 16)); // CS LOW
    SPI5_Transfer(reg | SPI_READ_CMD);               // Адрес + R
    value = SPI5_Transfer(0xFF);                     // Пустой байт
    GYRO_CS_PORT->BSRR = (1U << GYRO_CS_PIN);        // CS HIGH
    return value;
}

void BSP_GYRO_Init(void)
{
    char dbg_msg[64];
    SPI5_Init(); 
    
    uint8_t who = Gyro_Read(GYRO_WHO_AM_I_REG);
    
    if (who == GYRO_ID_I3G4250D || who == GYRO_ID_L3GD20) {
        int len = snprintf(dbg_msg, sizeof(dbg_msg), "Gyro: OK (0x%02X)\r\n", who);
        HAL_UART_Transmit(&huart1, (uint8_t*)dbg_msg, len, 100);
        
        Gyro_Write(GYRO_CTRL_REG1, GYRO_CFG_NORMAL_100HZ);
        Gyro_Write(GYRO_CTRL_REG4, GYRO_CFG_BDU_245DPS);
    } else {
        int len = snprintf(dbg_msg, sizeof(dbg_msg), "Gyro: ERROR (0x%02X)!\r\n", who);
        HAL_UART_Transmit(&huart1, (uint8_t*)dbg_msg, len, 100);
    }
}

void Gyro_ReadXYZ(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t buf[6];
    GYRO_CS_PORT->BSRR = (1U << (GYRO_CS_PIN + 16)); // CS LOW
    SPI5_Transfer(GYRO_OUT_X_L_REG | SPI_READ_CMD | SPI_MULTI_BYTE_CMD);
    for (int i = 0; i < 6; i++) {
        buf[i] = SPI5_Transfer(0xFF);
    }
    GYRO_CS_PORT->BSRR = (1U << GYRO_CS_PIN);        // CS HIGH

    *x = (int16_t)((buf[1] << 8) | buf[0]);
    *y = (int16_t)((buf[3] << 8) | buf[2]);
    *z = (int16_t)((buf[5] << 8) | buf[4]);
}

void Gyro_Calibrate(void)
{
    int32_t sum_x = 0, sum_y = 0, sum_z = 0;

    for (int i = 0; i < GYRO_CALIB_SAMPLES; i++)
    {
        int16_t x, y, z;
        Gyro_ReadXYZ(&x, &y, &z);
        sum_x += x; sum_y += y; sum_z += z;
        vTaskDelay(pdMS_TO_TICKS(GYRO_CALIB_DELAY));
    }

    int32_t gyro_bias_x = sum_x / GYRO_CALIB_SAMPLES;
    int32_t gyro_bias_y = sum_y / GYRO_CALIB_SAMPLES;
    int32_t gyro_bias_z = sum_z / GYRO_CALIB_SAMPLES;

    char msg[64];
    int len = snprintf(msg, sizeof(msg), "Gyro bias: X=%ld, Y=%ld, Z=%ld\r\n",
                       (long)gyro_bias_x, (long)gyro_bias_y, (long)gyro_bias_z);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, len, 100);
}