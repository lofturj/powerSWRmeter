//*********************************************************************************
//**
//**  The below code relies on a direct swipe from the AVRLIB lcd.c/h.
//**                  Therefore see the AVRLIB copyright notice below.
//** 
//**  The essentials for bargraph display have been copied and improved/adapted to
//**  my own taste, including several different customized bargraph display styles.
//**
//**  Peak Bar (sticky bar) indicator added as an option.
//**
//**  Initial version.: 2009-09-08, Loftur Jonasson, TF3LJ
//**
//**  Last update to this file: 2013-09-13, Loftur Jonasson, TF3LJ / VE2LJX
//**
//**
//*********************************************************************************

// Copy/Paste of copyright notice from AVRLIB lcd.h:
//*****************************************************************************
//
// File Name	: 'lcd.h'
// Title		: Character LCD driver for HD44780/SED1278 displays
//					(usable in mem-mapped, or I/O mode)
// Author		: Pascal Stang
// Created		: 11/22/2000
// Revised		: 4/30/2002
// Version		: 1.1
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
///	\ingroup driver_hw
/// \defgroup lcd Character LCD Driver for HD44780/SED1278-based displays (lcd.c)
/// \code #include "lcd.h" \endcode
/// \par Overview
///		This display driver provides an interface to the most common type of
///	character LCD, those based on the HD44780 or SED1278 controller chip
/// (about 90% of character LCDs use one of these chips).  The display driver
/// can interface to the display through the CPU memory bus, or directly via
/// I/O port pins.  When using the direct I/O port mode, no additional
/// interface hardware is needed except for a contrast potentiometer.
/// Supported functions include initialization, clearing, scrolling, cursor
/// positioning, text writing, and loading of custom characters or icons
/// (up to 8).  Although these displays are simple, clever use of the custom
/// characters can allow you to create animations or simple graphics.  The
/// "progress bar" function that is included in this driver is an example of
/// graphics using limited custom-chars.
///
/// \Note The driver now supports both 8-bit and 4-bit interface modes.
///
/// \Note For full text output functionality, you may wish to use the rprintf
/// functions along with this driver
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#include "PM.h"

// progress bar defines
#define PROGRESSPIXELS_PER_CHAR	6

// custom LCD characters
const unsigned char __attribute__ ((progmem)) LcdCustomChar[] =
{
	//
	// Five different bargrahph alternatives, the fourth alternative is the original
	// bargraph in the AVRLIB library.  TF3LJ - 2009-08-25
	//
	#if BARGRAPH_STYLE_1		// Used if LCD bargraph alternatives.  N8LP LP-100 look alike bargraph
	0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, // 0. 0/5 full progress block
	0x00, 0x10, 0x10, 0x15, 0x10, 0x10, 0x00, 0x00, // 1. 1/5 full progress block
	0x00, 0x18, 0x18, 0x1d, 0x18, 0x18, 0x00, 0x00, // 2. 2/5 full progress block
	0x00, 0x1c, 0x1c, 0x1d, 0x1C, 0x1c, 0x00, 0x00, // 3. 3/5 full progress block
	0x00, 0x1e, 0x1e, 0x1E, 0x1E, 0x1e, 0x00, 0x00, // 4. 4/5 full progress block
	0x00, 0x1f, 0x1f, 0x1F, 0x1F, 0x1f, 0x00, 0x00, // 5. 5/5 full progress block
	0x06, 0x06, 0x06, 0x16, 0x06, 0x06, 0x06, 0x06, // 6. Peak Bar
	#endif
	#if BARGRAPH_STYLE_2		// Used if LCD bargraph alternatives.  Bargraph with level indicators
	0x01, 0x01, 0x1f, 0x00, 0x00, 0x1f, 0x00, 0x00, // 0. 0/5 full progress block
	0x01, 0x01, 0x1f, 0x10, 0x10, 0x1f, 0x00, 0x00, // 1. 1/5 full progress block
	0x01, 0x01, 0x1f, 0x18, 0x18, 0x1f, 0x00, 0x00, // 2. 2/5 full progress block
	0x01, 0x01, 0x1f, 0x1C, 0x1C, 0x1f, 0x00, 0x00, // 3. 3/5 full progress block
	0x01, 0x01, 0x1f, 0x1E, 0x1E, 0x1f, 0x00, 0x00, // 4. 4/5 full progress block
	0x01, 0x01, 0x1f, 0x1F, 0x1F, 0x1f, 0x00, 0x00, // 5. 5/5 full progress block
	0x07, 0x07, 0x1f, 0x06, 0x06, 0x1f, 0x06, 0x06, // 6. Peak Bar
	#endif
	#if BARGRAPH_STYLE_3		// Used if LCD bargraph alternatives.  Another bargraph with level indicators
	0x01, 0x01, 0x1f, 0x00, 0x00, 0x00, 0x1F, 0x00, // 0. 0/5 full progress block
	0x01, 0x01, 0x1f, 0x10, 0x10, 0x10, 0x1F, 0x00, // 1. 1/5 full progress block
	0x01, 0x01, 0x1f, 0x18, 0x18, 0x18, 0x1F, 0x00, // 2. 2/5 full progress block
	0x01, 0x01, 0x1f, 0x1C, 0x1C, 0x1C, 0x1F, 0x00, // 3. 3/5 full progress block
	0x01, 0x01, 0x1f, 0x1E, 0x1E, 0x1E, 0x1F, 0x00, // 4. 4/5 full progress block
	0x01, 0x01, 0x1f, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, // 5. 5/5 full progress block
	0x07, 0x07, 0x1f, 0x06, 0x06, 0x06, 0x1f, 0x06, // 6. Peak Bar
	#endif
	#if BARGRAPH_STYLE_4		// Used if LCD bargraph alternatives.  Original bargraph, Empty space enframed
	0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, // 0. 0/5 full progress block
	0x00, 0x1F, 0x10, 0x10, 0x10, 0x10, 0x1F, 0x00, // 1. 1/5 full progress block
	0x00, 0x1F, 0x18, 0x18, 0x18, 0x18, 0x1F, 0x00, // 2. 2/5 full progress block
	0x00, 0x1F, 0x1C, 0x1C, 0x1C, 0x1C, 0x1F, 0x00, // 3. 3/5 full progress block
	0x00, 0x1F, 0x1E, 0x1E, 0x1E, 0x1E, 0x1F, 0x00, // 4. 4/5 full progress block
	0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, // 5. 5/5 full progress block
	0x06, 0x1f, 0x06, 0x06, 0x06, 0x06, 0x1f, 0x06, // 6. Peak Bar
	#endif
	#if BARGRAPH_STYLE_5		// Used if LCD bargraph alternatives.  True bargraph, Empty space is empty
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0. 0/5 full progress block
	0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, // 1. 1/5 full progress block
	0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, // 2. 2/5 full progress block
	0x00, 0x1c, 0x1C, 0x1C, 0x1C, 0x1C, 0x1c, 0x00, // 3. 3/5 full progress block
	0x00, 0x1e, 0x1E, 0x1E, 0x1E, 0x1E, 0x1e, 0x00, // 4. 4/5 full progress block
	0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, // 5. 5/5 full progress block
	0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, // 6. Peak Bar
	#endif

};


