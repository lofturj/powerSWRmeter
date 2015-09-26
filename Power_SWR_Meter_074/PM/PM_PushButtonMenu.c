//*********************************************************************************
//**
//** Project.........: A menu driven Multi Display RF Power and SWR Meter
//**                   using a Tandem Match Coupler and 2x AD8307.
//**                                 or alternately
//**                   using 2 transformers in an V and I arrangement,
//**                   to feed 2x AD8307 and a MCK12140 based 360 degree
//**                   capable Phase Detector circuit, implementing an
//**                   RF Power/SWR and Phase + Impedance meter
//**
//** Copyright (C) 2014  Loftur E. Jonasson  (tf3lj [at] arrl [dot] net)
//**
//** This program is free software: you can redistribute it and/or modify
//** it under the terms of the GNU General Public License as published by
//** the Free Software Foundation, either version 3 of the License, or
//** (at your option) any later version.
//**
//** This program is distributed in the hope that it will be useful,
//** but WITHOUT ANY WARRANTY; without even the implied warranty of
//** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//** GNU General Public License for more details.
//**
//** You should have received a copy of the GNU General Public License
//** along with this program.  If not, see <http://www.gnu.org/licenses/>.
//**
//** Platform........: AT90usb1286 @ 16MHz
//**
//** Initial version.: 0.50, 2013-09-29  Loftur Jonasson, TF3LJ / VE2LJX
//**                   (beta version)
//**
//** History.........: Check the PM.c file
//**
//**
//*********************************************************************************

#include "PM.h"
#include "lcd.h"
#include <stdio.h>


// First Level Menu Items

const uint8_t level0_menu_size = 10;
const char *level0_menu_items[] =
				{	"1 SWR Alarm",
					"2 SWR Alarm Power",
					"3 PEP Period",
					"4 Screensaver",
					"5 Scale Ranges",					
					"6 Serial Data",
					"7 Calibrate",
					//"6 Adj Encoder Res",
					"8 Debug Display",
					"9 Reset to Default",
					"0 Exit" };

// Flags for SWR Alarm & Threshold menu
#define SWR_ALARM_THRESHOLD		1
#define	SWR_ALARM_PWR_THRESHOLD	2
// SWR Alarm Power Threshold menu Items
const uint8_t swr_alarm_pwr_thresh_menu_size = 5;
const char *swr_alarm_pwr_thresh_menu_items[] =
				{	"1   1mW",
					"2  10mW",
					"3  0.1W",
					"4    1W",
					"5   10W"	};

// Flag for PEP Sample Period select
#define PEP_MENU		3
// PEP Sample Period select menu Items
const uint8_t pep_menu_size = 3;
const char *pep_menu_items[] =
				{   "1    1s",
					"2  2.5s",
					"3    5s"	};

// Flag for Encoder Resolution Change
//#define ENCODER_MENU	8


// Flag for Screensaver Threshold select menu
#define SCREENTHRESH_MENU	5
// Screensaver Threshold select menu Items
const uint8_t screenthresh_menu_size = 6;
const char *screenthresh_menu_items[] =
				{   "1    Off",
					"2    1uW",
					"3   10uW",
					"4  100uW",
					"5    1mW",
					"6   10mW"	};
					

// Flag for Scale Range menu
#define SCALERANGE_MENU	6

// Flags for Scale Range Submenu functions
#define SCALE_SET0_MENU	600
#define SCALE_SET1_MENU	601
#define SCALE_SET2_MENU	602

// Flag for Serial Data Out
#define SERIAL_MENU		7
// Serial menu Items
const uint8_t serial_menu_size = 2;
const char *serial_menu_items[] =
				{   "1  Off",
					"2  On"	};

// Flag for Calibrate menu
#define CAL_MENU		8
const uint8_t calibrate_menu_size = 5;
const char *calibrate_menu_items[] =
				{   "1 OneLevelCal(dBm)",
					"2  1st level (dBm)",
					"3  2nd level (dBm)",
					"9 Go Back",
					"0 Exit"	};

// Flags for Calibrate Submenu functions
#define CAL_SET0_MENU	800	// Single level calibration
#define CAL_SET1_MENU	801	// 1st level
#define CAL_SET2_MENU	802	// 2nd level

// Flag for Debug Screen
#define DEBUG_MENU		9

// Flag for Factory Reset
#define FACTORY_MENU	11
// Factory Reset menu Items
const uint8_t factory_menu_size = 3;
const char *factory_menu_items[] =
				{  "1 No  - Go back",
				   "2 Yes - Reset",
				   "0 No  - Exit"};


uint16_t		menu_level = 0;						// Keep track of which menu we are in
uint8_t			menu_data = 0;						// Pass data to lower menu

int8_t			gain_selection;						// keep track of which GainPreset is currently selected

//----------------------------------------------------------------------
// Display a Menu of choices, one line at a time
//
// **menu refers to a pointer array containing the Menu to be printed
//
// menu_size indicates how many pointers (menu items) there are in the array
//
// current_choice indicates which item is currently up for selection if
// pushbutton is pushed
//
// begin row, begin_col indicate the upper left hand corner of the one, three or 
// four lines to be printed
//
// lines indicates how many lines are displayed at any one time, 1, 3 or 4.
//----------------------------------------------------------------------
void lcd_scroll_Menu(char **menu, uint8_t menu_size,
uint8_t current_choice, uint8_t begin_row, uint8_t begin_col, uint8_t lines)
{
	uint8_t a, x;

	// Clear LCD from begin_col to end of line.
	lcdGotoXY(begin_col, begin_row);
	for (a = begin_col; a < 20; a++)
		lcdPrintData(" ",1);
	if (lines > 1)
	{
		lcdGotoXY(begin_col, begin_row+1);
		for (a = begin_col; a < 20; a++)
		lcdPrintData(" ",1);
	}
	if (lines > 2)
	{
		lcdGotoXY(begin_col, begin_row+2);
		for (a = begin_col; a < 20; a++)
		lcdPrintData(" ",1);
	}
	// Using Menu list pointed to by **menu, preformat for print:
	// First line contains previous choice, secon line contains
	// current choice preceded with a '->', and third line contains
	// next choice
	if (current_choice == 0) x = menu_size - 1;
	else x = current_choice - 1;
	if (lines > 1)
	{
		lcdGotoXY(begin_col + 2, begin_row);
		sprintf(lcd_buf,"%s", *(menu + x));
		lcdPrintData(lcd_buf, strlen(lcd_buf));
		
		lcdGotoXY(begin_col, begin_row + 1);
		sprintf(lcd_buf,"->%s", *(menu + current_choice));
		lcdPrintData(lcd_buf, strlen(lcd_buf));
		if (current_choice == menu_size - 1) x = 0;
		else x = current_choice + 1;

		lcdGotoXY(begin_col + 2, begin_row + 2);
		sprintf(lcd_buf,"%s", *(menu + x));
		lcdPrintData(lcd_buf, strlen(lcd_buf));
	}
	else
	{
		lcdGotoXY(begin_col, begin_row);
		sprintf(lcd_buf,"->%s", *(menu + current_choice));
		lcdPrintData(lcd_buf, strlen(lcd_buf));
	}
	// LCD print lines 1 to 3

	// 4 line display.  Preformat and print the fourth line as well
	if (lines == 4)
	{
		if (current_choice == menu_size-1) x = 1;
		else if (current_choice == menu_size - 2 ) x = 0;
		else x = current_choice + 2;
		lcdGotoXY(begin_col, begin_row+3);
		for (a = begin_col; a < 20; a++)
		lcdPrintData(" ",1);
		lcdGotoXY(begin_col + 2, begin_row + 3);
		sprintf(lcd_buf,"  %s", *(menu + x));
		lcdPrintData(lcd_buf, strlen(lcd_buf));
	}
}


