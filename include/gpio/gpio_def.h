#ifndef _GPIO_DEF_H
#define _GPIO_DEF_H

#define PIN(GPIOx, IOx) ((GPIOx-1)*32+IOx)


// facility switch
#define PIN_AUTO    PIN(1, 15)
#define PIN_MSG1    PIN(1, 14)
#define PIN_MSG2    PIN(1, 13)

// power supply & battery
#define PIN_MAIN_FAILURE    PIN(1, 10)
#define PIN_BATTERY_LOW     PIN(1, 11)
#define PIN_BATTERY_OPEN    PIN(1, 12)


#define PIN_CN7_4_IN9       PIN(4,23)
#define PIN_CN7_7_MSG3      PIN(1,22)
#define PIN_CN7_8_MSG4      PIN(4,20)
#define PIN_CN7_9_MSG5      PIN(4,24)
#define PIN_CN7_10_IN10     PIN(4,19)

#define PIN_CN9_10      PIN(1,23)

// command control power
#define PIN_MOSFET1_CTRL        PIN(1, 27)


// heartbeat & status LED
#define PIN_HB_LED      PIN(1, 5)
#define PIN_ST_LED      PIN(1, 9)

#define PIN_RELAY_CTRL      PIN(4, 14)

#define PIN_MOSFET2_CTRL     PIN(4, 16)

#define PIN_WDT         PIN(1, 18)

#define PIN_UART3_CTS   PIN(1, 26)


// spi gpio 504-511


#endif //_GPIO_DEF_H
