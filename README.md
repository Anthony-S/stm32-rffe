# stm32-rffe
## Bit-banged MIPI RFFE control library developed for STM32 HAL platforms
This code bit-bangs communication using STM32 HAL timers and GPIO to emulate the [MIPI RFFE](https://mipi.org/specifications/rf-front-end) protocol. It was developed (hastily) for interfacing with a [Skyworks SKY13492-21](http://www.skyworksinc.com/Product/1755/SKY13492-21) SP16T RF switch ([datasheet](http://www.skyworksinc.com/uploads/documents/SKY13492_21_202826F.pdf)) - the header file used for that is also included in the repo. The code was developed using an STM32L433 Nucleo dev board, as 1.8V IO comes easy on that board. It would be simple enough to port to another STM32 board, and with a little more effort could be used on an entirely different platform.

Note that I didn't have access to the official RFFE spec document -- I simply inferred as much as I could from the skyworks datasheet, so be wary that changes to the library might be needed depending on the chip you are communicating with. Basic usage is as follows:

```
#include "rffe.h"
#include "SKY13492_rffe.h"

#define RFFE_GPIO_PORT GPIOX
#define RFFE_GPIO_CLK_ENABLE() __GPIOX_CLK_ENABLE()
#define RFFE_SCK_PIN GPIO_PIN_XX
#define RFFE_SDA_PIN GPIO_PIN_XX

int main(void)
{

    HAL_Init();

    SetSystemClock();

    RFFE_GPIO_CLK_ENABLE();
    RFFE_Init(SKY13492_SLAVE_ADDR, RFFE_GPIO_PORT, RFFE_SCK_PIN, RFFE_SDA_PIN);
    
    while (1) 
    {
        // Example read from register
        uint8_t switchState = RFFE_ReadByte(SKY13492_SWITCH_CTRL_REG);

        // Example write to register
        int newGroupSIDValue = 0x02;
        RFFE_WriteByte(SKY13492_GROUP_SID_REG, newGroupSIDValue);

        // Example quick write to register zero
        switchState = SKY13492_SWITCH_TRX6;
        RFFE_WriteReg0Byte(switchState);
    }

}

```

## Also note...
I expected the parity bits in RFFE to always be odd, from what I could Google. For some strange reason, the read function required an even parity bit on the register address, otherwise the SKY13942 wouldn't respond. Not sure if this is expected in RFFE or if skyworks have a bug in their implementation, so you may need to change this for other devices. Note that all other parity bits are odd in the write functions (and even on the data that comes back from the SKY13942), so they should be as per spec.