//----------------------------------------------------------------------
// Menu functions begin:
//----------------------------------------------------------------------


//--------------------------------------------------------------------
// Debug Screen, exit on push
//--------------------------------------------------------------------
void debug_menu(void)
{
	lcd_display_debug();				// Display Config and measured input voltages etc...

	// Exit on Button Push
	if (Status & SHORT_PUSH)
	{
		Status &=  ~SHORT_PUSH;			// Clear pushbutton status

		lcdClear();
		lcdGotoXY(0,1);				
		lcdPrintData("Nothing Changed",15);
		_delay_ms(200);
		Menu_Mode &= ~CONFIG;			// We're done, EXIT
		//Menu_Mode |= CONFIG;			// We're NOT done, just backing off
		menu_level = 0;					// We are done with this menu level
	}
}


//--------------------------------------------------------------------
// SWR Alarm Threshold Set Menu
//--------------------------------------------------------------------
void swr_alarm_threshold_menu(void)
{
	static uint8_t LCD_upd = FALSE;		// Keep track of LCD update requirements
	
	// Selection modified by encoder.  We remember last selection, even if exit and re-entry
	if (Status & ENC_CHANGE)
	{
		if ((encOutput > 0) && (R.SWR_alarm_trig < 40) )  // SWR 4:1 is MAX value, and equals no alarm
		{
			R.SWR_alarm_trig++;
		}
		if ((encOutput < 0) && (R.SWR_alarm_trig > 15) )	// SWR of 1.5:1 is MIN value for SWR alarm
		{
			R.SWR_alarm_trig--;
		}
		// Reset data from Encoder
		Status &=  ~ENC_CHANGE;
		encOutput = 0;
		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		lcdClear();
		
		lcdGotoXY(0,0);
		lcdPrintData("SWR Alarm Threshold:",20);
		lcdGotoXY(0,1);
		lcdPrintData("Adjust->   ",11);
		if (R.SWR_alarm_trig == 40)
			lcdPrintData("Off",3);		
		else
		{
			sprintf(lcd_buf,"%1u.%01u",R.SWR_alarm_trig/10, R.SWR_alarm_trig%10);
			lcdPrintData(lcd_buf,strlen(lcd_buf));
		}					
		lcdGotoXY(0,2);
		lcdPrintData("Range is 1.5 to 3.9",19);
		lcdGotoXY(0,3);
		lcdPrintData("4.0 = SWR Alarm Off",19);

	}	
	// Enact selection by saving in EEPROM
	if (Status & SHORT_PUSH)
	{
		Status &=  ~SHORT_PUSH;			// Clear pushbutton status

		lcdClear();
		lcdGotoXY(1,1);
		
		if (eeprom_read_byte(&E.SWR_alarm_trig) != R.SWR_alarm_trig)			// New Value
		{
			eeprom_write_block(&R.SWR_alarm_trig, &E.SWR_alarm_trig, sizeof (R.SWR_alarm_trig));
			lcdPrintData("Value Stored",12);
		}
		else lcdPrintData("Nothing Changed",15);

		_delay_ms(200);
		Menu_Mode &= ~CONFIG;			// We're done, EXIT
		menu_level = 0;					// We are done with this menu level
		LCD_upd = FALSE;				// Make ready for next time
	}
}


//--------------------------------------------------------------------
// SWR Alarm Power Threshold Menu
//--------------------------------------------------------------------
void swr_alarm_power_threshold_menu(void)
{
	static int8_t	current_selection;
	static uint8_t	LCD_upd = FALSE;		// Keep track of LCD update requirements
	
	// Get Current value
	// 1mW=>0, 10mW=>1, 100mW=>2, 1000mW=>3, 10000mW=>4
	// current_selection = log10(R.SWR_alarm_pwr_thresh); // This should have worked :(
	if      (R.SWR_alarm_pwr_thresh == 1) current_selection = 0;
	else if (R.SWR_alarm_pwr_thresh == 10) current_selection = 1;
	else if (R.SWR_alarm_pwr_thresh == 100) current_selection = 2;
	else if (R.SWR_alarm_pwr_thresh == 1000) current_selection = 3;
	else if (R.SWR_alarm_pwr_thresh == 10000) current_selection = 4;
	
	// Selection modified by encoder.  We remember last selection, even if exit and re-entry
	if (Status & ENC_CHANGE)
	{
		if (encOutput > 0)
		{
			current_selection++;
		}
		else if (encOutput < 0)
		{
			current_selection--;
		}
		// Reset data from Encoder
		Status &=  ~ENC_CHANGE;
		encOutput = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = swr_alarm_pwr_thresh_menu_size;
		while(current_selection >= menu_size)
			current_selection -= menu_size;
		while(current_selection < 0)
			current_selection += menu_size;

		// Update with currently selected value
		R.SWR_alarm_pwr_thresh = pow(10,current_selection);
	
		lcdClear();
		lcdGotoXY(0,0);
		lcdPrintData("SWR Alarm Power:",16);
		lcdGotoXY(0,1);
		lcdPrintData("Select",6);

		// Print the Rotary Encoder scroll Menu
		lcd_scroll_Menu((char**)swr_alarm_pwr_thresh_menu_items, menu_size, current_selection, 1, 6,1);

		lcdGotoXY(0,2);
		lcdPrintData("Available thresholds",20);
		lcdGotoXY(0,3);
		lcdPrintData("1mW 10mW 0.1W 1W 10W",20);
	}
	
	// Enact selection
	if (Status & SHORT_PUSH)
	{
		lcdClear();
		lcdGotoXY(0,1);
		
		Status &=  ~SHORT_PUSH;			// Clear pushbutton status

		// Check if selected threshold is not same as previous
		if (eeprom_read_word(&E.SWR_alarm_pwr_thresh) != R.SWR_alarm_pwr_thresh)// New Value
		{
			eeprom_write_block(&R.SWR_alarm_pwr_thresh, &E.SWR_alarm_pwr_thresh, sizeof (R.SWR_alarm_pwr_thresh));
			lcdPrintData("Value Stored",12);
		}
		else lcdPrintData("Nothing Changed",15);

		_delay_ms(200);
		Menu_Mode &= ~CONFIG;			// We're done, EXIT
		menu_level = 0;					// We are done with this menu level
		LCD_upd = FALSE;				// Make ready for next time
	}
}


