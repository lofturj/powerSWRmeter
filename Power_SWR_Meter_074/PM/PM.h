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
#define                VERSION "0.74"
#define                DATE    "2014-05-08"
//**
//** History.........: Check the PM.c file
//**
//*********************************************************************************

#ifndef _TF3LJ_PM_H_
#define _TF3LJ_PM_H_ 1

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/power.h>
#include <lcd.h>

#include "usb_serial.h"	


//
//-----------------------------------------------------------------------------
// Features Selection
//-----------------------------------------------------------------------------  
//

// EEPROM settings Serial Number. Increment this number when firmware mods necessitate
// fresh "Factory Default Settings" to be forced into the EEPROM at first boot after
// an upgrade
#define COLDSTART_REF		0x71	// When started, the firmware examines this "Serial Number
									// and enforce factory reset if there is a mismatch.
									// This is useful if the EEPROM structure has been modified

//-----------------------------------------------------------------------------
// Simple 2xAD8307 Power & SWR Meter or also including the MCK12240 Phase detector
#define PHASE_DETECTOR		0			

//-----------------------------------------------------------------------------
// 20 to 1 Tandem Match with Power and SWR Meter - not used with Phase Detector code
// (default defines are for a 30 to 1 Tandem Match)
#define TWENTYTOONE			0

//-----------------------------------------------------------------------------
//Text for Startup Display
#if		PHASE_DETECTOR
#define STARTUPDISPLAY1		"Power and Impedance"
#else
#define STARTUPDISPLAY1		"AD8307 Power & SWR"
#endif
#define STARTUPDISPLAY2		"Meter"
#define STARTUPDISPLAY3		"milliwatts"
#define STARTUPDISPLAY4		"to kilowatts"
#define STARTUPDISPLAY5		"TF3LJ / VE2LJX"

								
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Various Initial Default settings for Meter
// (many of these are configurable by user through Config Menu or USB commands)
//
//-----------------------------------------------------------------------------
// PEP envelope sampling time in 5ms increments (200 = 1 second)
#define PEP_PERIOD			500			// 200, 500 or 1000, (1s, 2.5 or 5s)
//-----------------------------------------------------------------------------
// DEFS for SWR Alarm
#define SWR_ALARM			 30			// 30=3.0, default SWR Alarm Trigger
#define SWR_THRESHOLD		 10			// Default SWR Alarm Power Threshold in mW
//-----------------------------------------------------------------------------
// DEFS for User Definable Scale Ranges
#define SCALE_RANGE1		  6			// User definable Scale Ranges, up to 3 ranges per decade                   
#define SCALE_RANGE2		 12			// e.g. ... 6W 12W 24W 60W 120W 240W ...
#define SCALE_RANGE3		 24			// If all values set as "2", then ... 2W 20W 200W ...
//-----------------------------------------------------------------------------
// DEFS for Default Screensaver Message
#define SLEEPMSG	"ZZzzzz zzz ..."	// Shown when nothing else to display on LCD
										// Configurable by USB Serial input command:
										// $sleepmsg="blabla"
#define SLEEPTIME			  50		// Time to renew position of screensaver message
										// in tenths of a second
