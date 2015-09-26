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
//** Current version.: See PM.h
//**
//** History.........: 2013-09-29 First beta release. 
//**                   A reasonably decent and fully functional Power/SWR Meter
//**                   version.
//**                   Power/Impedance Meter version needs further refinement.
//**
//*********************************************************************************

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Microcontroller Pin assignment (defined in the PM.h file):
//
// ------LCD Display------
// PC0 = LCD control RS
// PC1 = LCD control RW
// PC2 = LCD control E
// PC3
// PC4 = LCD D4
// PC5 = LCD D5
// PC6 = LCD D6
// PC7 = LCD D7
//
// ------Rotary Encoder and Push Button------
// PB0 = Push Button Selector input
// PE6 = Rotary Encoder A input
// PE7 = Rotary Encoder B input
//
// ------Status and Alarm Indicators------
// PE0 = RED LED output, SWR Alarm = High
// PE1 = GREEN LED output, Serial Data Out = High
// PD6 = Teensy_2++ LED, used for timing measurements during coding
//
// ------Analog Inputs for Meter------
// These two used for Simple PWR/SWR Meter
// PF0 = AD MUX0, forward sensor input from AD8307
// PF1 = AD MUX0, reverse sensor input from AD8307
//
// These four used for Power and Impedance Meter:
// PF0 = AD MUX2, voltage sensor input from AD8307
// PF1 = AD MUX3, current sensor input from AD8307
// PF2 = AD MUX0, positive phase sense (U-pin), MCK12140
// PF3 = AD MUX1, negative phase sense (D-pin), MCK12140
//	OR, if I2C comms with an AD7991 4xADC chip:
// PD0 = SCL
// PD1 - SDA
// AD7991 channel 1: voltage sensor input from AD8307
// AD7991 channel 2: current sensor input from AD8307
// AD7991 channel 3: positive phase sense (U-pin), MCK12140
// AD7991 channel 4: negative phase sense, (D-pin) MCK12140
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#include "PM.h"
#include <math.h>

EEMEM		var_t	E;							// Variables in eeprom (user modifiable, stored)
			var_t	R							// Variables in ram/flash rom (default)
					=
					{
					#if PHASE_DETECTOR			// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
						{ PH_U_90_DEG_AVG,		// Phase voltage at  90 degrees, U pin.
						  PH_U_0_DEG_AVG,		// Phase voltage at   0 degrees, U pin.
						  PH_U_270DEG_AVG },	// Phase voltage at -90 degrees, U pin.
						{ PH_D_90_DEG_AVG,		// Phase voltage at  90 degrees, D pin.
						  PH_D_0_DEG_AVG,		// Phase voltage at   0 degrees, D pin.
						  PH_D_270DEG_AVG},		// Phase voltage at -90 degrees, D pin.
					#endif						// >>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection								
						COLDSTART_REF			// Update into eeprom if value mismatch
					, 	ENC_RES_DIVIDE			// Initial Encoder Resolution
					#if PHASE_DETECTOR			// >>>>>>>>>>>>>>> Power & Phase Detector Code
					,{{ CAL1_NOR_VALUE
					   ,CALV1_RAW_DEFAULT
					   ,CALI1_RAW_DEFAULT}		// First Calibrate point, db*10 + 2 x AD values
					, { CAL2_NOR_VALUE
					   ,CALV2_RAW_DEFAULT
					   ,CALI2_RAW_DEFAULT}}		// Second Calibrate point, db*10 + 2 x AD values
					 #else						// >>>>>>>>>>>>>>> Simple Power & SWR Code
 					, {{ CAL1_NOR_VALUE
 					,    CALFWD1_RAW_DEFAULT	// First Calibrate point, Forward direction, db*10 + 2 x AD values					 
 					,    CALREV1_RAW_DEFAULT}	// First Calibrate point, Reverse direction, db*10 + 2 x AD values
 					,  { CAL2_NOR_VALUE
 					,    CALFWD2_RAW_DEFAULT	// Second Calibrate point, Forward direction, db*10 + 2 x AD values
	 				,    CALREV2_RAW_DEFAULT}}	// Second Calibrate point, Reverse direction, db*10 + 2 x AD values
					 #endif						// >>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
					,	USB_DATA				// Default USB Serial Data out setting
					,	SWR_ALARM				// Default SWR Alarm trigger, defined in PM.h
					,	SWR_THRESHOLD			// Default SWR Alarm power threshold defined in PM.h
					,	0						// Bool USB_Flags, indicating which output command was requested last time
												// and whether Continuous Mode

					,	PEP_PERIOD				// PEP envelope sampling time in 5ms increments (200=1s, 500=2.5s, 1000=5s)
					, { SCALE_RANGE1			// User definable Scale Ranges, up to 3 ranges per decade
					,   SCALE_RANGE2			// e.g. ... 6W 12W 24W 60W 120W 240W ...
					,   SCALE_RANGE3 }			// If all values set as "2", then ... 2W 20W 200W ...
					,	SLEEPMSG				// Shown when nothing else to display on LCD
												// Configurable by USB Serial input command:
												// $sleepmsg="blabla"
					, SLEEPTHRESHOLD			// Minimum relevant power to exit Sleep Display (0.001=1uW),
												// valid values are 0, 0.001, 0.01, 0.1, 1 and 10
					};

