//*********************************************************************************
//**
//** Project.........: A menu driven Multi Display RF Power and SWR Meter
//**                   using a Tandem Match Coupler and 2x AD8307.
//**
//** Copyright (C) 2015  Loftur E. Jonasson  (tf3lj [at] arrl [dot] net)
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
//** Platform........: Teensy++ 2.0 or Teensy 3.1 (http://www.pjrc.com)
//**                   (Some other Arduino type platforms, such as the 
//**                    Mega 2560 may also work if appropriate modifications
//**                    to pin assigments are made in the PSWR_A.h file)
//**
//** Initial version.: 0.50, 2013-09-29  Loftur Jonasson, TF3LJ / VE2LJX
//**                   (beta version)
//**
#define                VERSION "0b19"
#define                DATE    "2015-09-04"
//**
//*********************************************************************************

//-----------------------------------------------------------------------------
// The code is written to accomodate both Teensy 3.1 and Teensy++ 2.0 Microcontrollers
// Default is Teensy 3.1
// For Teensy++ 2.0, a couple of lines are commented out near the top of the file:
// PSWR_A_Measure.ino   (this is due to an Arduino Preprocessor BUG)
// If set up for Teensy++2.0, and with inclusion of the appropriate libraries,
// the firmware may also work for Arduino Mega 2560, however this has not been tested.
// Sketch size for Teensy++ 2.0 is approx 40 kBytes Flash ROM + 6.5 kB RAM
// Sketch size for Teensy 3.1   is approx 80 kBytes Flash ROM + 16 kB RAM
//-----------------------------------------------------------------------------

#include <Arduino.h>

//
//-----------------------------------------------------------------------------
// Features Selection
//-----------------------------------------------------------------------------  
//

//-----------------------------------------------------------------------------
// 2X AD8307 Log Amp Detectors, or Diode Detectors
#define AD8307_INSTALLED        1       // 0 for Diode Detectors, 1 for 2x AD8307

//-----------------------------------------------------------------------------  
// Poll for I2C enabled external AD7991or no?
// (it is harmless to keep this on even if not used)
#define WIRE_ENABLED            0       // 1 to enable, 0 to disable

//-----------------------------------------------------------------------------
// Defaults for Teensy 3.1
#if defined(__MK20DX256__)
//If Teensy 3.1, then make use of the additional RAM and speed,
                                        // Sample the AD inputs 500 times per second
#define SAMPLE_TIME             2       // Time between samples, milliseconds
#define PEP_BUFFER              2500    // PEP Buffer size, can hold up to 5 second PEP
#define BUF_SHORT               50      // Buffer size for 100ms Peak
#define AVG_BUF                 500     // Buffer size for 1s Average measurement
//-----------------------------------------------------------------------------
// Defaults for Teensy++ 2.0 (and can probably also be coerced to work with Arduino Mega 2560)
#else                                   // Sample the ADs 200 times per second
#define SAMPLE_TIME             5       // Time between samples, milliseconds
#define PEP_BUFFER              1000    // PEP Buffer size, can hold up to 5 second PEP
#define BUF_SHORT               20      // Buffer size for 100ms Peak
#define AVG_BUF                 200     // Buffer size for 1s Average measurement
#endif

//-----------------------------------------------------------------------------
// EEPROM settings Serial Number. Increment this number when firmware mods necessitate
// fresh "Factory Default Settings" to be forced into the EEPROM at first boot after
// an upgrade
#define COLDSTART_REF           0x02   // When started, the firmware examines this "Serial Number
                                       // and enforce factory reset if there is a mismatch.
                                       // This is useful if the EEPROM structure has been modified

//-----------------------------------------------------------------------------
//Text for Startup Display... Play with and tailor to your own liking
#if AD8307_INSTALLED
#define STARTUPDISPLAY1         "AD8307 Power & SWR"
#define STARTUPDISPLAY2         "Meter"
#define STARTUPDISPLAY3         "milliwatts"
#define STARTUPDISPLAY4         "to kilowatts"
#else
#define STARTUPDISPLAY1         ""
#define STARTUPDISPLAY2         "Power & SWR Meter"
#define STARTUPDISPLAY3         ""
#define STARTUPDISPLAY4         ""
#endif
#define STARTUPDISPLAY5         "TF3LJ / VE2LJX" // You may want to indicate your own callsign :)

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Various Initial Default settings for Meter
// (many of these are configurable by user through Config Menu or USB commands)
//
//-----------------------------------------------------------------------------
// PEP envelope sample size for 1, 2.5 or 5 second sample time
#define PEP_PERIOD 2500/SAMPLE_TIME    // 2.5 seconds = Default

