/*
 * structures.c
 *
 * Created: 31.03.2014 16:52:47
 *  Author: Stefan
 */ 

#include "structures.h"
#include "lcd.h"
#include "button.h"
#include <stdint.h>

enum DistributionStatus {
	AVAILABLE,
	BACKORDER,
	SOLD_OUT = 99,
};
	
typedef enum DistributionStatus DistributionStatus; //Typedef für enum DistributioStatus

typedef struct  {
	uint8_t manufacturerID;
	uint8_t productID;
}ArticleNumber;

typedef union {
	uint16_t combinedNumber;
	ArticleNumber singleNumbers;
}FullArticleNumber;

typedef struct {
	FullArticleNumber articleNumber;
	DistributionStatus status;
}Article;


void displayArticles() {
	lcd_clear();
	Article articles[] = {{{0x1101}, AVAILABLE}, {{0x1110}, BACKORDER}, {{0x0101}, SOLD_OUT}}; //Initializiere einen Array mit drei Artikeln
	for (uint8_t i = 0; i < 3; i++)  { //Durch alle Artikel loopen
		lcd_clear();
		lcd_writeProgString(PSTR("Hersteller: "));
		lcd_writeHexByte(articles[i].articleNumber.singleNumbers.manufacturerID); //Ausgabe der ManufactureID
		lcd_line2();
		lcd_writeProgString(PSTR("Art#"));
		lcd_writeHexByte(articles[i].articleNumber.singleNumbers.productID); //Ausgabe der productID
		switch (articles[i].status) //Ausgabe des Richtigen Lables basierend auf status
		{
			case AVAILABLE:
				lcd_writeProgString(PSTR(" Lager"));
				break;
			case BACKORDER:
				lcd_writeProgString(PSTR(" Best."));
				break;
			case SOLD_OUT:
				lcd_writeProgString(PSTR(" Sold"));
				break;
			default:
				break;
		}
		waitForInput(); //Warten auf drücken eines Buttons
		waitForNoInput(); //und auf das wieder loslassen
	}
}