#if		PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
#define SLEEPTHRESHOLD		 0.1		// Minimum Power (mW) to exit Sleep Display
#else					// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
#define SLEEPTHRESHOLD	   0.001		// Minimum relevant power to exit Sleep Display (0.001=1uW)
#endif					// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
//-----------------------------------------------------------------------------
// Mode Intro Time (decides for how long mode intro is displayed when turning encoder
#define MODE_INTRO_TIME		  10		// Tenths of a second (10 equals 1s)
//-----------------------------------------------------------------------------
// USB Serial data out on or off
//#define USB_DATA			FALSE		// Default is USB Serial Port is disable
#define USB_DATA			TRUE		// Default is USB Serial Port is enabled
//-----------------------------------------------------------------------------
// Defs for Power and SWR indication
#if		PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
#define MIN_PWR_FOR_SWR_CALC 100.0	// Minimum Power in mW for SWR calculation and display
#define MIN_PWR_FOR_SWR_SHOW 0.01	// Minimum Power in mW for SWR indication (use recent value)
#else					// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
#define MIN_PWR_FOR_SWR_CALC 1.0	// Minimum Power in mW for SWR calculation and display
#define MIN_PWR_FOR_SWR_SHOW 0.01	// Minimum Power in mW for SWR indication (use recent value)
#endif					// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
//-----------------------------------------------------------------------------
// DEFS for AD8307 Calibration (dBm *10)
#if		PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
#define	CAL1_NOR_VALUE		 401	// 40 dBm, default dBm level1 for both AD8307
#define	CALV1_RAW_DEFAULT	 1666	// Default raw Voltage level1  for Voltage AD8307 at 40.0 dBm
#define	CALI1_RAW_DEFAULT	 1666	// Default raw Voltage level1  for Current AD8307 at 40.0 dBm
#define	CAL2_NOR_VALUE		 101	// 10 dBm, default dBm level2 for both AD8307
#define	CALV2_RAW_DEFAULT	 1052	// Default raw Voltage level2 for Voltage AD8307 at 10.0 dBm
#define	CALI2_RAW_DEFAULT	 1052	// Default raw Voltage level2 for Current AD8307 at 10.0 dBm
#define CAL_INP_MIN_LEVEL	 400	// Minimum raw Voltage level to allow calibration
#define CAL_INP_BALANCE		  50	// Maximum imbalance between the raw V & I levels to allow calibration
#else					// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
#if TWENTYTOONE						// Defs when using a 20 to 1 coupler and AD7991
#define	CAL1_NOR_VALUE		 400	// 40 dBm, default dBm level1 for both AD8307
#define	CAL2_NOR_VALUE		 100	// 10 dBm, default dBm level2 for both AD8307
#define	CALFWD1_RAW_DEFAULT	 3462	// Default raw Voltage level1 at  40 dBm
#define	CALREV1_RAW_DEFAULT	 3462	// Default raw Voltage level1 at  40 dBm
#define	CALFWD2_RAW_DEFAULT	 2232	// Default raw Voltage level2 at  10 dBm
#define	CALREV2_RAW_DEFAULT	 2232	// Default raw Voltage level2 at  10 dBm
#define CAL_INP_QUALITY		 400	// Minimum difference between the raw (12 bit ref 5V) input voltage
									// levels for the forward and reverse measurements - in order to
									// allow a Calibration
#else								// Defs when using a 30 to 1 coupler and internal A/D
#define	CAL1_NOR_VALUE		 400	// 40 dBm, default dBm level1 for both AD8307
#define	CAL2_NOR_VALUE		 100	// 10 dBm, default dBm level2 for both AD8307
#define	CALFWD1_RAW_DEFAULT	 3228	// Default raw Voltage level1 at  40 dBm
#define	CALREV1_RAW_DEFAULT	 3228	// Default raw Voltage level1 at  40 dBm
#define	CALFWD2_RAW_DEFAULT	 2028	// Default raw Voltage level2 at  10 dBm
#define	CALREV2_RAW_DEFAULT	 2028	// Default raw Voltage level2 at  10 dBm
#define CAL_INP_QUALITY		 400	// Minimum difference between the raw (12 bit ref 5V) input voltage
									// levels for the forward and reverse measurements - in order to
									// allow a Calibration
#endif	// End coupler selection
#endif					// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection

//-----------------------------------------------------------------------------
// DEFS for MCK12140+MC100ELT23 Calibration (see comments in PM_PowerImpedance_Meter.c)
#if		PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
#define PH_U_90_DEG_AVG	0.9054		// Phase voltage at  90 degrees, U pin.
#define PH_U_0_DEG_AVG	1.7737		// Phase voltage at   0 degrees, U pin.
#define PH_U_270DEG_AVG	2.6520		// Phase voltage at -90 degrees, U pin.
#define PH_D_90_DEG_AVG	2.6828		// Phase voltage at  90 degrees, D pin.
#define PH_D_0_DEG_AVG	1.7726		// Phase voltage at   0 degrees, D pin.
#define PH_D_270DEG_AVG 0.9127		// Phase voltage at -90 degrees, D pin.
#endif					// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection

//-----------------------------------------------------------------------------
// LED Blink
//
// None, or only one of the four should be selected
#define FAST_LOOP_THRU_LED	0		// Blink PB2 LED every time, when going through the mainloop *OR*
#define	MS_LOOP_THRU_LED	1		// Blink PB2 LED every 5ms, when going through the mainloop *OR*
#define	MED_LOOP_THRU_LED	0		// Blink PB2 LED every 10ms, when going through the mainloop *OR*
#define SLOW_LOOP_THRU_LED	0		// Blink PB2 LED every 100ms, when going through the mainloop

//-----------------------------------------------------------------------------
// DEFS for LEDS
#define LED_PORT			PORTD	// port for the LED
#define LED_DDR				DDRD	// port for the LED
#define LED					(1<<6)	// pin for LED

#define EXTLED_PORT			PORTE	// port for the LED
#define EXTLED_DDR			DDRE	// port for the LED
#define EXT_R_LED			(1<<0)	// pin for Red External LED
#define EXT_G_LED			(1<<1)	// pin for Green External LED

//-----------------------------------------------------------------------------
// DEFS for the Rotary Encoder function
// If Scan style is used, then any input pin can be used for Phase A and Phase B,
// even using two separate input ports.  However, if using Interrupt style, then
// at least one of the input pins has to be selected from those supporting INT.
//
#define ENCODER_SCAN_STYLE	0		// Scanning type Rotary Encoder routine, regularly scans the GPIO inputs
									// Gives full resolution of rotary encoder                *OR*
#define ENCODER_INT_STYLE	1		// Interrupt driven Rotary Encoder VFO, uses one interrupt, gives
									// only half the resolution of the encoder (every other click inactive)
//-----------------------------------------------------------------------------
#define ENC_A_PORT		PORTE		// PhaseA port register
#define ENC_A_DDR		DDRE		// PhaseA port direction register
#define ENC_A_PORTIN	PINE		// PhaseA port input register
#define ENC_A_PIN		(1 << 7)	// PhaseA port pin
#define ENC_B_PORT		PORTE		// PhaseB port register
#define ENC_B_DDR		DDRE		// PhaseB port direction register
#define ENC_B_PORTIN	PINE		// PhaseB port input register
#define ENC_B_PIN		(1 << 6)	// PhaseB port pin
#define ENCODER_DIR_REVERSE	0		// Reverse the direction of the Rotary Encoder
//-----------------------------------------------------------------------------
#if ENCODER_SCAN_STYLE				// Defines for Scan Style Encoder Routine
#define	ENC_RES_DIVIDE		2		// Default reduction of the Encoder Resolution
//-----------------------------------------------------------------------------
#else  // ENCODER_INT_STYLE			// Defines for Interrupt Style Encoder Routine
#define	ENC_RES_DIVIDE		1		// Default reduction of the Encoder Resolution
// Usable external interrupt pins on the AT90USB1286:
// INT0->PD0, INT1->PD1, INT2->PD2, INT3->PD3,
// INT4->PE4, INT5->PE5, INT6->PE6, INT7->PE7
// Interrupt Configuration Parameters
#define ENC_A_SIGNAL	INT6_vect	// Interrupt signal name: INTx, x = 0-7
#define ENC_A_IREG		EIMSK		// Interrupt register, always EIMSK
#define ENC_A_ICR		EICRB		// Interrupt Config Register
									// INT0-3: EICRA, INT4-7: EICRB
#define ENC_A_INT		(1 << INT6)	// matching INTx bit in EIMSK
#define ENC_A_ISCX0		(1 << ISC60)// Interrupt Sense Config bit0 (ISCx0)
#define ENC_A_ISCX1		(1 << ISC61)// Interrupt Sense Config bit1 (ISCx1)
#endif

