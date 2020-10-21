#include "button.h"
#include "pointer.h"
#include "lcd.h"
#include <stdlib.h>

/*! 
 *	Optional Tutorial on Pointers.
 *	Follow the steps in the document.
 */

//----------------------------------------------------------------------------
// Function headers
//----------------------------------------------------------------------------
//! Function to display single steps during the tutorial on LCD.
void displayStep(uint8_t step, uint8_t newLine, const char string[], uint8_t character, uint16_t decimal);

//! Function to display the result on LCD.
void displayResult(uint8_t* pAddress);

//! Simple Example of Call By Value.
void callByValue(uint8_t character);

//! Simple Example of Call By Reference.
void callByReference(uint8_t* character);

//! Simple function which returns a single char. Helps to realise an example for pointer on functions.
uint8_t charFunction(void);

//----------------------------------------------------------------------------
// Structure
//----------------------------------------------------------------------------
//! Simple dummy structure. Helps to realise an example for pointer on structs.
typedef struct sFirst{
	uint8_t bar;
} sFirst;

//! Simple second structure. Helps to realise an example for pointer on structs.
typedef struct sSecond{
	sFirst* sPoi;
} sSecond;


//----------------------------------------------------------------------------
//Required variables
//----------------------------------------------------------------------------
uint8_t alphabet[7] = {'P','O','I','N','T','E','R'};
uint8_t charVar = 'A';

//Here will be your result
uint8_t* memory;

//! Main function. Starting a tutorial on pointers.
void learningPointers(void){

	//----------------------------------------------------------------------------
	// Step 1: allocate some memory to store some characters
	//----------------------------------------------------------------------------
	memory = malloc(6 * sizeof(uint8_t));
	
	if(!memory){
		lcd_writeProgString(PSTR("Could not alloc"));
		while(1);
	}
	displayStep('1', 0, PSTR("Alloc Mem.Start:"), 0, (uint16_t)(memory));


	//----------------------------------------------------------------------------
	// Step 2: learning to count with addresses and dereferencing
	//----------------------------------------------------------------------------
	displayStep('2', 1, PSTR("Dereferencing"), 0, 0);
	*(memory+5) = alphabet[6];


	//----------------------------------------------------------------------------
	// Step 3: create a pointer and learn to use '*' and '&' operator
	//----------------------------------------------------------------------------
	charVar = 'A';
	displayStep('3', 1, PSTR("Character: "), charVar, 0);

	uint8_t* pCharVar = &charVar;
	*(pCharVar) = alphabet[2];

	displayStep('3', 1, PSTR("Character: "), charVar, 0);
	
	//----------------------------------------------------------------------------
	// Step 4: Call by Value and Call by Reference
	//----------------------------------------------------------------------------
	callByValue(alphabet[0]);
	displayStep('4', 0, PSTR(" Call by  Value: "), alphabet[0], 0);

	callByReference(alphabet);
	displayStep('4', 0, PSTR(" Call by  Reference: "), alphabet[0], 0);

	//----------------------------------------------------------------------------
	// Step 5: pointer on pointer
	//----------------------------------------------------------------------------
	uint8_t** ppChar = &pCharVar;
	pCharVar = alphabet;
	*(memory+4) = **ppChar;

	displayStep('5', 1, PSTR("**ppChar: "), **ppChar, 0);

	//----------------------------------------------------------------------------
	// Step 6: pointer on structure
	//----------------------------------------------------------------------------
	sFirst sfoo = {.bar = 'Z'};
	displayStep('6', 1, PSTR("sfoo.bar: "), sfoo.bar, 0);

	sSecond pStruct = {.sPoi = &sfoo};
	*(memory) = pStruct.sPoi->bar;
	
	//----------------------------------------------------------------------------
	// Step 7: function pointer
	//----------------------------------------------------------------------------
	uint8_t (*pCharFunction)(void) = &charFunction;
	*(memory+3) = pCharFunction();

	displayStep('7', 0, PSTR("FunctionPointer Addr:"), 0, (uint16_t)charFunction);


	//----------------------------------------------------------------------------
	// Step 8: Result is displayed on LCD.What is the expected result?
	//----------------------------------------------------------------------------
	displayResult(memory);
	free(memory);
}//End of tutorial.


//----------------------------------------------------------------------------
// Utility Functions
//----------------------------------------------------------------------------

//! Simple function which returns a single char. Helps to realise an example for pointer on functions.
uint8_t charFunction(void){
	return 'G';
}

//! Simple Example of Call By Value.
void callByValue(uint8_t character){
	character = 'E';
}

//! Simple Example of Call By Reference.
void callByReference(uint8_t* character){
	*character = 'E';
}


//! Function to display single steps during the tutorial on LCD.
void displayStep(uint8_t step, uint8_t newLine, const char string[], uint8_t character, uint16_t decimal){
	lcd_clear();
	lcd_writeProgString(PSTR("Step"));
	lcd_writeChar(step);
	lcd_writeChar(':');
	if(newLine)
		lcd_line2();
	lcd_writeProgString(string);
	if(character)
		lcd_writeChar(character);
	else if(decimal)
		lcd_writeDec(decimal);
	waitForInput();
	waitForNoInput();
}

uint8_t j = 0;
//! Function to display the result on LCD.
void displayResult(uint8_t* pAddress){
	*(memory+2) = charVar;
	*(memory+1) = alphabet[0];
	lcd_clear();
	lcd_writeProgString(PSTR("Solution:"));
	lcd_line2();
	uint8_t i;
	for(i=0; i<6; i++)
		lcd_writeChar(*(pAddress++));
	waitForInput();
	waitForNoInput();
}
