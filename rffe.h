#ifndef RFFE_H
#define RFFE_H

#include "stm32l4xx_hal.h"

// With the default timer settings, runs at around 35 kHz on L433
#define RFFE_DEFAULT_PRESCALER 56
#define RFFE_DEFAULT_PERIOD 40
#define RFFE_DEFAULT_CLKDIV TIM_CLOCKDIVISION_DIV1

// Set the timer to use for driving the pins
#define RFFE_TIMER_ENABLE() __TIM2_CLK_ENABLE()
#define RFFE_TIM TIM2

// RFFE-specific defines
#define RFFE_READ_FLAG 0b011
#define RFFE_WRITE_FLAG 0b010
#define RFFE_START_SEQ_LEN 2
#define RFFE_SLAVE_ADDR_LEN 4
#define RFFE_READ_WRITE_FLAG_LEN 3
#define RFFE_REG_ADDR_PLUS_PARITY_LEN 6

struct rffe {
    uint8_t slave_addr;
    uint16_t sck;
    uint16_t sda;
    GPIO_TypeDef *port;
    uint16_t tim_prescaler;
    uint16_t tim_period;
} rffe;

void RFFE_Init(uint8_t slave_addr, GPIO_TypeDef *gpio_port, uint16_t sck_pin, uint16_t sda_pin);
void RFFE_SetSpeed(uint16_t prescaler, uint16_t period, uint32_t clk_div);
void RFFE_WriteByte(uint8_t addr, uint8_t data);
void RFFE_WriteReg0Byte(uint8_t data);
uint8_t RFFE_ReadByte(uint8_t addr);

#endif