//--------------------------------------------------------------------
// Peak Envelope Power (PEP) period selection Menu
//--------------------------------------------------------------------
void pep_menu(void)
{
	static int8_t	current_selection;
	static uint8_t	LCD_upd = FALSE;	// Keep track of LCD update requirements
	
	// Get Current value
	if (R.PEP_period == 500) current_selection = 1;			// 2.5 seconds
	else if (R.PEP_period == 1000) current_selection = 2;	// 5 seconds
	else current_selection = 0;								// Any other value, other than 1s, is invalid
	
	// Selection modified by encoder.  We remember last selection, even if exit and re-entry
	if (Status & ENC_CHANGE)
	{
		if (encOutput > 0)
		{
			current_selection++;
		}
		else if (encOutput < 0)
		{
			current_selection--;
		}
		// Reset data from Encoder
		Status &=  ~ENC_CHANGE;
		encOutput = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = pep_menu_size;
		while(current_selection >= menu_size)
			current_selection -= menu_size;
		while(current_selection < 0)
			current_selection += menu_size;

		if      (current_selection == 1) R.PEP_period = 500;
		else if (current_selection == 2) R.PEP_period = 1000;
		else R.PEP_period = 200;			
		
		lcdClear();
		lcdGotoXY(0,0);
		lcdPrintData("PEP sampling period:",20);
		lcdGotoXY(0,1);
		lcdPrintData("Select",6);

		// Print the Rotary Encoder scroll Menu
		lcd_scroll_Menu((char**)pep_menu_items, menu_size, current_selection, 1, 6,1);
	
		lcdGotoXY(0,2);
		lcdPrintData("Available periods",17);
		lcdGotoXY(0,3);
		lcdPrintData("1, 2.5 or 5 seconds",19);	}

	// Enact selection
	if (Status & SHORT_PUSH)
	{
		lcdClear();
		lcdGotoXY(0,1);
		
		Status &=  ~SHORT_PUSH;			// Clear pushbutton status
		
		// Check if selected threshold is not same as previous
		if (eeprom_read_word(&E.PEP_period) != R.PEP_period)
		{
			eeprom_write_block(&R.PEP_period, &E.PEP_period, sizeof (R.PEP_period));
			lcdPrintData("Value Stored",12);
		}
		else lcdPrintData("Nothing Changed",15);

		_delay_ms(200);
		Menu_Mode &= ~CONFIG;			// We're done with Menu, EXIT
		menu_level = 0;					// We are done with this menu level
		LCD_upd = FALSE;				// Make ready for next time
	}
}


//--------------------------------------------------------------------
// Screensaver Threshold Sensitivity selection Menu
//--------------------------------------------------------------------
void screenthresh_menu(void)
{
	static int8_t	current_selection;
	static uint8_t	LCD_upd = FALSE;		// Keep track of LCD update requirements
	
	// Get Current value
	// 0=0, 1uW=0.001=>1, 10uW=0.01=>2... 100uW=>3, 1mW=>4, 10mW=>5
	if (R.idle_disp_thresh == 0) current_selection = 0;
	else current_selection = log10(R.idle_disp_thresh) + 4;
	
	// Selection modified by encoder.  We remember last selection, even if exit and re-entry
	if (Status & ENC_CHANGE)
	{
		if (encOutput > 0)
		{
			current_selection++;
		}
		else if (encOutput < 0)
		{
			current_selection--;
		}
		// Reset data from Encoder
		Status &=  ~ENC_CHANGE;
		encOutput = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = screenthresh_menu_size;
		while(current_selection >= menu_size)
		current_selection -= menu_size;
		while(current_selection < 0)
		current_selection += menu_size;

		// Update with currently selected value
		// 0=0, 1uW=0.001=>1, 10uW=0.01=>2... 100uW=>3, 1mW=>4, 10mW=>5
		if (current_selection == 0) R.idle_disp_thresh = 0;
		else R.idle_disp_thresh = pow(10,current_selection - 4);
		
		lcdClear();
		lcdGotoXY(0,0);
		lcdPrintData("ScreensaverThreshld:",20);
		lcdGotoXY(0,1);
		lcdPrintData("Select",6);

		// Print the Rotary Encoder scroll Menu
		lcd_scroll_Menu((char**)screenthresh_menu_items, menu_size, current_selection, 1, 6,1);

		lcdGotoXY(0,2);
		lcdPrintData("Available thresholds",20);
		lcdGotoXY(0,3);
		lcdPrintData("1uW-10mW,  10x steps",20);
	}
	
	// Enact selection
	if (Status & SHORT_PUSH)
	{
		lcdClear();
		lcdGotoXY(0,1);
		
		Status &=  ~SHORT_PUSH;			// Clear pushbutton status

		// Check if selected threshold is not same as previous
		if (eeprom_read_float(&E.idle_disp_thresh) != R.idle_disp_thresh)// New Value
		{
			eeprom_write_block(&R.idle_disp_thresh, &E.idle_disp_thresh, sizeof (R.idle_disp_thresh));
			lcdPrintData("Value Stored",12);
		}
		else lcdPrintData("Nothing Changed",15);

		_delay_ms(200);
		Menu_Mode &= ~CONFIG;			// We're done, EXIT
		menu_level = 0;					// We are done with this menu level
		LCD_upd = FALSE;				// Make ready for next time
	}
}


