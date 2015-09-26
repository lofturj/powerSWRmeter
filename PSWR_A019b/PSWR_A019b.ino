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
//**
//*********************************************************************************

#include <LiquidCrystalFast.h>
#include <Metro.h>
#include <EEPROM.h>
#include <Encoder.h>

#include "_EEPROMAnything.h"
#include "PSWR_A.h"

#if WIRE_ENABLED
#include <Wire.h>                    // I2C Comms, if enabled
#endif

modeflags   mode;                    // Display and Menu Mode flags
flags       flag;                    // Various op flags
uint8_t     Menu_disp_timer;         // Used for a timed display when returning from Menu

int16_t     fwd;                     // AD input - 12 bit value, v-forward
int16_t     ref;                     // AD input - 12 bit value, v-reverse

uint8_t     Reverse;                 // BOOL: True if reverse power is greater than forward power

#if AD8307_INSTALLED
double      ad8307_FdBm;             // Measured AD8307 forward voltage in dBm
double      ad8307_RdBm;             // Measured AD8307 reverse current in dBm
double      fwd_power_mw;            // Calculated forward power in mW
double      ref_power_mw;            // Calculated reflected power in mW
double      power_mw;                // Calculated power in mW
double      power_mw_pep;            // Calculated PEP power in mW
double      power_mw_pk;             // Calculated 100ms peak power in mW
double      power_mw_avg;            // Calculated AVG power in mW
#else
int32_t     fwd_power_mw;            // Calculated forward power in mW
int32_t     ref_power_mw;            // Calculated reflected power in mW
int32_t     power_mw;                // Calculated power in mW
int32_t     power_mw_pep;            // Calculated PEP power in mW
int32_t     power_mw_pk;             // Calculated 100ms peak power in mW
int32_t     power_mw_avg;            // Calculated AVG power in mW
#endif

double      fwd_power_db;            // Calculated forward power in dBm
double      ref_power_db;            // Calculated reflected power in dBm
double      power_db;                // Calculated power in dBm
double      power_db_pep;            // Calculated PEP power in dBm
double      power_db_pk;             // Calculated 100ms peak power in dBm
double      power_db_avg;            // Calculated AVG power in dBm

//double    modulation_index;        // Modulation index
double      swr=1.0;                 // SWR as an absolute value
uint16_t    swr_bar;                 // logarithmic SWR value for bargraph, 1000 equals SWR of 10:1

char lcd_buf[82];                    // Used to process data to be passed to LCD and USB Serial

uint8_t     X_LedState;              // BOOL: Debug LED				

//-----------------------------------------------------------------------------------------
// Variables in ram/flash rom (default)
var_t  R  = {
              {
                {                       // Meter calibration if 2x AD8307
                  CAL1_NOR_VALUE,
                  CALFWD1_RAW_DEFAULT,  // First Calibrate point, Forward direction, db*10 + 2 x AD values					 
                  CALREV1_RAW_DEFAULT   // First Calibrate point, Reverse direction, db*10 + 2 x AD values
                },
                {  
                  CAL2_NOR_VALUE,
                  CALFWD2_RAW_DEFAULT,  // Second Calibrate point, Forward direction, db*10 + 2 x AD values
                  CALREV2_RAW_DEFAULT   // Second Calibrate point, Reverse direction, db*10 + 2 x AD values
                }
              },                        // Second Calibrate point, Reverse direction, db*10 + 2 x AD values
                                        // Meter calibration if diode detectors
              (uint8_t) METER_CAL*100,  // Calibration fudge of diode detector style meter
              SWR_ALARM,                // Default SWR Alarm trigger, defined in PSWR_A.h
              SWR_THRESHOLD,            // Default SWR Alarm power threshold defined in PSWR_A.h
              0,                        // USB Continuous reporting off
              1,                        // USB Reporting type, 1=Instantaneous Power (raw format) and SWR to USB 
              PEP_PERIOD,               // PEP envelope sampling time in 5ms increments (200=1s, 500=2.5s, 1000=5s)
              {  
                SCALE_RANGE1,           // User definable Scale Ranges, up to 3 ranges per decade
                SCALE_RANGE2,           // e.g. ... 6W 12W 24W 60W 120W 240W ...
                SCALE_RANGE3            // If all values set as "2", then ... 2W 20W 200W ...
              },
              SLEEPMSG,                 // Shown when nothing else to display on LCD
                                        // Configurable by USB Serial input command: $sleepmsg="blabla"
              SLEEPTHRESHOLD            // Minimum relevant power to exit Sleep Display (0.001=1uW),
                                        // valid values are 0, 0.001, 0.01, 0.1, 1 and 10
            };
            

//-----------------------------------------------------------------------------------------
// Instanciate an Encoder Object
Encoder   Enc(EncI, EncQ);

//-----------------------------------------------------------------------------------------
// Timers for various tasks:
Metro     pswrMetro = Metro(SAMPLE_TIME);   // AD Sample timer for Power and SWR measurements
                                            // Also used to pace out characters to the LCD,
                                            // three characters per millisecond.