uint16_t	Status = 0;						// Contains various status flags
uint16_t	Menu_Mode;						// Menu Mode Flags

#if PHASE_DETECTOR							// >>>>>>>>>>>>>>> Power & Phase Detector Code
int16_t		ad8307_adV;						// Measured A/D value from the AD8307 Voltage sensor as a 10 or 12 bit value
int16_t		ad8307_adI;						// Measured A/D value from the AD8307 Current sensor as a 10 or 12 bit value
int16_t		mck12140pos;					// Measured A/D value from the MCK12140 Phase Sensor, positive direction pin
int16_t		mck12140neg;					// Measured A/D value from the MCK12140 Phase Sensor, negative direction pin
double		ad8307_VdBm;					// Measured AD8307 voltage in dBm
double		ad8307_IdBm;					// Measured AD8307 current in dBm
double		imp_R;							// Resistive component of the measured impedance (normalized, multiply by 50)
double		imp_jX;							// Reactive component of the measured impedance (normalized, multiply by 50)
double		phase;							// Radians
double		Gamma;							// Complex number describing both magnitude and phase of reflection

#else										// >>>>>>>>>>>>>>> Simple Power & SWR Code
int16_t		ad8307_adF;						// Measured A/D value from the AD8307 Forward sensor as a 10 bit value
int16_t		ad8307_adR;						// Measured A/D value from the AD8307 Reverse sensor as a 10 bit value
double		ad8307_FdBm;					// Measured AD8307 forward voltage in dBm
double		ad8307_RdBm;					// Measured AD8307 reverse current in dBm

#endif										// >>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection

BOOL		Reverse;						// True if reverse power is greater than forward power
double		fwd_power_db;					// Calculated forward power in dBm
double		ref_power_db;					// Calculated reflected power in dBm
double		power_db;						// Calculated power in dBm
double		power_db_pep;					// Calculated PEP power in dBm
double		power_db_pk;					// Calculated 100ms peak power in dBm
double		power_db_avg;					// Calculated AVG power in dBm

double		fwd_power_mw;					// Calculated forward power in mW
double		ref_power_mw;					// Calculated reflected power in mW
double		power_mw;						// Calculated power in mW
double		power_mw_pep;					// Calculated PEP power in mW
double		power_mw_pk;					// Calculated 100ms peak power in mW
double		power_mw_avg;					// Calculated AVG power in mW
//double	modulation_index;				// Modulation index
double		swr=1.0;						// SWR as an absolute value
uint16_t	swr_bar;						// logarithmic SWR value for bargraph, 1000 equals SWR of 10:1

char lcd_buf[80];							// Used to process data to be passed to LCD and USB Serial				



