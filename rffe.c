#include "rffe.h"
#include <stdlib.h>

struct rffe;

void SetPinOutput(GPIO_TypeDef *port, uint16_t pin);

static TIM_HandleTypeDef rffe_TimerInstance = {
    .Instance = RFFE_TIM
};

/* Initialise the RFFE pins and prepare the timer.
 */
void RFFE_Init(uint8_t slave_addr, GPIO_TypeDef *gpio_port, uint16_t sck_pin, uint16_t sda_pin) {
    rffe.slave_addr = slave_addr;
    rffe.sda = sda_pin;
    rffe.sck = sck_pin;
    rffe.port = gpio_port;

    SetPinOutput(gpio_port, sck_pin);
    SetPinOutput(gpio_port, sda_pin);

    RFFE_TIMER_ENABLE();
    RFFE_SetSpeed(RFFE_DEFAULT_PRESCALER, RFFE_DEFAULT_PERIOD, RFFE_DEFAULT_CLKDIV);
}

/* Set the values that control the speed of the RFFE bus.
 */
void RFFE_SetSpeed(uint16_t prescaler, uint16_t period, uint32_t clk_div) {
    rffe.tim_prescaler = prescaler;
    rffe.tim_period = period;

    rffe_TimerInstance.Init.Prescaler = prescaler;
    rffe_TimerInstance.Init.CounterMode = TIM_COUNTERMODE_UP;
    rffe_TimerInstance.Init.Period = period;
    rffe_TimerInstance.Init.ClockDivision = clk_div;
    HAL_TIM_Base_Init(&rffe_TimerInstance);
}

/* Private func: Simple helper for setting pins as outputs
 */
