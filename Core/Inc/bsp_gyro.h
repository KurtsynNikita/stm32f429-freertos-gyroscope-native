#ifndef BSP_GYRO_H
#define BSP_GYRO_H

#include "stm32f4xx_hal.h"

// Регистры Гироскопа
#define GYRO_WHO_AM_I_REG   0x0F
#define GYRO_CTRL_REG1      0x20
#define GYRO_CTRL_REG4      0x23
#define GYRO_OUT_X_L_REG    0x28

// ID Гироскопа
#define GYRO_ID_I3G4250D    0xD3
#define GYRO_ID_L3GD20      0xD4

// Настройки гироскопа
#define GYRO_CFG_NORMAL_100HZ  0x0F
#define GYRO_CFG_BDU_245DPS    0x80

// Пины SPI
#define GYRO_SPI            SPI5
#define GYRO_CS_PORT        GPIOC
#define GYRO_CS_PIN         1

#define SPI_SCK_PIN         7
#define SPI_MISO_PIN        8
#define SPI_MOSI_PIN        9
#define SPI_PORT            GPIOF
#define SPI_AF              GPIO_AF5_SPI5

#define GYRO_SPI_BR_DIV_64  (4U << 3) 

// Биты регистров SPI
#define SPI_READ_CMD        0x80
#define SPI_WRITE_CMD       0x00
#define SPI_MULTI_BYTE_CMD  0x40

// Параметры калибровки
#define GYRO_CALIB_SAMPLES  100
#define GYRO_CALIB_DELAY    5

void BSP_GYRO_Init(void);
void Gyro_ReadXYZ(int16_t *x, int16_t *y, int16_t *z);
void Gyro_Calibrate(void);

#endif