//
//-----------------------------------------------------------------------------------------
// Top level task
// runs in an endless loop
//-----------------------------------------------------------------------------------------
//
void maintask(void)
{
	// Now we can do all kinds of business, such as measuring the AD8307 voltage output, 
	// scanning Rotary Encoder, updating LCD etc...
	static uint16_t lastIteration, lastIteration1, lastIteration2;	// Counters to keep track of time
	uint16_t Timer1val, Timer1val1, Timer1val2;		// Timers used for 100ms and 10ms polls
	static uint8_t pushcount=0;						// Measure push button time (max 2.5s)
	
	//-------------------------------------------------------------------------------
	// Here we do routines which are to be run through as often as possible
	// currently measured to be approximately once every 25 - 50 us
	//-------------------------------------------------------------------------------
	#if ENCODER_SCAN_STYLE							// On the other hand, if interrupt driven,
													// then no need to scan
	encoder_Scan();									// Scan the Rotary Encoder
	#endif
	
	#if FAST_LOOP_THRU_LED							// Blink a LED every time when going through the main loop 
	LED_PORT = LED_PORT ^ LED;						// Blink a led
	#endif
	
	//-------------------------------------------------------------------------------
	// Here we do routines which are to be accessed once every approx 5ms
	// We have a free running timer which matures once every ~1.05 seconds
	//-------------------------------------------------------------------------------
	//Timer1val1 = TCNT1/328; // get current Timer1 value, changeable every ~5ms
	Timer1val1 = TCNT1/313;   // get current Timer1 value, changeable every ~5ms
	
	if (Timer1val1 != lastIteration1)				// Once every 5ms, do stuff
	{
		lastIteration1 = Timer1val1;				// Make ready for next iteration
		#if MS_LOOP_THRU_LED						// Blink LED every 5*2 ms, when going through the main loop 
		LED_PORT = LED_PORT ^ LED;  				// Blink a led
		#endif

		adc_poll();									// Read AD, external or internal
		#if PHASE_DETECTOR							// >>>>>>>>>>>>>>> Power & Phase Detector Code
		imp_determine_dBm();						// Convert raw A/D values to dBm
		imp_determine_phase();						// Process Phase Detector outputs
		imp_calc_Power();							// Calculate all kinds of Power
		#else										// >>>>>>>>>>>>>>> Simple Power & SWR Code
		pswr_determine_dBm();						// Convert raw A/D values to dBm
		pswr_calc_Power();							// Calculate all kinds of Power
		#endif										// >>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
	}

	//-------------------------------------------------------------------------------
	// Here we do routines which are to be accessed once every 1/100th of a second (10ms)
	// We have a free running timer which matures once every ~1.05 seconds
	//-------------------------------------------------------------------------------
	//Timer1val2 = TCNT1/656; // get current Timer1 value, changeable every ~1/100th sec
	Timer1val2 = TCNT1/626;   // get current Timer1 value, changeable every ~1/100th sec
	if (Timer1val2 != lastIteration2)				// Once every 1/100th of a second, do stuff
	{
		lastIteration2 = Timer1val2;				// Make ready for next iteration

		#if MED_LOOP_THRU_LED						// Blink LED every 10ms, when going through the main loop 
		LED_PORT = LED_PORT ^ LED;  				// Blink a led
		#endif
		
		// Nothing here
	}

	//-------------------------------------------------------------------------------
	// Here we do routines which are to be accessed once every 1/10th of a second
	// We have a free running timer which matures once every ~1.05 seconds
	//-------------------------------------------------------------------------------
	//Timer1val = TCNT1/6554; // get current Timer1 value, changeable every ~1/10th sec
	Timer1val = TCNT1/6253;   // get current Timer1 value, changeable every ~1/10th sec
	if (Timer1val != lastIteration)	// Once every 1/10th of a second, do stuff
	{
		lastIteration = Timer1val;					// Make ready for next iteration

		#if SLOW_LOOP_THRU_LED						// Blink LED every 100ms, when going through the main loop 
		LED_PORT = LED_PORT ^ LED;					// Blink a led
		#endif
		//EXTLED_PORT = EXTLED_PORT ^ EXT_G_LED;	// Blink a led
		//EXTLED_PORT = EXTLED_PORT ^ EXT_R_LED;	// Blink a led

		//-------------------------------------------------------------------
		// Read Encoder to cycle back and forth through modes
		//
		static int8_t current_mode = 0;			// Which display mode is active?

		#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
		#define MAX_MODES 7						// Number of available modes minus one
		#else				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
		#define MAX_MODES 4						// Number of available modes minus one
		#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
		
		// If the encoder was used while not in config mode:
		if ((!(Menu_Mode & CONFIG)) && (Status & ENC_CHANGE))
		{
			// Mode switching travels only one click at a time, ignoring extra clicks
			if (encOutput > 0)
			{
				current_mode++;
				if (current_mode > MAX_MODES) current_mode = 0;
	    		// Reset data from Encoder
				Status &=  ~ENC_CHANGE;
				encOutput = 0;
			}
			else if (encOutput < 0)
			{
				current_mode--;
				if (current_mode < 0) current_mode = MAX_MODES;	
	  		  	// Reset data from Encoder
				Status &=  ~ENC_CHANGE;
				encOutput = 0;
			}

			switch (current_mode)
			{
				#if PHASE_DETECTOR	// >>>>>>>>>>>>>>> Power & Phase Detector Code		
				case 0:
					Menu_Mode = POWER_RJX_BARPK;
					break;
				case 1:
					Menu_Mode = POWER_RJX_BARAVG;
					break;
				case 2:
					Menu_Mode = POWER_RJX_BARINST;
					break;
				case 3:
					Menu_Mode = POWER_BARPK;
					break;
				case 4:
					Menu_Mode = POWER_BARAVG;
					break;
				case 5:
					Menu_Mode = POWER_BARINST;
					break;
				case 6:
					Menu_Mode = POWER_CLEAN_DBM;
					break;
				case 7:
					Menu_Mode = POWER_MIXED;
					break;
				
				#else				// >>>>>>>>>>>>>>> Simple Power & SWR Code
				case 0:
					Menu_Mode = POWER_BARPK;
					break;
				case 1:
					Menu_Mode = POWER_BARAVG;
					break;
				case 2:
					Menu_Mode = POWER_BARINST;
					break;
				case 3:
					Menu_Mode = POWER_CLEAN_DBM;
					break;
				case 4:
					Menu_Mode = POWER_MIXED;
					break;
				#endif				// >>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
			}

			// Force Mode Intro Display whenever Mode has been changed
			Status |= MODE_CHANGE | MODE_DISPLAY;
		}
		
		//-------------------------------------------------------------------
		// Read Pushbutton state
		//
		// Enact Long Push (pushbutton has been held down for a certain length of time):
		if (pushcount >= ENC_PUSHB_MAX)				// "Long Push", goto Configuration Mode
		{
			Menu_Mode |= CONFIG;					// Switch into Configuration Menu, while
													// retaining memory of runtime function

			Status |= LONG_PUSH;					// Used with Configuration Menu functions	
			pushcount = 0;							// Initialize push counter for next time
		}
		// Enact Short Push (react on release if only short push)
		else if (ENC_PUSHB_INPORT & ENC_PUSHB_PIN) 	// Pin high = just released, or not pushed
		{
			// Do nothing if this is a release after Long Push
			if (Status & LONG_PUSH)					// Is this a release following a long push?
			{
					Status &= ~LONG_PUSH;			// Clear pushbutton status
			}
			// Do stuff on command
			else if (pushcount >= ENC_PUSHB_MIN)	// Check if this is more than a short spike
			{	
				if (Menu_Mode & CONFIG)
					Status |= SHORT_PUSH;			// Used with Configuration Menu functions	

				else
				{
					//
					// Various things to be done if short push... depending on which mode is active
					//
					// Nothing much here so far...
					EXTLED_PORT &= ~EXT_R_LED;		// Clear SWR Alarm LED
					Status &= ~SWR_FLAG;			// Clear SWR Alarm Flag
				} 
			}
			pushcount = 0;							// Initialize push counter for next time
		}
		else if (!(Status & LONG_PUSH))				// Button Pushed, count up the push timer
		{											// (unless this is tail end of a long push,
			pushcount++;							//  then do nothing)
		}

		//-------------------------------------------------------------------
		// Various Menu (rotary encoder) selectable display/function modes
		//
		if (Menu_Mode & CONFIG)					// Pushbutton Configuration Menu
		{
			PushButtonMenu();
		}	
		#if PHASE_DETECTOR						// >>>>>>>>>>>>>>> Power & Phase Detector Code
		else if (Menu_Mode == POWER_RJX_BARPK)	// Power Meter, Bargraph, PWR, SWR, PEP & R+jX
		{
			lcd_display_rjx("100ms Peak Power", power_mw_pk);
		}
		else if (Menu_Mode == POWER_RJX_BARAVG)	// Power Meter, Bargraph, PWR, SWR, PEP & R+jX
		{
			lcd_display_rjx("1 sec Average Power", power_mw_avg);
		}
		else if (Menu_Mode == POWER_RJX_BARINST)// Power Meter, Bargraph, PWR, SWR, PEP & R+jX
		{
			lcd_display_rjx("Instantaneous Power", power_mw);
		}
		#endif									// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
		else if (Menu_Mode == POWER_BARPK)		// 100ms Peak Power, Bargraph, PWR, SWR, PEP
		{
			lcd_display_clean("100ms Peak Power", "pk ", power_mw_pk);
		}
		else if (Menu_Mode == POWER_BARAVG)		// 1s Average Power, Bargraph, PWR, SWR, PEP
		{
			lcd_display_clean("1 Sec Average Power", "avg", power_mw_avg);
		}
		else if (Menu_Mode == POWER_BARINST)	// Instantaneous Power, Bargraph, PWR, SWR, PEP
		{
			lcd_display_clean("Instantaneous Power", "   ", power_mw);
		}
		else if (Menu_Mode == POWER_CLEAN_DBM)	// Power Meter in dBm
		{
			lcd_display_clean_dBm();
		}
		else if (Menu_Mode == POWER_MIXED)		// Fwd and Reflected, [R+jX and Phase if Impedance Meter] etc...
		{
			lcd_display_mixed();
		}

		if (R.USB_data && (Status & USB_AVAILABLE))	// Handle USB serial port, if enabled and available
		{
			// If Continuous USB Send mode is selected, then send data every 100ms to computer
			// Only one of these is selected at any time
			if (R.USB_Flags & USBPCONT)
			{
				if (R.USB_Flags & USBPPOLL) usb_poll_data();	// Machine readable data
				else if (R.USB_Flags & USBP_DB)					// We want decibels
				{
					if (R.USB_Flags & USBPINST) usb_poll_instdb();		// Inst power, dB
					else if (R.USB_Flags & USBPPK)  usb_poll_pkdb();	// peak, dB
					else if (R.USB_Flags & USBPPEP) usb_poll_pepdb();	// PEP, dB
					else if (R.USB_Flags & USBPAVG) usb_poll_avgdb();	// avg, dB
				}
				else if (R.USB_Flags & USBPINST) usb_poll_inst();		// Inst power
				else if (R.USB_Flags & USBPPK )  usb_poll_pk();			// peak
				else if (R.USB_Flags & USBPPEP ) usb_poll_pep();		// PEP
				else if (R.USB_Flags & USBPAVG ) usb_poll_avg();		// avg
				
				else if (R.USB_Flags & USBPLONG) usb_poll_long();		// Verbose message
			}
		}		
	}
	//wdt_reset();								// Whoops... must remember to reset that running watchdog
}


