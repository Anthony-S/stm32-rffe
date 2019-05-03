#ifndef SKY13492_RFFE_H
#define SKY13492_RFFE_H

/* Definitions parsed from the SKY13492_21 datasheet.
 */

#define SKY13492_SLAVE_ADDR 0b1011

#define SKY13492_SWITCH_CTRL_REG 0x00           // Switch control register, uses the below switch state values
#define SKY13492_RFFE_STATUS_REG 0x1A           // Reset [bit 7] and error flags
#define SKY13492_GROUP_SID_REG 0x1B             // Default value of 0x00
#define SKY13492_PM_TRIG_REG 0x1C               // Power modes and trigger management
#define SKY13492_PRODUCT_ID_REG 0x1D            // Read-only value of 0b01011101
#define SKY13492_MANUFACTURER_ID_REG 0x1E       // Read-only value of 0b10100101
#define SKY13492_MAN_USID_REG 0x1F              // Default value of 0b00011011

#define SKY13492_SWITCH_TRX1      0b00000100
#define SKY13492_SWITCH_TRX2      0b00000101
#define SKY13492_SWITCH_TRX3      0b00000110
#define SKY13492_SWITCH_TRX4      0b00000111
#define SKY13492_SWITCH_TRX5      0b00001001
#define SKY13492_SWITCH_TRX6      0b00001011
#define SKY13492_SWITCH_TRX7      0b00001100
#define SKY13492_SWITCH_TRX8      0b00000001
#define SKY13492_SWITCH_TRX9      0b00000010
#define SKY13492_SWITCH_TRX10     0b00000011
#define SKY13492_SWITCH_TRX11     0b00001101
#define SKY13492_SWITCH_TRX12     0b00001110
#define SKY13492_SWITCH_TRX13     0b00010000
#define SKY13492_SWITCH_TRX14     0b00010001
#define SKY13492_SWITCH_TRXHB     0b00001000
#define SKY13492_SWITCH_TRXLB     0b00001010
#define SKY13492_SWITCH_ISOLATED  0b01111111

#endif