//-----------------------------------------------------------------------------
// DEFS for SWR Alarm
#define SWR_ALARM                30    // 30=3.0, default SWR Alarm Trigger
#define SWR_THRESHOLD            10    // Default SWR Alarm Power Threshold in mW

//-----------------------------------------------------------------------------
// DEFS for User Definable Scale Ranges
#define SCALE_RANGE1              6    // User definable Scale Ranges, up to 3 ranges per decade                   
#define SCALE_RANGE2             12    // e.g. ... 6W 12W 24W 60W 120W 240W ...
#define SCALE_RANGE3             24    // If all values set as "2", then ... 2W 20W 200W ...

//-----------------------------------------------------------------------------
// DEFS for Default Screensaver Message
#define SLEEPMSG   "ZZzzzz zzz ..."    // Shown when nothing else to display on LCD
                                       // Configurable by USB Serial input command:
                                       // $sleepmsg="blabla"
#define SLEEPTIME                50    // Time to renew position of screensaver message
                                       // in tenths of a second
#define SLEEPTHRESHOLD        0.001    // Only used if AD8307. Minimum relevant power to exit Sleep Display (0.001=1uW)

//-----------------------------------------------------------------------------
// Mode Intro Time (decides for how long mode intro is displayed when turning encoder
#define MODE_INTRO_TIME          10    // Tenths of a second (10 equals 1s)

//-----------------------------------------------------------------------------
// DEFS for FWD and REF coupler, 2xAD8307 or Diode detectors
//-----------------------------------------------------------------------------
// DEFS for AD8307 Calibration (dBm *10)
// levels for the forward and reverse measurements - in order to allow a Calibration
//
//-----------------------------------------------------------------------------
// 20 to 1 Tandem Match with Power and SWR Meter
// (default defines are for a 30 to 1 Tandem Match)
#define TWENTYTOONE               0    // 1 to select
//
#if TWENTYTOONE                        // Defs when using a 20 to 1 coupler and AD7991
#define	CAL1_NOR_VALUE          400    // 40 dBm, default dBm level1 for both AD8307
#define	CAL2_NOR_VALUE          100    // 10 dBm, default dBm level2 for both AD8307
#define	CALFWD1_RAW_DEFAULT    3462    // Default raw Voltage level1 at  40 dBm
#define	CALREV1_RAW_DEFAULT    3462    // Default raw Voltage level1 at  40 dBm
#define	CALFWD2_RAW_DEFAULT    2232    // Default raw Voltage level2 at  10 dBm
#define	CALREV2_RAW_DEFAULT    2232    // Default raw Voltage level2 at  10 dBm
#define CAL_INP_QUALITY         400    // Minimum difference between the raw (12 bit) input voltages
                                       //   to be allowed to calibrate
#else                                  // Defs when using a 30 to 1 coupler and internal A/D
#define	CAL1_NOR_VALUE          400    // 40 dBm, default dBm level1 for both AD8307
#define	CAL2_NOR_VALUE          100    // 10 dBm, default dBm level2 for both AD8307
#define	CALFWD1_RAW_DEFAULT    3228    // Default raw Voltage level1 at  40 dBm
#define	CALREV1_RAW_DEFAULT    3228    // Default raw Voltage level1 at  40 dBm
#define	CALFWD2_RAW_DEFAULT    2028    // Default raw Voltage level2 at  10 dBm
#define	CALREV2_RAW_DEFAULT    2028    // Default raw Voltage level2 at  10 dBm
#define CAL_INP_QUALITY         400    // Minimum difference between the raw (12 bit) input voltages
                                       //   to be allowed to calibrate
#endif	                               // End coupler selection
//-----------------------------------------------------------------------------
// DEFS for Diode Detectors
// Typical Tandem Match parameters for a 200W bridge
//
// BRIDGE_COUPLING (attenuation) = N_transformer, e.g. 20 turns Tandem Match equals 20
#define BRIDGE_COUPLING       20.0  // Bridge Coupling
#define METER_CAL             1.00  // Calibration fudge factor (0 - 2.5)
#define VALUE_R1ST           22000  // Resistor values of Resistor to bridge in voltage divider (Rx)
#define VALUE_R2ND           18000  // Resistor values of Resistor to ground in voltage divider (Ry)
#define D_VDROP               0.25  // Voltage Drop over Diode
//#endif