//
//-----------------------------------------------------------------------------------------
// 			Setup Ports, timers, start the works and never return, unless reset
//								by the watchdog timer
//						then - do everything, all over again
//-----------------------------------------------------------------------------------------
//
int main(void)
{
	MCUSR &= ~(1 << WDRF);							// Disable watchdog if enabled by bootloader/fuses
	wdt_disable();

	clock_prescale_set(clock_div_1); 				// with 16MHz crystal this means CLK=16000000

	//------------------------------------------
	// 16-bit Timer1 Initialization
	TCCR1A = 0; //start the timer
	TCCR1B = (1 << CS12); // prescale Timer1 by CLK/256
	// 16000000 Hz / 256 = 62500 ticks per second
	// 16-bit = 2^16 = 65536 maximum ticks for Timer1
	// 65536 / 62500 = ~1.05 seconds
	// so Timer1 will overflow back to 0 about every 1 seconds
	// Timer1val = TCNT1; // get current Timer1 value

	//------------------------------------------
	// Init and set output for LEDS
	LED_DDR = LED;
	LED_PORT = 0;
	
	EXTLED_DDR = EXT_G_LED | EXT_R_LED;				// Init Green and Red LEDs
	EXTLED_PORT = 0;
	
	//------------------------------------------
	// Init Pushbutton input
	ENC_PUSHB_DDR = ENC_PUSHB_DDR & ~ENC_PUSHB_PIN;	// Set pin for input
	ENC_PUSHB_PORT= ENC_PUSHB_PORT | ENC_PUSHB_PIN;	// Set pull up

	//------------------------------------------
	// Set run time parameters to Factory default under certain conditions
	//
	// Enforce "Factory default settings" when firmware is run for the very first time after
	// a fresh firmware installation with a new "serial number" in the COLDSTART_REF #define
	// This may be necessary if there is garbage in the EEPROM, preventing startup
	// To activate, roll "COLDSTART_REF" Serial Number in the PM.h file
	if (eeprom_read_byte(&E.EEPROM_init_check) != R.EEPROM_init_check)
	{
		eeprom_write_block(&R, &E, sizeof(E));		// Initialize eeprom to "factory defaults".
	}
	else
	{
		eeprom_read_block(&R, &E, sizeof(E));		// Load the persistent data from eeprom
	}

   	uint8_t i2c_status = I2C_Init();				// Initialize I2C comms
   	
	lcd_Init();										// Init the LCD

	// Initialize the LCD bargraph, load the bargraph custom characters
	lcd_bargraph_Init();

	//------------------------------------------
	// LCD Print Version and I2C information (6 seconds in total during startup)
	lcdClear();
	lcdGotoXY(0,0);
	lcdPrintData(STARTUPDISPLAY1,strlen(STARTUPDISPLAY1));
	lcdGotoXY(0,1);
	lcdPrintData(STARTUPDISPLAY2,strlen(STARTUPDISPLAY2));
	_delay_ms(300);
	lcdGotoXY(20-strlen(STARTUPDISPLAY3),1);
	lcdPrintData(STARTUPDISPLAY3,strlen(STARTUPDISPLAY3));
	_delay_ms(200);
	lcdGotoXY(20-strlen(STARTUPDISPLAY4),2);
	lcdPrintData(STARTUPDISPLAY4,strlen(STARTUPDISPLAY4));
	_delay_ms(2500);

	lcdGotoXY(0,3);
	lcdPrintData(STARTUPDISPLAY5,strlen(STARTUPDISPLAY5));
	sprintf(lcd_buf,"V%s", VERSION);
	lcdGotoXY(20-strlen(lcd_buf),3);
	lcdPrintData(lcd_buf, strlen(lcd_buf));
	_delay_ms(2000);

	lcdGotoXY(0,3);
	if (i2c_status==1) lcdPrintData("AD7991-0 detected   ",20);
	else if (i2c_status==2) lcdPrintData("AD7991-1 detected   ",20);
	else lcdPrintData("Using built-in A/D  ",20);	// No I2C device detected, 
													// we will be using the builtin 10 bit ADs
													
	if (R.USB_data)									// Enumerate USB serial port, if USB Serial Data enabled
	{
		usb_init();									// Initialize USB communications
		Status&=~USB_AVAILABLE;						// Disable USB communications until checked if actually available
	}
	
	_delay_ms(1000);	
		
	//wdt_enable(WDTO_1S);							// Start the Watchdog Timer, 1 second
	
	encoder_Init();									// Init Rotary encoder

	Menu_Mode = DEFAULT_MODE;						// Power Meter Mode is normal default
	
	Status |= MODE_CHANGE | MODE_DISPLAY;			// Force a Display of Mode Intro when starting up
	
	// Start the works, we're in business
	while (1)
	{
		maintask();									// Do useful stuff
		
		if (R.USB_data)								// Do the below if USB Port has been enabled
		{
			// If USB port is available and not busy, then use it - otherwise mark it as blocked.
			if (usb_configured() && (usb_serial_get_control() & USB_SERIAL_DTR))
			{
				Status |= USB_AVAILABLE;			// Enable USB communications
				EXTLED_PORT |= EXT_G_LED;			// Turn Green LED On
				usb_read_serial();
			}
			else
			{
				Status&=~USB_AVAILABLE;				// Clear USB Available Flag to disable USB communications
				EXTLED_PORT &= ~EXT_G_LED;			// Turn Green LED off, if previously on
			}			
		}
	}
}
