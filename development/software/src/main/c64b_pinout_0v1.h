
#ifndef C64B_PINOUT_0V1_H
#define C64B_PINOUT_0V1_H

#define PIN_KCA0 21
#define PIN_KCA1 19
#define PIN_KCA2 18

#define PIN_COL0 26
#define PIN_COL1 22
#define PIN_COL2 23
#define PIN_COL3 33
#define PIN_COL4 32

#define PIN_KRA0 17
#define PIN_KRA1 16
#define PIN_KRA2 4

#define PIN_ROW0 25
#define PIN_ROW1 14
#define PIN_ROW2 27
#define PIN_ROW3 12
#define PIN_ROW4 13

#define PIN_CTRL 2
#define PIN_SHFT 0
#define PIN_CMDR 15
#define PIN_KBEN 5

#if (CONFIG_BLUEPAD32_UART_OUTPUT_ENABLE == 1)
	#define PIN_nRST 1
#else
	#define PIN_nRST 255
#endif

#endif