//--------------------------------------------------------------------
// Scale Range Setup Submenu functions
//--------------------------------------------------------------------
void scalerange_menu_level2(void)
{
	static int16_t	current_selection;	// Keep track of current LCD menu selection
	static uint8_t LCD_upd = FALSE;		// Keep track of LCD update requirements

	uint8_t scale_set;					// Determine whether CAL_SET0, CAL_SET1 or CAL_SET2

	if (menu_level == SCALE_SET0_MENU) scale_set = 0;		// SCALE_SET0_MENU
	else if (menu_level == SCALE_SET1_MENU) scale_set = 1;	// SCALE_SET1_MENU
	else scale_set = 2;										// SCALE_SET2_MENU
	
	
	// Get Current value
	current_selection = R.ScaleRange[scale_set];

	// Selection modified by encoder.  We remember last selection, even if exit and re-entry
	if (Status & ENC_CHANGE)
	{
		if (encOutput > 0)
		{
			current_selection++;
			// Reset data from Encoder
			Status &=  ~ENC_CHANGE;
			encOutput = 0;
		}
		else if (encOutput < 0)
		{
			current_selection--;
			// Reset data from Encoder
			Status &=  ~ENC_CHANGE;
			encOutput = 0;
		}
		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}


	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;								// We are about to have serviced LCD

		// Keep Encoder Selection Within Scale Range Bounds
		int16_t max_range = R.ScaleRange[0] * 10;	// Highest permissible Scale Range for ranges 2 and 3,
		if (max_range > 99) max_range = 99;			// never larger than 10x range 1 and never larger than 99
		int16_t min_range = R.ScaleRange[0];		// Lowest permissible Scale Range for ranges 2 and 3,
													// never smaller than range 1
		if (scale_set==0)
		{
			// Set bounds for Range 1 adjustments
			if(current_selection > 99) current_selection = 99;	// Range 1 can take any value between 1 and 99
			if(current_selection < 1) current_selection =	1;			
		}
		if (scale_set>0)
		{
			// Set bounds for Range 2 and 3 adjustments
			if(current_selection > max_range) current_selection = max_range;
			if(current_selection < min_range) current_selection = min_range;
		}
		// Store Current value in running storage
		R.ScaleRange[scale_set] = current_selection;

		//
		// Bounds dependencies check and adjust
		//
		// Ranges 2 and 3 cannot ever be larger than 9.9 times Range 1
		// Range 2 is equal to or larger than Range 1
		// Range 3 is equal to or larger than Range 2
		// If two Ranges are equal, then only two Scale Ranges in effect
		// If all three Ranges are equal, then only one Range is in effect
		// If Range 1 is being adjusted, Ranges 2 and 3 can be pushed up or down as a consequence
		// If Range 2 is being adjusted up, Range 3 can be pushed up
		// If Range 3 is being adjusted down, Range 2 can be pushed down
		if (R.ScaleRange[1] >= R.ScaleRange[0]*10) R.ScaleRange[1] = R.ScaleRange[0]*10 - 1;
		if (R.ScaleRange[2] >= R.ScaleRange[0]*10) R.ScaleRange[2] = R.ScaleRange[0]*10 - 1;
		// Ranges 2 and 3 cannot be smaller than Range 1			
		if (R.ScaleRange[1] < R.ScaleRange[0]) R.ScaleRange[1] = R.ScaleRange[0];
		if (R.ScaleRange[2] < R.ScaleRange[0]) R.ScaleRange[2] = R.ScaleRange[0];

		// Adjustment up of Range 2 can push Range 3 up
		if ((scale_set == 1) && (R.ScaleRange[1] > R.ScaleRange[2])) R.ScaleRange[2] = R.ScaleRange[1];
		// Adjustment down of Range 3 can push Range 2 down:
		if ((scale_set == 2) && (R.ScaleRange[2] < R.ScaleRange[1])) R.ScaleRange[1] = R.ScaleRange[2];

		lcdClear();
		
		// Populate the Display - including current values selected for scale ranges
		lcdGotoXY(0,0);
		lcdPrintData("Adjust, Push to Set:",20);
		
		lcdGotoXY(6,1);
		sprintf(lcd_buf,"1st Range = %2u",R.ScaleRange[0]);
		lcdPrintData(lcd_buf,strlen(lcd_buf));
		lcdGotoXY(6,2);
		sprintf(lcd_buf,"2nd Range = %2u",R.ScaleRange[1]);
		lcdPrintData(lcd_buf,strlen(lcd_buf));
		lcdGotoXY(6,3);
		sprintf(lcd_buf,"3rd Range = %2u",R.ScaleRange[2]);
		lcdPrintData(lcd_buf,strlen(lcd_buf));

		// Place "===>" in front of the "ScaleRange" currently being adjusted
		lcdGotoXY(0,scale_set+1);
		lcdPrintData("===>",4);
	}
	
	// Enact selection by saving in EEPROM
	if (Status & SHORT_PUSH)
	{
		Status &=  ~SHORT_PUSH;				// Clear pushbutton status
		lcdClear();
		lcdGotoXY(1,1);

		// Save modified value
		// There are so many adjustable values that it is simplest just to assume
		// a value has always been modified.  Save all 3
		eeprom_write_block(&R.ScaleRange, &E.ScaleRange, sizeof (R.ScaleRange));
		lcdPrintData("Value Stored",12);
		_delay_ms(200);
		Menu_Mode |= CONFIG;				// We're NOT done, just backing off
		menu_level = SCALERANGE_MENU;		// We are done with this menu level
		LCD_upd = FALSE;					// Make ready for next time
	}
}



//--------------------------------------------------------------------
// Scale Range Menu functions
//--------------------------------------------------------------------
void scalerange_menu(void)
{
	static int8_t	current_selection;		// Keep track of current LCD menu selection
	static uint8_t LCD_upd = FALSE;			// Keep track of LCD update requirements

	// Selection modified by encoder.  We remember last selection, even if exit and re-entry
	if (Status & ENC_CHANGE)
	{
		if (encOutput > 0)
		{
			current_selection++;
			// Reset data from Encoder
			Status &=  ~ENC_CHANGE;
			encOutput = 0;
		}
		else if (encOutput < 0)
		{
			current_selection--;
			// Reset data from Encoder
			Status &=  ~ENC_CHANGE;
			encOutput = 0;
		}
		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;						// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = 4;
		while(current_selection >= menu_size)
		current_selection -= menu_size;
		while(current_selection < 0)
		current_selection += menu_size;

		lcdClear();
		
		// Populate the Display - including current values selected for scale ranges
		lcdGotoXY(0,0);
		if (current_selection<3)
			lcdPrintData("Select Scale Range:",19);
		else
			lcdPrintData("Turn or Push to Exit",20);
		
		lcdGotoXY(6,1);
		sprintf(lcd_buf,"1st Range = %2u",R.ScaleRange[0]);
		lcdPrintData(lcd_buf,strlen(lcd_buf));
		lcdGotoXY(6,2);
		sprintf(lcd_buf,"2nd Range = %2u",R.ScaleRange[1]);
		lcdPrintData(lcd_buf,strlen(lcd_buf));
		lcdGotoXY(6,3);
		sprintf(lcd_buf,"3rd Range = %2u",R.ScaleRange[2]);
		lcdPrintData(lcd_buf,strlen(lcd_buf));

		// Place "->" in front of the relevant "ScaleRange" to be selected with a push
		if (current_selection<3)
		{
			lcdGotoXY(4,current_selection+1);
			lcdPrintData("->",2);
		}
	}

	// Enact selection
	if (Status & SHORT_PUSH)
	{
		Status &=  ~SHORT_PUSH;				// Clear pushbutton status

		switch (current_selection)
		{
			case 0:
				menu_level = SCALE_SET0_MENU;
				LCD_upd = FALSE;			// force LCD reprint
				break;
			case 1:
				menu_level = SCALE_SET1_MENU;
				LCD_upd = FALSE;			// force LCD reprint
				break;
			case 2:
				menu_level = SCALE_SET2_MENU;
				LCD_upd = FALSE;			// force LCD reprint
				break;
			default:
				lcdClear();
				lcdGotoXY(0,1);
				lcdPrintData("Done w. Scale Ranges",20);
				_delay_ms(200);
				Menu_Mode &=  ~CONFIG;		// We're done
				menu_level = 0;				// We are done with this menu level
				LCD_upd = FALSE;			// Make ready for next time
		}
	}
}