//-----------------------------------------------------------------------------------------
// Load custom character into HD44780 LCD controller
//-----------------------------------------------------------------------------------------
void lcdLoadCustomChar(uint8_t* lcdCustomCharArray, uint8_t romCharNum, uint8_t lcdCharNum)
{
	uint8_t i;

	// multiply the character index by 8
	lcdCharNum = (lcdCharNum<<3);	// each character occupies 8 bytes
	romCharNum = (romCharNum<<3);	// each character occupies 8 bytes

	// copy the 8 bytes into CG (character generator) RAM
	for(i=0; i<8; i++)
	{
		// set CG RAM address
		lcdControlWrite((1<<LCD_CGRAM) | (lcdCharNum+i));
		// write character data
		lcdDataWrite( pgm_read_byte(lcdCustomCharArray+romCharNum+i));
	}
}


//-----------------------------------------------------------------------------------------
// Display Bargraph - including Peak Bar, if relevant
//
// "length" indicates length of bargraph in characters 
// (max 16 on a 16x2 display or max 20 on a 20x4 display)
//
// (each character consists of 6 bars, thereof only 5 visible)
//
// "maxprogress" indicates full scale (16 bit unsigned integer)
//
// "progress" shown as a proportion of "maxprogress"  (16 bit unsigned integer)
//
// if "prog_peak" (16 bit unsigned integer) is larger than "progress",
// then Peak Bar is shown in the middle of that character position
//-----------------------------------------------------------------------------------------
void lcdProgressBarPeak(uint16_t progress, uint16_t prog_peak, uint16_t maxprogress, uint8_t length)
{
	uint8_t i;
	uint16_t pixelprogress;
	uint8_t c;

	if (progress >= maxprogress) progress = maxprogress;	// Clamp the upper bound to prevent funky readings

	// draw a progress bar displaying (progress / maxprogress)
	// starting from the current cursor position
	// with a total length of "length" characters
	// ***note, LCD chars 0-6 must be programmed as the bar characters
	// char 0 = empty ... char 5 = full, char 6 = peak bar - disabled if maxprogress set as 0 (or lower than progress)

	// total pixel length of bargraph equals length*PROGRESSPIXELS_PER_CHAR;
	// pixel length of bar itself is
	pixelprogress = ((uint32_t)progress*(length*PROGRESSPIXELS_PER_CHAR)/maxprogress);
		
	// print exactly "length" characters
	for(i=0; i<length; i++)
	{
		// check if this is a full block, or partial or empty
		if( ((i*PROGRESSPIXELS_PER_CHAR)+PROGRESSPIXELS_PER_CHAR) > pixelprogress )
		{
			// this is a partial or empty block
			if( ((i*PROGRESSPIXELS_PER_CHAR)) > pixelprogress )
			{
				// If an otherwise empty block contains previous "Peak", then print peak char
				// If this function is not desired, simply set prog_peak at 0 (or as equal to progress)
				if(i == ((uint32_t)length * prog_peak)/maxprogress)
					c = 6;				
				// othwerwise this is an empty block
				// use space character?
				else
					c = 0;
			}
			else
			{
				// this is a partial block
				c = pixelprogress % PROGRESSPIXELS_PER_CHAR;
			}
		}
		else
		{
			// this is a full block
			c = 5;
		}
		
		// write character to display
		lcdDataWrite(c);
	}
}


//-----------------------------------------------------------------------------------------
// Initialize LCD for bargraph display - Load 6 custom bargraph symbols
//-----------------------------------------------------------------------------------------
void lcd_bargraph_Init(void)
{

	for (uint8_t i=0; i<7; i++)
	{
		lcdLoadCustomChar((uint8_t*)LcdCustomChar,i,i);
	}
}