//-----------------------------------------------------------------------------
// Definitions for the Pushbutton input pin
#define ENC_PUSHB_PORT		PORTB
#define ENC_PUSHB_DDR		DDRB
#define	ENC_PUSHB_INPORT	PINB
#define	ENC_PUSHB_PIN		(1 << 0)// PB0

// Definitions for the Pushbutton functionality
#define ENC_PUSHB_MIN		1		// Min pushdown for valid push (x 10ms)
#define	ENC_PUSHB_MAX		10		// Min pushdown for memory save (x 10ms)

//-----------------------------------------------------------------------------
// Select Bargraph display style
#define BARGRAPH_STYLE_1	1		// N8LP LP-100 look alike bargraph         *OR*
#define BARGRAPH_STYLE_2	0		// Bargraph with level indicators          *OR*
#define BARGRAPH_STYLE_3	0		// Another bargraph with level indicators  *OR*
#define BARGRAPH_STYLE_4	0		// Original bargraph, Empty space enframed *OR*
#define BARGRAPH_STYLE_5	0		// True bargraph, Empty space is empty


//
//-----------------------------------------------------------------------------
// Miscellaneous software defines, functions and variables
//-----------------------------------------------------------------------------
//

//-----------------------------------------------------------------------------
// Flags

// DEFS for all kinds of Flags
extern uint16_t			Status;
#define ENC_CHANGE		(1 << 0)	// Indicates that Encoder value has been modified
#define SHORT_PUSH		(1 << 1)	// Short Push Button Action
#define	LONG_PUSH		(1 << 2)	// Long Push Button Action
#define BARGRAPH_CAL	(1 << 3)	// 16dB Bargraph has been re-centred
#define MODE_CHANGE		(1 << 4)	// Display mode has changed
#define MODE_DISPLAY	(1 << 5)	// Display mode has changed
#define USB_AVAILABLE	(1 << 6)	// USB Serial Data Output Enabled
#define SWR_FLAG		(1 << 7)	// SWR Flag indicating SWR above threshold
#define IDLE_REFRESH	(1 << 8)	// Force Screensaver Reprint

// Operation Mode Flags
extern	uint16_t			Menu_Mode;		// Which Menu Mode is active
#if PHASE_DETECTOR		// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
#define POWER_RJX_BARPK		(1 << 0)
#define POWER_RJX_BARAVG	(1 << 1)
#define	POWER_RJX_BARINST	(1 << 2)
#define POWER_BARPK			(1 << 3)
#define POWER_BARAVG		(1 << 4)
#define POWER_BARINST		(1 << 5)
#define POWER_CLEAN_DBM		(1 << 6)
#define POWER_MIXED			(1 << 7)
//#define DEBUG				(1 << 8)
#define	CONFIG				(1 << 9)
//
#define	DEFAULT_MODE		(1 << 0)		// Default Menu Mode
#else					// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code				
#define POWER_BARPK			(1 << 0)
#define POWER_BARAVG		(1 << 1)
#define POWER_BARINST		(1 << 2)
#define POWER_CLEAN_DBM		(1 << 3)
#define POWER_MIXED			(1 << 4)
//#define DEBUG				(1 << 5)
#define	CONFIG				(1 << 6)
//
#define	DEFAULT_MODE 		(1 << 0)		// Default Menu Mode
#endif

// USB Output Flags, used with [var_t].USB_Flags
#define USBPPOLL		(1 << 0)			// $pinst command last selected
#define USBPINST		(1 << 1)			// $pinst command last selected
#define	USBPPK			(1 << 2)			// $ppep command last selected
#define	USBPPEP			(1 << 3)			// $ppep command last selected
#define USBPAVG			(1 << 4)			// $pavg command last selected
#define USBP_DB			(1 << 5)			// if set, then inst/pk/pep/avg will be in dB.
#define USBPLONG		(1 << 6)			// $plong (inst, pep, avg) selected
#define	USBPCONT		(1 << 7)			// $pcont, continuous transmission of last selected