//--------------------------------------------------------------------
// USB Serial Data output ON/OFF
//--------------------------------------------------------------------
void serial_menu(void)
{
	static int8_t	current_selection;
	static uint8_t	LCD_upd = FALSE;		// Keep track of LCD update requirements

	current_selection = R.USB_data;
	
	// Selection modified by encoder.  We remember last selection, even if exit and re-entry
	if (Status & ENC_CHANGE)
	{
		if (encOutput > 0)
		{
			current_selection++;
		}
		else if (encOutput < 0)
		{
			current_selection--;
		}
		// Reset data from Encoder
		Status &=  ~ENC_CHANGE;
		encOutput = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = serial_menu_size;
		while(current_selection >= menu_size)
			current_selection -= menu_size;
		while(current_selection < 0)
			current_selection += menu_size;

		R.USB_data = current_selection;
		
		lcdClear();
		lcdGotoXY(0,0);
		lcdPrintData("USB Virtual Com Port",20);
		lcdGotoXY(0,1);
		lcdPrintData("Select",6);

		// Print the Rotary Encoder scroll Menu
		lcd_scroll_Menu((char**)serial_menu_items, menu_size, current_selection, 1, 0,1);
	
		lcdGotoXY(0,2);
		lcdPrintData("SerialCommunications",20);
		lcdGotoXY(0,3);
		lcdPrintData("over USB      Off/On",20);
	}

	// Enact selection
	if (Status & SHORT_PUSH)
	{
		Status &=  ~SHORT_PUSH;			// Clear pushbutton status

		// Check if selected threshold is not same as previous
		if (eeprom_read_byte(&E.USB_data) != R.USB_data)
		{
			eeprom_write_block(&R.USB_data, &E.USB_data, sizeof (R.USB_data));
			lcdGotoXY(1,1);
			lcdPrintData("Value Stored",12);
			_delay_ms(200);
			lcdClear();
			lcdGotoXY(1,3);
			if (R.USB_data==0)
				lcdPrintData("Serial Data is OFF",18);
			else
				lcdPrintData("Serial Data is ON",17);
			lcdGotoXY(0,2);
			lcdPrintData("Reset Meter Now!!!",18);
			_delay_ms(200);
			asm volatile("jmp 0x00000");	// Soft Reset
			//while (1);					// Bye bye, Death by Watchdog
			//								// BUG, Watchdog is unreliable
			//								// problem related to teensy bootloader?
		}
		else
		{
			lcdClear();
			lcdGotoXY(1,0);
			lcdPrintData("Nothing Changed",15);
			lcdGotoXY(1,1);
			if (R.USB_data==0)
				lcdPrintData("Serial Data is OFF",18);
			else
				lcdPrintData("Serial Data is ON",17);
			_delay_ms(200);
			Menu_Mode &=  ~CONFIG;		// We're done
			menu_level = 0;				// We are done with this menu level
			LCD_upd = FALSE;			// Make ready for next time
		}
	}
}


/*
//--------------------------------------------------------------------
// Rotary Encoder Resolution
//--------------------------------------------------------------------
void encoder_menu(void)
{

	uint8_t	current_selection;			// Keep track of current Encoder Resolution

	static uint8_t LCD_upd = FALSE;		// Keep track of LCD update requirements

	// Get Current value
	current_selection = R.encoderRes;

	// Selection modified by encoder.  We remember last selection, even if exit and re-entry
	if (Status & ENC_CHANGE)
	{
		if (encOutput > 0)
		{
			current_selection=current_selection<<1;
	    	// Reset data from Encoder
			Status &=  ~ENC_CHANGE;
			encOutput = 0;
		}
		else if (encOutput < 0)
		{
			current_selection=current_selection>>1;
	  	  	// Reset data from Encoder
			Status &=  ~ENC_CHANGE;
			encOutput = 0;
		}
		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		if(current_selection > 128) current_selection = 128;
		if(current_selection < 1) current_selection = 1;

		// Store Current value in running storage
		R.encoderRes = current_selection;

		lcd_clrscr();
		lcd_gotoxy(0,0);	
		lcd_puts_P("Encoder ResDivide:");

		lcd_gotoxy(0,1);
		lcd_puts_P("Rotate to Adjust");
		lcd_gotoxy(0,2);
		lcd_puts_P("Push to Save");
		// Format and print current value
		lcd_gotoxy(0,3);
		lcd_puts_P("->");

		int16_t val = current_selection;
		rprintf("%3u",val);
	}

	// Enact selection by saving in EEPROM
	if (Status & SHORT_PUSH)
	{
		Status &=  ~SHORT_PUSH;			// Clear pushbutton status

		// Save modified value
		eeprom_write_block(&R.encoderRes, &E.encoderRes, sizeof (R.encoderRes));

		Status &=  ~SHORT_PUSH;			// Clear pushbutton status
		lcd_clrscr();
		lcd_gotoxy(1,1);				
		lcd_puts_P("Value Stored");
		_delay_ms(200);
		Menu_Mode |= CONFIG;			// We're NOT done, just backing off
		menu_level = 0;					// We are done with this menu level
		LCD_upd = FALSE;				// Make ready for next time
	}
}
*/