//-----------------------------------------------------------------------------
// Defs for Power and SWR indication
//
#if AD8307_INSTALLED                // --------------Used with AD8307:
#if TWENTYTOONE                     // Defs when using a 20 to 1 coupler and AD7991
#define MIN_PWR_FOR_SWR_CALC   0.1  // Minimum Power in mW for SWR calculation and display
#else                               // Defs when using a 30 to 1 coupler and AD7991
#define MIN_PWR_FOR_SWR_CALC   1.0  // Minimum Power in mW for SWR calculation and display
#endif
#define MIN_PWR_FOR_SWR_SHOW  0.01  // Minimum Power in mW for SWR indication (use recent value)
//-----------------------------------------------------------------------------
#else                               // --------------Used if Diode detectors:
#define MIN_PWR_FOR_METER       20  // Minimum Power in mW for Power/SWR Meter indication on LCD
#define MIN_PWR_FOR_SWR_CALC    10  // Minimum Power in mW for SWR calculation and display
#define MIN_PWR_FOR_SWR_SHOW    10  // Minimum Power in mW for SWR indication (use recent value)
#endif

//-----------------------------------------------------------------------------
// LED Blink
//
// None, or only one of the three below should be selected
#define FAST_LOOP_THRU_LED        0    // Blink the builtin led on Microprocessor every time, when going through the mainloop *OR*
#define	MS_LOOP_THRU_LED          1    // Blink the builtin led on Microprocessor when going through the mainloop *OR*
#define SLOW_LOOP_THRU_LED        0    // Blink the builtin led on Microprocessor every 100ms, when going through the mainloop

//-----------------------------------------------------------------------------
// Definitions for Rotary Encoder and Pushbutton
#define  ENC_RESDIVIDE            2    // Encoder resolution reduction
#define  ENACT_MIN                2    // Minimum Menu/Enact push for "short push" (x 5 ms)
#define  ENACT_MAX              200    // Minimum Menu/Enact push for Menu Mode (x 5 ms)

//-----------------------------------------------------------------------------  
// Assign pins to inputs and outputs (Arduino style)
//-----------------------------------------------------------------------------  
//
#if defined(__MK20DX256__)
//-----------------------------------------------------------------------------
// Teensy 3.1 - Pinout "mostly" compatible with Automatic Magnetic Loop Antenna Controller
const int R_Led              =   14;   // SWR Alarm
const int G_Led              =   15;   // Not used currently
const int X_Led              =   13;   // Debug
const int EncI               =    9;
const int EncQ               =   10;
const int EnactSW            =   23;   // Menu/Enact Switch
// LCD pins
const int LCD_D4             =    5;
const int LCD_D5             =    6;
const int LCD_D6             =    7;
const int LCD_D7             =    8;
const int LCD_RS             =    2;
const int LCD_RW             =    3;
const int LCD_E              =    4;
// AD inputs for Forward and Reflected Power (SWR measurement)
const int Pfwd               =  A10;   // ADC1 - Need to select a pair which can use both builtin ADCs
const int Pref               =  A11;   // ADC0 -  for example A11 (ADC0) & A10 (ADC1).     AD0-AD9 can
                                       //         do ADC0, AD2 and AD3 can also do ADC1.
//const int SCL0             =   19    // Not compatible with Automatic Magnetic Loop Controller
//const int SDA0             =   18
//const int SCL1             =   29    // These could potentially be used if using
//const int SDA1             =   30    //   i2c_t3 library instead of Wire library
#else
//-----------------------------------------------------------------------------
// Default for Teensy++ 2.0 (pinout will have to be modified for
// Arduino Mega 2560 or Arduino Mega ADK)
const int R_Led              =    8;   // SWR Alarm
const int G_Led              =    9;   // Not used currently
const int X_Led              =    6;   // Debug
const int EncI               =   19;   // Swap 18 and 19 if wrong
const int EncQ               =   18;   //   direction of Encoder
const int EnactSW            =   20;   // Menu/Enact Switch
// LCD pins
const int LCD_D4             =   14;
const int LCD_D5             =   15;
const int LCD_D6             =   16;
const int LCD_D7             =   17;
const int LCD_RS             =   10;
const int LCD_RW             =   11;
const int LCD_E              =   12;
// AD inputs for Forward and Reflected Power (SWR measurement)
const int Pfwd               =   A0;
const int Pref               =   A1;
//const int SCL              =    0    // No need to define, default
//const int SDA              =    1    //   pins for Wire (I2C)
#endif