Metro     buttonMetro = Metro(5);           // 5ms timer to scan the pushbutton

Metro     slowMetro = Metro(100);           // 100 millisecond timer for various tasks

//-----------------------------------------------------------------------------------------
// initialize the LCD
LiquidCrystalFast lcd(LCD_RS, LCD_RW, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

//
//-----------------------------------------------------------------------------------------
// Top level task
// runs in an endless loop
//-----------------------------------------------------------------------------------------
//
void loop()
{
  static uint8_t  multi_button;             // State of Multipurpose Enact Switch
                                            // (Multipurpose pushbutton)
  static uint8_t  old_multi_button;         // Last state of Multipurpose Enact Switch

  //-------------------------------------------------------------------------------
  // Here we do routines which are to be run through as often as possible
  // currently measured to be approximately once every 25 - 50 us
  //-------------------------------------------------------------------------------
  #if FAST_LOOP_THRU_LED                    // Blink a LED every time
  digitalWrite(X_Led,X_LedState ^= 1);      // Blink a led
  #endif

  //-------------------------------------------------------------------------------
  // Here we do routines which are to be accessed once every SAMPLE_TIME milliseconds
  //-------------------------------------------------------------------------------
  // AD sample  Loop timer.
  if (pswrMetro.check() == TRUE)            // check if the metro has passed its interval .
  {
    #if MS_LOOP_THRU_LED                    // Blink LED every time going through here 
    digitalWrite(X_Led,X_LedState ^= 1);    // Blink a led
    #endif

    adc_poll();                             // Read AD, external or internal
    #if AD8307_INSTALLED
    pswr_determine_dBm();                   // Convert raw A/D values to dBm
    pswr_calc_Power();                      // Calculate all kinds of Power
    #else
    measure_power_and_swr();                // Calculate power and swr, diode detector
    calculate_pep_and_pk(power_mw);         // Calculate all kinds of power
    #endif    
    
    //-------------------------------------------------------------------
    // Print from virtual LCD to real LCD, approx one char per millisecond
    // (a typical HD44780 LCD is measured to use approx 15us per character)
    //
    virt_LCD_to_real_LCD();
  } 

  //-------------------------------------------------------------------------------
  // Here we do routines which are to be accessed once every 5ms (pushbutton scan)
  //-------------------------------------------------------------------------------
  // Multipurpose (Enact/Menu) Pushbutton state stuff
  if (buttonMetro.check() == TRUE)
  {
    multi_button = multipurpose_pushbutton();
    
    if (old_multi_button != multi_button)    // A new state of the Multi Purpose Pushbutton
    {
      if (multi_button == 1)                 // Short Push detected
      {
        flag.short_push = TRUE;              // Used with Configuraton Menu functions
      }
      else if (multi_button == 2)            // Long Push detected
      {
        mode.conf = TRUE;                    // Activate Configuration Menu
      }
    }
    old_multi_button = multi_button;
  }

  //-------------------------------------------------------------------------------
  // Here we do timing related routines which are to be accessed once every 1/10th of a second
  //-------------------------------------------------------------------------------
  if (slowMetro.check() == TRUE)            // check if the metro has passed its interval .
  {
    #if SLOW_LOOP_THRU_LED                  // Blink LED every 100ms, when going through the main loop 
    digitalWrite(X_Led,X_LedState ^= 1);    // Blink a led
    #endif

    //-------------------------------------------------------------------
    // The Menu function has 5 seconds lag time precedence
    if (Menu_disp_timer > 0) Menu_disp_timer--;  
    if (Menu_disp_timer == 1) virt_lcd_clear();

    //-------------------------------------------------------------------
    // Read Encoder to cycle back and forth through modes
    //
    // If the encoder was used while not in config mode:
    if ((mode.conf == FALSE) && (Enc.read()/ENC_RESDIVIDE != 0))
    {
      // Mode switching travels only one click at a time, ignoring extra clicks
      if (Enc.read()/ENC_RESDIVIDE > 0)
      {
        mode.disp++;
        if (mode.disp > MAX_MODE) mode.disp = 1;
        Enc.write(0);        // Reset data from Encoder
      }
      else if (Enc.read()/ENC_RESDIVIDE < 0)
      {
        mode.disp--;
        if (mode.disp == 0) mode.disp = MAX_MODE;
        Enc.write(0);        // Reset data from Encoder
      }

      // Force Mode Intro Display whenever Mode has been changed
      flag.mode_change = TRUE;
      flag.mode_display = TRUE;
    }

    //-------------------------------------------------------------------
    // Various Menu (rotary encoder) selectable display/function modes
    //
    if (mode.conf == TRUE)                   // Pushbutton Configuration Menu
    {
      PushButtonMenu();
    }	
    else if (mode.disp == POWER_BARPK)       // 100ms Peak Power, Bargraph, PWR, SWR, PEP
    {
      lcd_display_clean("100ms Peak Power", "pk ", power_mw_pk);
    }
    else if (mode.disp == POWER_BARAVG)      // 1s Average Power, Bargraph, PWR, SWR, PEP
    {
      lcd_display_clean("1 Sec Average Power", "avg", power_mw_avg);
    }
    else if (mode.disp == POWER_BARINST)     // Instantaneous Power, Bargraph, PWR, SWR, PEP
    {
      lcd_display_clean("Instantaneous Power", "   ", power_mw);
    }
    else if (mode.disp == POWER_CLEAN_DBM)   // Power Meter in dBm
    {
      lcd_display_clean_dBm();
    }
    else if (mode.disp == POWER_MIXED)       // Fwd and Reflected, [R+jX and Phase if Impedance Meter] etc...
    {
      lcd_display_mixed();
    }
    
    //-------------------------------------------------------------------     
    // Short push, not in config menu:
    if ((mode.conf == FALSE) && (flag.short_push == TRUE))
    {
      flag.short_push = FALSE;
      digitalWrite(R_Led,FALSE);             // Clear SWR Alarm LED
      flag.swr_alarm = FALSE;                // Clear SWR Alarm Flag
    }

    usb_cont_report();                       // Report Power and SWR to USB, if in Continuous mode
    
    //----------------------------------------------
    // Green LED if power is detected
    if (power_mw > R.idle_disp_thresh) digitalWrite(G_Led, 1);
    else digitalWrite(G_Led, 0);
  }

  //-------------------------------------------------------------------
  // Check USB Serial port for incoming commands
  usb_read_serial();
}


//
//-----------------------------------------------------------------------------------------
//      Setup Ports, timers, start the works and never return, unless reset
//                          by the watchdog timer
//                   then - do everything, all over again
//-----------------------------------------------------------------------------------------
//
void setup()
{
  uint8_t coldstart;

  // Enable LEDs and set to LOW = Off
  pinMode(R_Led, OUTPUT);                        // SWR Alarm
  digitalWrite(R_Led, LOW);
  pinMode(G_Led, OUTPUT);                        // Not used for now (previously used for USB traffic indication)
  digitalWrite(G_Led, LOW);
  pinMode(X_Led, OUTPUT);                        // Debug LED
  digitalWrite(X_Led, LOW);

  pinMode(EnactSW, INPUT_PULLUP);                // Menu/Enact pushbutton switch
  
  adc_init();                                    // Init Builtin ADCs
  
  Serial.begin(9600);                            // initialize USB virtual serial serial port

  #if WIRE_ENABLED                               // I2C enabled in PSWR_A.h?
  Wire.begin();                                  // Start I2C
  TWBR = 12;                                     // Set I2C rate at 400 kHz (is default at 100 kHz)
  #endif                                         // (16 MHz/400 kHz)/2 = 12             

  lcd.begin(20, 4);                              // Initialize a 20x4 LCD
  lcd_bargraph_Init();                           // Initialize LCD Bargraph

  coldstart = EEPROM.read(0);                    // Grab the coldstart byte indicator in EEPROM for
                                                 // comparison with the COLDSTART_REFERENCE
  //
  // Initialize all memories if first upload or if COLDSTART_REF has been modified
  // either through PSWR_A.h or through Menu functions
  if (coldstart != COLDSTART_REF)
  { 
    EEPROM.write(0,COLDSTART_REF);               // COLDSTART_REF in first byte indicates all initialized
    EEPROM_writeAnything(1,R);                   // Write default settings into EEPROM
  }
  else                                           // EEPROM contains stored data, retrieve the data
  {
    EEPROM_readAnything(1,R);                    // Read the stored data
  }

  #if WIRE_ENABLED
  uint8_t i2c_status = I2C_Init();               // Initialize I2C comms
  #endif
  
  //------------------------------------------
  // LCD Print Version and I2C information (6 seconds in total during startup)
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F(STARTUPDISPLAY1));
  lcd.setCursor(0,1);
  lcd.print(F(STARTUPDISPLAY2));
  delay(300);
  lcd.setCursor(20-strlen(STARTUPDISPLAY3),1);
  lcd.print(STARTUPDISPLAY3);
  delay(200);
  lcd.setCursor(20-strlen(STARTUPDISPLAY4),2);
  lcd.print(STARTUPDISPLAY4);
  delay(2500);

  lcd.setCursor(0,3);
  lcd.print(F(STARTUPDISPLAY5));
  sprintf(lcd_buf,"V%s", VERSION);
  lcd.setCursor(20-strlen(lcd_buf),3);
  lcd.print(lcd_buf);
  delay(2000);

  #if WIRE_ENABLED                               // I2C scan report
  lcd.setCursor(0,3);
  if      (i2c_status==1) lcd.print(F("AD7991-0 detected   "));
  else if (i2c_status==2) lcd.print(F("AD7991-1 detected   "));
  else                    lcd.print(F("Using built-in A/D  "));
  delay(1000);	
  #endif
 
  mode.disp = DEFAULT_MODE;                      // Set default Display Mode
  flag.mode_change = TRUE;                       // Force a Display of Mode Intro when starting up
  flag.mode_display = TRUE;

  virt_lcd_clear();                              // Prep for going live: LCD clear using the Virtual LCD code
                                                 // for paced print, in order not to interfere with AD sample timing 
}