//--------------------------------------------------------------------
// Calibrate Submenu functions
//--------------------------------------------------------------------
void calibrate_menu_level2(void)
{
	static int16_t	current_selection;	// Keep track of current LCD menu selection
	static uint8_t LCD_upd = FALSE;		// Keep track of LCD update requirements

	uint8_t cal_set;					// Determine whether CAL_SET0, CAL_SET1 or CAL_SET2

	if (menu_level == CAL_SET2_MENU) cal_set = 1;	// CAL_SET2_MENU
	else cal_set = 0;								// CAL_SET0_MENU or CAL_SET1_MENU
		
	#if PHASE_DETECTOR			// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
	// Below bool indicates quality and balance of detected input "calibration signal"
	static int8_t cal_sig_quality;					// TRUE or FALSE
	#else						// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple PSWR Code
	// These defines to aid readability of code
	#define CAL_BAD	0	// Input signal of insufficient quality for calibration
	#define CAL_FWD	1	// Good input signal detected, forward direction
	#define CAL_REV	2	// Good input signal detected, reverse direction (redundant)
	// Below variable can take one of the three above defined values, based on the
	// detected input "calibration" signal
	static uint8_t cal_sig_direction_quality;
	#endif						// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
	
	// Get Current value
	current_selection = R.cal_AD[cal_set].db10m;

	// Selection modified by encoder.  We remember last selection, even if exit and re-entry
	if (Status & ENC_CHANGE)
	{
		if (encOutput > 0)
		{
			current_selection++;
	    	// Reset data from Encoder
			Status &=  ~ENC_CHANGE;
			encOutput = 0;
		}
		else if (encOutput < 0)
		{
			current_selection--;
	  	  	// Reset data from Encoder
			Status &=  ~ENC_CHANGE;
			encOutput = 0;
		}
		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	#if PHASE_DETECTOR			// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
	// Determine balance and level of calibration signal input
	// Check for imbalance between V and I inputs
	// ToDo:  Could also check phase
	if ((ABS((ad8307_adV - ad8307_adI)) > CAL_INP_BALANCE) &&
				(cal_sig_quality != FALSE))
	{
		cal_sig_quality = FALSE;
		LCD_upd = FALSE;		// Indicate that an LCD update is needed
	}
	// Check for insufficient level
	else if ((ad8307_adV <= CAL_INP_MIN_LEVEL) &&
				(cal_sig_quality != FALSE))
	{
		cal_sig_quality = FALSE;
		LCD_upd = FALSE;		// Indicate that an LCD update is needed
	}
	// If neither of the above conditions is true, then we're OK
	else if	((ad8307_adV > CAL_INP_MIN_LEVEL) &&
				(cal_sig_quality != TRUE))
	{
		cal_sig_quality = TRUE;
		LCD_upd = FALSE;		// Indicate that an LCD update is needed
	}
	#else						// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple PSWR Code
	// Determine direction and level of calibration signal input
	// Check forward direction and sufficient level
	if (((ad8307_adF - ad8307_adR) > CAL_INP_QUALITY) &&
				(cal_sig_direction_quality != CAL_FWD))
	{
		cal_sig_direction_quality = CAL_FWD;
		LCD_upd = FALSE;		// Indicate that an LCD update is needed
	}
	// Check reverse direction and sufficient level
	else if (((ad8307_adR - ad8307_adF) > CAL_INP_QUALITY) &&
				(cal_sig_direction_quality != CAL_REV))
	{
		cal_sig_direction_quality = CAL_REV;
		LCD_upd = FALSE;		// Indicate that an LCD update is needed
	}
	// Check insufficient level
	else if ((ABS((ad8307_adF - ad8307_adR)) <= CAL_INP_QUALITY) &&
				(cal_sig_direction_quality != CAL_BAD))
	{
		cal_sig_direction_quality = CAL_BAD;
		LCD_upd = FALSE;		// Indicate that an LCD update is needed
	}
	#endif						// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		int16_t max_value = 530;		// Highest permissible Calibration value in dBm * 10
		int16_t min_value = -100;		// Lowest permissible Calibration value in dBm * 10
		if(current_selection > max_value) current_selection = max_value;
		if(current_selection < min_value) current_selection = min_value;

		// Store Current value in running storage
		R.cal_AD[cal_set].db10m = current_selection;

		lcdClear();
		lcdGotoXY(0,0);	

		if (menu_level == CAL_SET0_MENU) // equals cal_set == 0
		{
			lcdPrintData("Single Level Cal:",17);
		}
		else if (menu_level == CAL_SET1_MENU) // equals cal_set == 1
		{
			lcdPrintData("First Cal SetPoint:",19);
		}
		else if (menu_level == CAL_SET2_MENU)
		{
			lcdPrintData("Second Cal SetPoint:",20);
		}

		lcdGotoXY(0,1);
		lcdPrintData("Adjust (dBm)->",14);
		// Format and print current value
		int16_t val_sub = current_selection;
		int16_t val = val_sub / 10;
		val_sub = val_sub % 10;
		if (current_selection < 0)
		{
			val*=-1;
			val_sub*=-1;
			sprintf(lcd_buf," -%1u.%01u",val, val_sub);
		}
		else
		{
			sprintf(lcd_buf," %2u.%01u",val, val_sub);
		}
		lcdPrintData(lcd_buf,strlen(lcd_buf));
		
		#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
		if (cal_sig_quality == TRUE)
		{
			lcdGotoXY(0,2);
			lcdPrintData(">Push to set<",13);
			lcdGotoXY(0,3);
			lcdPrintData("Signal detected",15);
		}
		else				// cal_sig_quality == FALSE
		{
			lcdGotoXY(0,2);
			lcdPrintData(">Push to exit<",14);
			lcdGotoXY(0,3);
			lcdPrintData("Poor signal quality",19);
		}
		#else				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple PSWR Code
		if (cal_sig_direction_quality == CAL_FWD)
		{
			lcdGotoXY(0,2);
			lcdPrintData(">Push to set<",13);
			lcdGotoXY(0,3);
			lcdPrintData("Signal detected",15);
		}
		else if (cal_sig_direction_quality == CAL_REV)
		{
			lcdGotoXY(0,2);
			lcdPrintData(">Push to set<",13);
			lcdGotoXY(0,3);
			lcdPrintData("Reverse detected",16);
		}
		else	// cal_sig_direction_quality == CAL_BAD
		{
			lcdGotoXY(0,2);
			lcdPrintData(">Push to exit<",14);
			lcdGotoXY(0,3);
			lcdPrintData("Poor signal quality",19);
		}
		#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
	}

	// Enact selection by saving in EEPROM
	if (Status & SHORT_PUSH)
	{
		Status &=  ~SHORT_PUSH;				// Clear pushbutton status
		lcdClear();
		lcdGotoXY(1,1);

		// Save modified value
		#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
		if (CAL_SET0_MENU && cal_sig_quality == TRUE)
		{
			uint16_t thirty_dB;					// Used for single shot calibration
			if (ad7991_addr) thirty_dB = 614;	// If AD7991 was detected during init
			else thirty_dB = 1200;

			R.cal_AD[0].V = ad8307_adV;
			R.cal_AD[0].I = ad8307_adI;
			eeprom_write_block(&R.cal_AD[0], &E.cal_AD[0], sizeof (R.cal_AD[0]));
			// Set second calibration point at 30 dB less, assuming 25mV per dB 
			R.cal_AD[1].db10m = R.cal_AD[0].db10m - 300;
			R.cal_AD[1].V = R.cal_AD[0].V - thirty_dB;
			R.cal_AD[1].I = R.cal_AD[0].I - thirty_dB;
			eeprom_write_block(&R.cal_AD[1], &E.cal_AD[1], sizeof (R.cal_AD[1]));
		}
		else if (cal_sig_quality == TRUE)
		{
			R.cal_AD[cal_set].V = ad8307_adV;
			R.cal_AD[cal_set].I = ad8307_adI;
			eeprom_write_block(&R.cal_AD[cal_set], &E.cal_AD[cal_set], sizeof (R.cal_AD[cal_set]));
			lcdPrintData("Value Stored",12);
		}
		else	// cal_sig_quality == FALSE
		{
			lcdPrintData("Nothing changed",15);
		}
		#else				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
		// If forward direction, then we calibrate for both, using the measured value for
		// in the forward direction only
		if (cal_sig_direction_quality == CAL_FWD)
		{
			if (CAL_SET0_MENU)
			{
				uint16_t thirty_dB;					// Used for single shot calibration
				if (ad7991_addr) thirty_dB = 1165;	// If AD7991 was detected during init
				else thirty_dB = 1200;

				R.cal_AD[0].Fwd = ad8307_adF;
				R.cal_AD[0].Rev = ad8307_adF;
				eeprom_write_block(&R.cal_AD[0], &E.cal_AD[0], sizeof (R.cal_AD[0]));
				// Set second calibration point at 30 dB less, assuming 25mV per dB
				R.cal_AD[1].db10m = R.cal_AD[0].db10m - 300;
				R.cal_AD[1].Fwd = R.cal_AD[0].Fwd - thirty_dB;
				R.cal_AD[1].Rev = R.cal_AD[0].Fwd - thirty_dB;
				eeprom_write_block(&R.cal_AD[1], &E.cal_AD[1], sizeof (R.cal_AD[1]));
			}
			else
			{
				R.cal_AD[cal_set].Fwd = ad8307_adF;
				R.cal_AD[cal_set].Rev = ad8307_adF;
				eeprom_write_block(&R.cal_AD[cal_set], &E.cal_AD[cal_set], sizeof (R.cal_AD[cal_set]));
			}
			lcdPrintData("Value Stored",12);
		}
		// If reverse, then we calibrate for reverse direction only
		else if (cal_sig_direction_quality == CAL_REV)
		{
			if (CAL_SET0_MENU)
			{
				R.cal_AD[0].Rev = ad8307_adR;
				eeprom_write_block(&R.cal_AD[0].Rev, &E.cal_AD[0].Rev, sizeof (R.cal_AD[0].Rev));
				// Set second calibration point at 30 dB less, assuming 25mV per dB
				R.cal_AD[1].Rev = R.cal_AD[0].Fwd - 1200;
				eeprom_write_block(&R.cal_AD[1].Rev, &E.cal_AD[1].Rev, sizeof (R.cal_AD[1].Rev));
			}
			else
			{
				R.cal_AD[cal_set].Rev = ad8307_adR;
				eeprom_write_block(&R.cal_AD[cal_set].Rev, &E.cal_AD[cal_set].Rev, sizeof (R.cal_AD[cal_set].Rev));
			}
			lcdPrintData("Value Stored",12);
		}
		else	// cal_sig_direction_quality == CAL_BAD
		{
			lcdPrintData("Nothing changed",15);
		}
		#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection

		_delay_ms(200);
		Menu_Mode |= CONFIG;				// We're NOT done, just backing off
		menu_level = CAL_MENU;				// We are done with this menu level
		LCD_upd = FALSE;					// Make ready for next time
	}
}