//-----------------------------------------------------------------------------
// Macros
#ifndef SQR
#define SQR(x) ((x)*(x))
#endif
#ifndef ABS
#define ABS(x) ((x>0)?(x):(-x))
#endif


//-----------------------------------------------------------------------------
// Structures and Unions

typedef struct {
	int16_t	db10m;							// Calibrate, value in dBm x 10
	#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
	int16_t	V;								// corresponding A/D value for AD8307 Voltage sensor
	int16_t	I;								// corresponding A/D value for AD8307 Curent sensor
	#else				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
	int16_t	Fwd;							// corresponding A/D value for AD8307 Forward output
	int16_t	Rev;							// corresponding A/D value for AD8307 Reverse output
	#endif
} cal_t;

typedef struct {							// Phase reference levels for the two MCK12140+MC100ELT23 outputs.
	double pos90deg;						// Phase voltage referenced at 90 degrees
	double zerodeg;							// Phase voltage referenced at 0 degrees
	double neg90deg;						// Phase voltage referenced at -90 degrees
} phase_t;

typedef struct 
{
		#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
		phase_t		U;						// Phase reference levels for the two
		phase_t		D;						// MCK12140+MC100ELT23 outputs.  
		#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
		uint8_t		EEPROM_init_check;		// If value mismatch,
											// then update EEPROM to factory settings
		int16_t		encoderRes;				// (initial) Encoder resolution
		cal_t		cal_AD[2];				// 2 Calibration points for both AD8307
		uint8_t		USB_data;				// Bool indicating whether output is being
											// transmitted over serial/USB
		uint8_t		SWR_alarm_trig;			// SWR level*10 for Alarm trigger
		uint16_t	SWR_alarm_pwr_thresh;	// Minimum Power required to trigger SWR Alarm
		uint8_t		USB_Flags;				// Bool indicating whether continuous output over
											// serial/USB (selected/deselected w USB commands $pcont/$ppoll)
		uint16_t	PEP_period;				// PEP envelope sampling time in 5ms increments (200 = 1 second)
		uint8_t		ScaleRange[3];			// User settable Scale ranges, up to 3 ranges per decade.
		char		idle_disp[21];			// Sleep Display (configurable by USB serial command)
		float		idle_disp_thresh;		// Minimum level in mW to exit Sleep Display	
} var_t;


//-----------------------------------------------------------------------------
// Global variables
extern	EEMEM 		var_t E;				// Default Variables in EEPROM
extern				var_t R;				// Runtime Variables in Ram

extern	char		lcd_buf[];				// Used to process data to be passed to LCD and USB Serial

#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
extern 	int16_t		ad8307_adV;				// Measured A/D value from AD8307 voltage sensor as a 12 bit value
extern 	int16_t		ad8307_adI;				// Measured A/D value from AD8307 current sensor as a 12 bit value
extern 	int16_t		mck12140pos;			// Measured A/D value from MCK12140 phase sensor, positive direction pin
extern	int16_t		mck12140neg;			// Measured A/D value from MCK12140 phase sensor, negative direction pin
extern	double		ad8307_VdBm;			// Measured AD8307 voltage in dBm
extern	double		ad8307_IdBm;			// Measured AD8307 current in dBm
extern	double		phase;					// Radians
extern	double		Gamma;					// Complex number describing both magnitude and phase of reflection
extern	double		imp_R;					// Resistive component of the measured impedance (normalized, multiply by 50)
extern	double		imp_jX;					// Reactive component of the measured impedance (normalized, multiply by 50)
#else				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
extern	int16_t		ad8307_adF;				// Measured A/D value from the AD8307 Forward sensor as a 12 bit value
extern	int16_t		ad8307_adR;				// Measured A/D value from the AD8307 Reverse sensor as a 12 bit value
extern	double		ad8307_FdBm;			// Measured AD8307 forward voltage in dBm
extern	double		ad8307_RdBm;			// Measured AD8307 reverse current in dBm
#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection

extern	BOOL		Reverse;				// True if reverse power is greater than forward power
extern	double		fwd_power_db;			// Calculated forward power in dBm
extern	double		ref_power_db;			// Calculated reflected power in dBm
extern	double		power_db;				// Calculated power in dBm
extern	double		power_db_pep;			// Calculated PEP power in dBm
extern	double		power_db_pk;			// Calculated 100ms peak power in dBm
extern	double		power_db_avg;			// Calculated AVG power in dBm

extern	double		fwd_power_mw;			// Calculated forward power in mW
extern	double		ref_power_mw;			// Calculated reflected power in mW
extern	double		power_mw;				// Calculated power in mW
extern	double		power_mw_pep;			// Calculated PEP power in mW
extern	double		power_mw_pk;			// Calculated 100ms peak power in mW
extern	double		power_mw_avg;			// Calculated AVG power in mW

//extern	double	modulation_index;		// Modulation index
extern	double		swr;					// SWR as an absolute value
extern	uint16_t	swr_bar;				// logarithmic SWR value for bargraph, 1000 equals SWR of 10:1

extern	int16_t		encOutput;				// Output From Encoder

extern	uint8_t		ad7991_addr;			// Address of AD7991 I2C connected A/D, 0 if none detected
										
//-----------------------------------------------------------------------------
// Prototypes for functions
// PM.c

// PM_Encoder.c
extern void 		encoder_Init(void);		// Initialise the Rotary Encoder
#if ENCODER_SCAN_STYLE						// Only used if Scan Style Encoder Routine
extern void			encoder_Scan(void);		// Scan the Rotary Encoder
#endif

// Push Button and Rotary Encoder Menu functions
extern void			PushButtonMenu(void);

// PM_Print_Format__Functions.c
extern void			print_swr(void);
extern void			print_dbm(int16_t);
extern void			print_p_mw(double);
extern void			print_p_reduced(double);

// PM_Display_Functions.c
#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
extern void			lcd_display_rjx(char *, double);
#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
extern void			lcd_display_clean(char *, char *, double);
extern void			lcd_display_clean_dBm(void);
extern void			lcd_display_mixed(void);
extern void			lcd_display_debug(void);

// PM_USB_Serial.c
extern void			usb_poll_data(void);	// Write data to USB virtual serial port
extern void			usb_poll_inst(void);	// Write data to USB virtual serial port
extern void			usb_poll_pk(void);		// Write data to USB virtual serial port
extern void			usb_poll_pep(void);		// Write data to USB virtual serial port
extern void			usb_poll_avg(void);		// Write data to USB virtual serial port
extern void			usb_poll_instdb(void);	// Write data to USB virtual serial port
extern void			usb_poll_pkdb(void);	// Write data to USB virtual serial port
extern void			usb_poll_pepdb(void);	// Write data to USB virtual serial port
extern void			usb_poll_avgdb(void);	// Write data to USB virtual serial port
extern void			usb_poll_long(void);	// Write data to USB virtual serial port
extern void			usb_read_serial(void);	// Read incoming messages from USB bus

// LCD Bargraph stuff
extern void			lcdProgressBarPeak(uint16_t, uint16_t, uint16_t, uint8_t);
extern void			lcd_bargraph_Init(void);

// Read A/D inputs, either builtin or I2C connected AD7991-1 or AD7991-0
extern uint8_t		I2C_Init(void);			// Initialize I2C and determine if AD7991 is connected
extern void			adc_poll(void);			// Read builtin or AD7991 AD inputs

// Determine Power, SWR etc...
#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
extern void			imp_determine_dBm(void);		// Convert raw A/D values to dBm
extern void			imp_determine_phase(void);		// Process Phase Detector outputs
extern void			imp_calc_Power(void);			// Calculate all kinds of Power
#else				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
extern void			pswr_determine_dBm(void);		// Convert raw A/D values to dBm
extern void			pswr_calc_Power(void);			// Calculate all kinds of Power
#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection

#endif
