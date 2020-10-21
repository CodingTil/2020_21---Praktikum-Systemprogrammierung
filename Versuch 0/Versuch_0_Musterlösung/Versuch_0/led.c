/*
 * led.c
 *
 * Created: 31.01.2014 10:15:10
 *  Author: sr466916
 */ 

#include "led.h"
#include "button.h"

#include <avr/io.h>

uint8_t result = 0;

void led_init() {
	//LEDs
	DDRA = 0xFF;
	PORTA = 0XFF;
}

void led_fun() {
	//Init
	PORTA = ~result;
		
	// Check Button1
	if(getInput() == 0b1){
		waitForNoInput();
		//LED On/Off
		result = result ^ 0b1; //Kritik: einfacher mit Variable/LED abfragen
	}
		
	// Check Button2
	if(getInput() == 0b10){
		waitForNoInput();
		uint8_t last_pin = result &0x80;
		result = result << 1;
		if(last_pin)
		result |= 0b1;
	}
		
	// Check Button3
	if(getInput() == 0b100){
		waitForNoInput();
		result = ~result;
	}
}