//--------------------------------------------------------------------
// Calibrate Menu functions
//--------------------------------------------------------------------
void calibrate_menu(void)
{
	static int8_t	current_selection;		// Keep track of current LCD menu selection
	static uint8_t LCD_upd = FALSE;			// Keep track of LCD update requirements

	// Selection modified by encoder.  We remember last selection, even if exit and re-entry
	if (Status & ENC_CHANGE)
	{
		if (encOutput > 0)
		{
			current_selection++;
	    	// Reset data from Encoder
			Status &=  ~ENC_CHANGE;
			encOutput = 0;
		}
		else if (encOutput < 0)
		{
			current_selection--;
	  	  	// Reset data from Encoder
			Status &=  ~ENC_CHANGE;
			encOutput = 0;
		}
		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;						// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = calibrate_menu_size;
		while(current_selection >= menu_size)
			current_selection -= menu_size;
		while(current_selection < 0)
			current_selection += menu_size;

		lcdClear();
		
		// Print the Rotary Encoder scroll Menu
		lcd_scroll_Menu((char**)calibrate_menu_items, menu_size, current_selection,1, 0,1);

		switch (current_selection)
		{
			case 0:
				lcdGotoXY(0,2);
				lcdPrintData("Calibrate using one",19);
				lcdGotoXY(6,3);
				lcdPrintData("accurate level",14);
				break;
			case 1:
				lcdGotoXY(0,2);
				lcdPrintData("Set higher of two",17);
				lcdGotoXY(5,3);
				lcdPrintData("accurate levels",15);
				break;
			case 2:
				lcdGotoXY(0,2);
				lcdPrintData("Set lower of two",16);
				lcdGotoXY(5,3);
				lcdPrintData("accurate levels",15);
				break;
		}

		// Indicate Current value stored under the currently selected GainPreset
		// The "stored" value indication changes according to which GainPreset is currently selected.
		lcdGotoXY(0,0);				
		lcdPrintData("Calibrate",9);
		if (current_selection < 2)
		{
			int16_t value=0;

			switch (current_selection)
			{
				case 0:
				case 1:
					value = R.cal_AD[0].db10m;
					break;
				case 2:
					value = R.cal_AD[1].db10m;
					break;
			}
			int16_t val_sub = value;
			int16_t val = val_sub / 10;
			val_sub = val_sub % 10;

			// Print value of currently indicated reference
			lcdGotoXY(16,0);
			if (value < 0)
			{
				val*=-1;
				val_sub*=-1;
				sprintf(lcd_buf,"-%1u.%01u",val, val_sub);
			}
			else
			{
				sprintf(lcd_buf,"%2u.%01u",val, val_sub);
			}
			lcdPrintData(lcd_buf,strlen(lcd_buf));
		}
		else
		{
			lcdGotoXY(16,0);
			lcdPrintData(" --",3);
		}
	}

	// Enact selection
	if (Status & SHORT_PUSH)
	{
		Status &=  ~SHORT_PUSH;				// Clear pushbutton status

		switch (current_selection)
		{
			case 0:
				menu_level = CAL_SET0_MENU;
				LCD_upd = FALSE;			// force LCD reprint
				break;
			case 1:
				menu_level = CAL_SET1_MENU;
				LCD_upd = FALSE;			// force LCD reprint
				break;
			case 2:
				menu_level = CAL_SET2_MENU;
				LCD_upd = FALSE;			// force LCD reprint
				break;
			case 3:
				lcdClear();
				lcdGotoXY(1,1);				
				lcdPrintData("Done w. Cal",11);
				_delay_ms(200);
				Menu_Mode |= CONFIG;		// We're NOT done, just backing off
				menu_level = 0;				// We are done with this menu level
				LCD_upd = FALSE;			// Make ready for next time
				break;
			default:
				lcdClear();
				lcdGotoXY(1,1);				
				lcdPrintData("Done w. Cal",11);
				_delay_ms(200);
				Menu_Mode &=  ~CONFIG;		// We're done
				menu_level = 0;				// We are done with this menu level
				LCD_upd = FALSE;			// Make ready for next time
				break;
		}
	}
}