void SetPinOutput(GPIO_TypeDef *port, uint16_t pin) {
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

/* Private func: Simple helper for setting pins as inputs
 */
void SetPinInput(GPIO_TypeDef *port, uint16_t pin) {
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

/* Private func: Helper for packing int values into specific positions within a buffer.
 * Packs 'val' into 'buff', starting from index 'start_ix' and ending after 'len' bits.
 */
uint8_t PackBitsIntoBuff(uint8_t *buff, int val, uint8_t start_ix, uint8_t len) {
    for (int i = 0; i < len; i++) {
        buff[start_ix + i] = (val >> (len - i - 1)) & 1;
    }
    return start_ix + len;
}

/* Private func: Helper for getting an even parity bit for val
 */
uint8_t GetEvenParity(uint32_t val) {
    for (int i = 16; i > 0; i /= 2) {
        val ^= val >> i;
    }
    return val & 1;
}

/* Private func: Helper for getting an odd parity bit for val
 */
uint8_t GetOddParity(uint32_t val) {
    return (~GetEvenParity(val)) & 1;
}

/* Private func: Send data from 'buff' up until the index 'rx_start_ix',
 * then receive into the same buffer from the given index. Can also just 
 * transmit all data in 'buff', using 'rx_start_ix' = 0 (write only call).
 */
void RFFE_SendRecvBitBuffer(uint8_t *buff, uint8_t buff_len, uint8_t rx_start_ix) {
    HAL_TIM_Base_Start(&rffe_TimerInstance);

    uint8_t bytesSent = 0; // this count will include the two start sequence bits.
    uint8_t sckState = 0;
    uint8_t sdaState = 0;
    uint8_t readMode = 0;
    HAL_GPIO_WritePin(rffe.port, rffe.sck, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(rffe.port, rffe.sda, GPIO_PIN_RESET);
    while (bytesSent < buff_len + RFFE_START_SEQ_LEN) {
        int timerValue = __HAL_TIM_GET_COUNTER(&rffe_TimerInstance);
        if (rx_start_ix != 0 && bytesSent >= rx_start_ix + RFFE_START_SEQ_LEN && readMode == 0) {
            readMode = 1;
            SetPinInput(rffe.port, rffe.sda);
        }
        if (timerValue == rffe.tim_period/2 && sckState == 1) {
            if (bytesSent > RFFE_START_SEQ_LEN) { // Only change CLK pin after the start sequence
                HAL_GPIO_WritePin(rffe.port, rffe.sck, GPIO_PIN_RESET);
            }
            sckState = 0; // Set the CLK state always, to retain timing
        }
        if (timerValue == rffe.tim_period && sckState == 0) {   
            bytesSent++;
            if (bytesSent > RFFE_START_SEQ_LEN) { // Only change CLK pin after the start sequence
                uint8_t buffIx = bytesSent - RFFE_START_SEQ_LEN - 1;
                if (readMode) {   // In read mode, read before clocking
                    buff[buffIx] = HAL_GPIO_ReadPin(rffe.port, rffe.sda);
                }

                HAL_GPIO_WritePin(rffe.port, rffe.sck, GPIO_PIN_SET);

                if (!readMode) {   // In write mode, clock first, then set SDA
                    HAL_GPIO_WritePin(rffe.port, rffe.sda, buff[buffIx] > 0);
                    sdaState = buff[buffIx];
                }
            } else {
                sdaState ^= 1;
                HAL_GPIO_WritePin(rffe.port, rffe.sda, sdaState);
            }
            sckState = 1; // Set the CLK state always, to retain timing
        }
    }
    HAL_GPIO_WritePin(rffe.port, rffe.sck, GPIO_PIN_RESET);
    HAL_Delay(3); // Give the slave a few milliseconds to get out of write mode...
    SetPinOutput(rffe.port, rffe.sda);
    HAL_GPIO_WritePin(rffe.port, rffe.sda, GPIO_PIN_RESET);

    HAL_TIM_Base_Stop(&rffe_TimerInstance);
}

/* Prepare the write byte packet for a specific register address and send it off.
 */
void RFFE_WriteByte(uint8_t addr, uint8_t data) {

    // Set up the buffer to transmit
    uint8_t addrWithParity = addr << 1 | GetOddParity(addr);
    uint16_t dataWithParity = ((uint16_t)data) << 1 | GetOddParity(data);

    uint8_t dataPlusParityLength = 9;
    uint8_t packetLength = RFFE_SLAVE_ADDR_LEN + RFFE_READ_WRITE_FLAG_LEN + 
            RFFE_REG_ADDR_PLUS_PARITY_LEN + dataPlusParityLength + 1;

    uint8_t *buff = (uint8_t*) malloc(sizeof(uint8_t) * packetLength);
    uint8_t nextIx = 0;
    nextIx = PackBitsIntoBuff(buff, rffe.slave_addr, nextIx, RFFE_SLAVE_ADDR_LEN);
    nextIx = PackBitsIntoBuff(buff, RFFE_WRITE_FLAG, nextIx, RFFE_READ_WRITE_FLAG_LEN);
    nextIx = PackBitsIntoBuff(buff, addrWithParity, nextIx, RFFE_REG_ADDR_PLUS_PARITY_LEN);
    nextIx = PackBitsIntoBuff(buff, dataWithParity, nextIx, dataPlusParityLength);
    buff[packetLength - 1] = 0; // Bus park bit


    // Send buffered data
    RFFE_SendRecvBitBuffer(buff, packetLength, 0);
}

/* Prepare the read byte packet for a specific register address and perform the call.
 */
uint8_t RFFE_ReadByte(uint8_t addr) {

    // Set up the buffer to transmit
    uint8_t addrWithParity = addr << 1 | GetEvenParity(addr); // Expected this to be odd!

    uint8_t dataLength = 8;
    uint8_t packetLength = RFFE_SLAVE_ADDR_LEN + RFFE_READ_WRITE_FLAG_LEN + 
            RFFE_REG_ADDR_PLUS_PARITY_LEN + 1 + dataLength + 2; // A bus park and a parity at the end

    uint8_t *buff = (uint8_t*) malloc(sizeof(uint8_t) * packetLength);
    uint8_t nextIx = 0;
    nextIx = PackBitsIntoBuff(buff, rffe.slave_addr, nextIx, RFFE_SLAVE_ADDR_LEN);
    nextIx = PackBitsIntoBuff(buff, RFFE_READ_FLAG, nextIx, RFFE_READ_WRITE_FLAG_LEN);
    nextIx = PackBitsIntoBuff(buff, addrWithParity, nextIx, RFFE_REG_ADDR_PLUS_PARITY_LEN);
    buff[nextIx] = 0; // Bus park bit

    uint8_t outputVal = 0;

    // Send buffered data
    RFFE_SendRecvBitBuffer(buff, packetLength, ++nextIx);

    // Get received data (expect odd parity for data frames)
    for (int i = ++nextIx; i < nextIx + dataLength; i++) {
        outputVal |= buff[i] << (nextIx + dataLength - i - 1);
    }
    uint8_t outputParity = buff[nextIx + dataLength]; // Currently not doing anything with this...

    return outputVal;
}

/* Prepare the write byte packet for register address 0x00 and send it off.
 */
void RFFE_WriteReg0Byte(uint8_t data) {

    // Set up the buffer to transmit
    uint16_t dataWithParity = ((uint16_t)data) << 1 | GetOddParity(data);

    // For REG_0 writes, most significant bit is always 1 (9 bits for data plus parity)
    dataWithParity |= 0x100;

    uint8_t dataPlusParityLength = 9;
    uint8_t packetLength = RFFE_SLAVE_ADDR_LEN + dataPlusParityLength + 1;

    uint8_t *buff = (uint8_t*) malloc(sizeof(uint8_t) * packetLength);
    uint8_t nextIx = 0;
    nextIx = PackBitsIntoBuff(buff, rffe.slave_addr, nextIx, RFFE_SLAVE_ADDR_LEN);
    nextIx = PackBitsIntoBuff(buff, dataWithParity, nextIx, dataPlusParityLength);
    buff[packetLength - 1] = 0; // Bus park bit


    // Send buffered data
    RFFE_SendRecvBitBuffer(buff, packetLength, 0);
}