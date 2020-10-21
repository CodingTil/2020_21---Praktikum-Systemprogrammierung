#include "button.h"
#include "lcd.h"

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

/*! \file

Everything that is necessary to get the input from the Buttons in a clean format.

*/


/*!
 *  A simple "Getter"-Function for the Buttons on the evaluation board.\n
 * 
 *  \returns The state of the button(s) in the lower bits of the return value.\n
 *  example: 1 Button:  -pushed:   00000001
 *                      -released: 00000000
 *           4 Buttons: 1,3,4 -pushed: 000001101
 */
uint8_t getInput(void) {
	return (~PIND &0xF);
}

/*!
 *	Initializes DDR and PORT for input
 */
void initInput(){
	//Set Data Direction Register of used pins of PortC to input
	DDRD &= ~(0b00001111);

	//Set Pullups
	PORTD |= 0b00001111;
}

/*!
 *  Endless loop as long as at least one button is pressed.
 */
void waitForNoInput() {
	while((PIND & 0b00001111) != 0b00001111);
}

/*!
 *  Endless loop until at least one button is pressed.
 */
void waitForInput() {
	//Wait as long as no button is pushed and then just return
	//Note that the buttons are active low
	while((PIND & 0b00001111) == 0b00001111);
}

/*!
 *	Tests all button functions
 */
void buttonTest() {
	//init everything
	initInput();
	lcd_init();
	lcd_clear();
	lcd_writeProgString(PSTR("Initializing\nInput"));
	_delay_ms(1000);
	lcd_clear();
	if ((DDRD & 0x0F) != 0) 
	{
		lcd_writeProgString(PSTR("DDR wrong"));
		while (1);
	}
	else{
		lcd_writeProgString(PSTR("DDR Ok"));
		_delay_ms(1000);
	}
	
	lcd_clear();
	if ((PORTD & 0x0F) != 0x0F)
	{
		lcd_writeProgString(PSTR("Pullups wrong"));
		while(1);
	}
	else{
		lcd_writeProgString(PSTR("Pullups Ok"));
		_delay_ms(1000);
	}

	//test all 4 buttons
	for (uint8_t i = 0; i < 4; i++) {
		lcd_clear();
		lcd_writeProgString(PSTR("Press Button "));
		lcd_writeDec(i + 1);
		uint8_t pressed = 0;
		while (!pressed)
		{
			waitForInput();
			uint8_t buttonsPressed = getInput();
			if (buttonsPressed != 1 << i) //Not the right button pressed so show which ones were pressed and wait for release
			{
				lcd_line2();
				lcd_writeProgString(PSTR("Pressed: "));
				lcd_writeHexByte(buttonsPressed);
				waitForNoInput();
				lcd_line2();
				lcd_writeProgString(PSTR("                "));
			}else {	//right button pressed so wait for release and continue to next one
				pressed = 1;
				lcd_clear();
				lcd_writeProgString(PSTR("Release Button "));
				lcd_writeDec(i + 1);
				waitForNoInput();
			}
		}
	}
	lcd_clear();
	lcd_writeProgString(PSTR("Test passed"));
	while(1);
}