//--------------------------------------------------------------------
// Factory Reset with all default values
//--------------------------------------------------------------------
void factory_menu(void)
{
	static int8_t	current_selection;
	static uint8_t	LCD_upd = FALSE;		// Keep track of LCD update requirements

	// Selection modified by encoder.  We remember last selection, even if exit and re-entry
	if (Status & ENC_CHANGE)
	{
		if (encOutput > 0)
		{
			current_selection++;
		}
		else if (encOutput < 0)
		{
			current_selection--;
		}
  	  	// Reset data from Encoder
		Status &=  ~ENC_CHANGE;
		encOutput = 0;

		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	// If LCD update is needed
	if (LCD_upd == FALSE)
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = factory_menu_size;
		while(current_selection >= menu_size)
			current_selection -= menu_size;
		while(current_selection < 0)
			current_selection += menu_size;

		lcdClear();
		lcdGotoXY(0,0);
		lcdPrintData("All to Default?",15);

		// Print the Rotary Encoder scroll Menu
		lcd_scroll_Menu((char**)factory_menu_items, menu_size, current_selection, 1, 0,1);

		lcdGotoXY(0,3);
		lcdPrintData("Factory Reset Menu",18);
	}

	// Enact selection
	if (Status & SHORT_PUSH)
	{
		Status &=  ~SHORT_PUSH;			// Clear pushbutton status

		switch (current_selection)
		{
			case 0:
				lcdClear();
				lcdGotoXY(1,1);
				lcdPrintData("Nothing Changed",15);
				_delay_ms(200);
				Menu_Mode |= CONFIG;// We're NOT done, just backing off
				menu_level = 0;		// We are done with this menu level
				LCD_upd = FALSE;	// Make ready for next time
				break;
			case 1: // Factory Reset
				// Force an EEPROM update:
				R.EEPROM_init_check = 0;		// Initialize eeprom to "factory defaults by indicating a mismatch
				eeprom_write_block(&R.EEPROM_init_check, &E.EEPROM_init_check, sizeof (R.EEPROM_init_check));
				lcdClear();
				lcdGotoXY(0,0);				
				lcdPrintData("Factory Reset",13);
				lcdGotoXY(0,1);
				lcdPrintData("All default",11);
				_delay_ms(200);
				//while (1);					// Bye bye, Death by Watchdog
				//								// BUG, Watchdog is unreliable
				//								// problem related to teensy bootloader?
				asm volatile("jmp 0x00000");	// Soft Reset
			default:
				lcdClear();
				lcdGotoXY(1,1);				
				lcdPrintData("Nothing Changed",15);
				_delay_ms(200);
				Menu_Mode &=  ~CONFIG;	// We're done
				menu_level = 0;			// We are done with this menu level
				LCD_upd = FALSE;		// Make ready for next time
				break;
		}
	}
}


//
//--------------------------------------------------------------------
// Manage the first level of Menus
//--------------------------------------------------------------------
//
void menu_level0(void)
{
	static int8_t	current_selection;	// Keep track of current LCD menu selection
	static uint8_t	LCD_upd = FALSE;	// Keep track of LCD update requirements

	// Selection modified by encoder.  We remember last selection, even if exit and re-entry
	if (Status & ENC_CHANGE)
	{
		if (encOutput > 0)
		{
			current_selection++;
	    	// Reset data from Encoder
			Status &=  ~ENC_CHANGE;
			encOutput = 0;
		}
		else if (encOutput < 0)
		{
			current_selection--;
	  	  	// Reset data from Encoder
			Status &=  ~ENC_CHANGE;
			encOutput = 0;
		}
		// Indicate that an LCD update is needed
		LCD_upd = FALSE;
	}

	if (LCD_upd == FALSE)				// Need to update LCD
	{
		LCD_upd = TRUE;					// We have serviced LCD

		// Keep Encoder Selection Within Bounds of the Menu Size
		uint8_t menu_size = level0_menu_size;
		while(current_selection >= menu_size)
			current_selection -= menu_size;
		while(current_selection < 0)
			current_selection += menu_size;

		lcdClear();
		lcdGotoXY(0,0);
		lcdPrintData("Config Menu:",12);

		// Print the Menu
		lcd_scroll_Menu((char**)level0_menu_items, menu_size, current_selection,1, 0,3);
	}

	if (Status & SHORT_PUSH)
	{
		Status &= ~SHORT_PUSH;			// Clear pushbutton status

		switch (current_selection)
		{
			case 0: // SWR Alarm Threshold Set
				menu_level = SWR_ALARM_THRESHOLD;
				LCD_upd = FALSE;		// force LCD reprint
				break;

			case 1: // SWR Alarm Power Threshold Set
				menu_level = SWR_ALARM_PWR_THRESHOLD;
				LCD_upd = FALSE;		// force LCD reprint
				break;

			case 2: // PEP sampling period select
				menu_level = PEP_MENU;
				LCD_upd = FALSE;		// force LCD reprint
				break;

			case 3: // Screensaver Threshold
				menu_level = SCREENTHRESH_MENU;
				LCD_upd = FALSE;		// force LCD reprint
				break;

			case 4: // Scale Range Set
				menu_level = SCALERANGE_MENU;
				LCD_upd = FALSE;		// force LCD reprint
				break;

			case 5: // Serial Data
				menu_level = SERIAL_MENU;
				LCD_upd = FALSE;		// force LCD reprint
				break;

			case 6: // Calibrate
				menu_level = CAL_MENU;
				LCD_upd = FALSE;		// force LCD reprint
				break;

			case 7: // Display Debug stuff
				menu_level = DEBUG_MENU;
				LCD_upd = FALSE;		// force LCD reprint
				break;

			case 8: // Factory Reset
				menu_level = FACTORY_MENU;
				LCD_upd = FALSE;		// force LCD reprint
				break;
			
			//case x:// Encoder Resolution
			//	menu_level = ENCODER_MENU;
			//	LCD_upd = FALSE;		// force LCD reprint
			//	break;

			default:
				// Exit
				lcdClear();
				lcdGotoXY(1,1);
		   		lcdPrintData("Return from Menu",16);
				Menu_Mode &=  ~CONFIG;	// We're done
				LCD_upd = FALSE;		// Make ready for next time
		}
	}
}


//
//--------------------------------------------------------------------
// Scan the Configuraton Menu Status and delegate tasks accordingly
//--------------------------------------------------------------------
//
void PushButtonMenu(void)
{
	// Select which menu level to manage
	if (menu_level == 0) menu_level0();

	else if (menu_level == SWR_ALARM_THRESHOLD) swr_alarm_threshold_menu();
	else if (menu_level == SWR_ALARM_PWR_THRESHOLD) swr_alarm_power_threshold_menu();

	else if (menu_level == PEP_MENU) pep_menu();

	else if (menu_level == SCREENTHRESH_MENU) screenthresh_menu();

	else if (menu_level == SCALERANGE_MENU) scalerange_menu();
	else if (menu_level == SCALE_SET0_MENU) scalerange_menu_level2();
	else if (menu_level == SCALE_SET1_MENU) scalerange_menu_level2();
	else if (menu_level == SCALE_SET2_MENU) scalerange_menu_level2();

	else if (menu_level == SERIAL_MENU) serial_menu();

	//else if (menu_level == ENCODER_MENU) encoder_menu();
	
	else if (menu_level == CAL_MENU) calibrate_menu();
	else if (menu_level == CAL_SET0_MENU) calibrate_menu_level2();
	else if (menu_level == CAL_SET1_MENU) calibrate_menu_level2();
	else if (menu_level == CAL_SET2_MENU) calibrate_menu_level2();

	else if (menu_level == DEBUG_MENU) debug_menu();

	else if (menu_level == FACTORY_MENU) factory_menu();
}