//
//-----------------------------------------------------------------------------
// Miscellaneous non-configurable software defines and variables
//-----------------------------------------------------------------------------
//

//-----------------------------------------------------------------------------
// Structures and Unions

typedef struct {
          int16_t  db10m;                     // Calibrate, value in dBm x 10
          int16_t  Fwd;                       // corresponding A/D value for AD8307 Forward output
          int16_t  Rev;                       // corresponding A/D value for AD8307 Reverse output
               }  cal_t;

typedef struct {
          cal_t    cal_AD[2];                 // 2 Calibration points for both AD8307, if AD8307 option
          uint8_t  meter_cal;                 // Calibration multiplier for diode detector option - 100=1.0
          uint8_t  SWR_alarm_trig;            // SWR level*10 for Alarm trigger
          uint16_t SWR_alarm_pwr_thresh;      // Minimum Power required to trigger SWR Alarm
          unsigned usb_report_cont     : 1;   // Continuous USB Serial Reporting of Power and SWR
          unsigned usb_report_type     : 4;   // Which type of Power reporting is active over USB?
                                              // serial/USB (selected/deselected w USB commands $pcont/$ppoll)
                   #define  REPORT_DATA    1    // Report Instantaneous Power (raw format) and SWR to USB
                   #define  REPORT_INST    2    // Report Instantaneous Power (formatted) and SWR to USB
                   #define  REPORT_PK      3    // Report Peak (100ms) Power and SWR to USB
                   #define  REPORT_PEP     4    // Report PEP (1s) Power and SWR to USB
                   #define  REPORT_AVG     5    // Report Average (1s) Power and SWR to USB
                   #define  REPORT_INSTDB  6    // Report Instantaneous Power (formatted) and SWR to USB
                   #define  REPORT_PKDB    7    // Report Peak (100ms) Power and SWR to USB
                   #define  REPORT_PEPDB   8    // Report PEP (1s) Power and SWR to USB
                   #define  REPORT_AVGDB   9    // Report Average (1s) Power and SWR to USB
                   #define  REPORT_LONG   10    // Report Power and SWR to USB, long Human Readable format                   
          uint16_t PEP_period;                // PEP envelope sampling time in SAMPLE_TIME increments
          uint8_t  ScaleRange[3];             // User settable Scale ranges, up to 3 ranges per decade.
          char     idle_disp[21];             // Sleep Display (configurable by USB serial command)
          float    idle_disp_thresh;          // Minimum level in mW to exit Sleep Display	
               } var_t;

typedef struct  {
          unsigned short_push          : 1;   // Short Push Button Action
          unsigned long_push           : 1;   // Long Push Button Action
          unsigned bargraph_cal        : 1;   // 16dB Bargraph has been re-centred
          unsigned mode_change         : 1;   // Display mode has changed
          unsigned mode_display        : 1;   // Display mode has changed
          unsigned swr_alarm           : 1;   // SWR Flag indicating SWR above threshold
          unsigned idle_refresh        : 1;   // Force Screensaver reprint
          unsigned menu_lcd_upd        : 1;   // Refresh/Update LCD when in Menu Mode
                } flags;

typedef struct  {
          unsigned disp                : 3;   // Indicates current Display mode where:
                   #define POWER_BARPK     1
                   #define POWER_BARAVG    2
                   #define POWER_BARINST   3
                   #define POWER_CLEAN_DBM 4
                   #define POWER_MIXED     5
                   //#define DEBUG         6
                   #define DEFAULT_MODE    1
                   #define MAX_MODE        5  // Same as highest         
          unsigned conf                : 1;
                } modeflags;

//-----------------------------------------------------------------------------
// Macros
#ifndef SQR
#define SQR(x) ((x)*(x))
#endif
#ifndef ABS
#define ABS(x) ((x>0)?(x):(-x))
#endif

//-----------------------------------------------------------------------------
// Soft Reset
#if defined(__MK20DX256__) // Soft Reset, Teensy 3 style
#define RESTART_ADDR       0xE000ED0C
#define RESTART_VAL        0x5FA0004
#define SOFT_RESET()       ((*(volatile uint32_t *)RESTART_ADDR) = (RESTART_VAL))
#else	                   // Soft Reset, Atmel style
#define SOFT_RESET()       asm volatile("jmp 0x00000");
#endif

//-----------------------------------------------------------------------------
// Bool stuff
#define	TRUE       1
#define FALSE      0

