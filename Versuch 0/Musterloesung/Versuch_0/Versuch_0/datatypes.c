/*
 * datatypes.c
 *
 * Created: 31.01.2014 10:19:37
 *  Author: sr466916
 */ 

#include "datatyes.h"
#include "button.h"

void convert() {
	uint8_t target = 200; //Solution: uint8_t target = 200;
	
	uint16_t i = target + 98;
	target = i;
	lcd_writeProgString(PSTR("Convert finished"));
	lcd_line2();
	lcd_writeDec(target);
	waitForInput();
	waitForNoInput();
	lcd_clear();
}

void loop() {
	uint8_t result = 0;
	for (int8_t i = 5; i >= 0; i--) //Solution: int8_t i = 5 ...
	{
		result += i * 2 + 2;
	}
	lcd_writeProgString(PSTR("Loop finished:"));
	lcd_line2();
	lcd_writeDec(result);
	waitForInput();
	waitForNoInput();
	lcd_clear();
}

void shift() {
	uint32_t result = (uint32_t)1 << 31; //Solution: (uint32_t)1 << 31
	
	lcd_writeProgString(PSTR("Shift finished"));
	lcd_line2();
	lcd_write32bitHex(result);
	waitForInput();
	waitForNoInput();
	lcd_clear();
}

