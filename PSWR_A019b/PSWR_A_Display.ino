//*********************************************************************************
//**
//** Project.........: A menu driven Multi Display RF Power and SWR Meter
//**                   using a Tandem Match Coupler and 2x AD8307.
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

//*********************************************************************************
//**
//**  The Bargraph function lcdProgressBarPeak() below relies on a direct swipe
//**  from the AVRLIB lcd.c/h by Pascal Stang.
//**
//**  The original Bargraph function has been modified as follows:
//**
//**  A new Peak Bar (sticky bar) functionality has been added.
//**  A different set of symbols to construct the bars, simulating an N8LP style bargraph.
//**  Adaptation for use with the LicuidCrystalFast library and the virt_lcd_write function
//**
//**                  See the AVRLIB copyright notice below.
//**
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
/// (about 90% of character LCDs use one of these chips).† The display driver
/// can interface to the display through the CPU memory bus, or directly via
/// I/O port pins.† When using the direct I/O port mode, no additional
/// interface hardware is needed except for a contrast potentiometer.
///†Supported functions include initialization, clearing, scrolling, cursor
/// positioning, text writing, and loading of custom characters or icons
/// (up to 8).† Although these displays are simple, clever use of the custom
/// characters can allow you to create animations or simple graphics.† The
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


//
//-----------------------------------------------------------------------------------------
// 20x4 LCD Print Routines
//
// LCD Printing is done as a two step process:
//
// 1) Data is printed to a virtual LCD consisting of an 80 character long string; 
//
// 2) The string is then printed piecemeal, in a round robin fashion, to the real 20x4 LCD,
//    one character per 333 microseconds approximately.
//
// The piecemeal print is to ensure no undue delays associated with servicing the LCD,
// such delays would translate to uneven meter sampling rate
//-----------------------------------------------------------------------------------------
//

char    virt_lcd[82];     // virtual LCD string (20x4 + 2 chars for safety margin)
uint8_t virt_x, virt_y;   // x and y coordinates for LCD

//
//-----------------------------------------------------------------------------------------
//      Move characters to LCD from virtual LCD
//      This function is called once every time the ADs are sampled for Fwd and Ref power
//
//      3 character are transferred to the LCD per one millisecond
//-----------------------------------------------------------------------------------------
//
void virt_LCD_to_real_LCD(void)
{
  static int8_t LCD_pos;

  for (uint8_t i=0; i<(3*SAMPLE_TIME); i++) // Print three chars to LCD per millisecond
  {
    if (LCD_pos==-1) lcd.setCursor(0,0);    // If at beginning of virt LCD, Set LCD position at 0,0,
                                            // should be unnecessary, but belt and braces is good :)
    else
    {
      lcd.write(virt_lcd[LCD_pos]);         // Print one oharacter
    }
    LCD_pos++;
    if (LCD_pos == 80) LCD_pos = -1;        // At end, wrap around to beginning 
  }
}

//
//-----------------------------------------------------------------------------------------
//      Print to a Virtual LCD - 80 character long string representing a 20x4 LCD
//-----------------------------------------------------------------------------------------
//
void virt_lcd_write(char ch)
{
  uint8_t virt_pos;
  
  // Print to the 20x4 virtual LCD
  virt_pos = virt_x + 20*virt_y;          // Determine position on virt LCD
  virt_lcd[virt_pos++] = ch;              // Place character on virt LCD
  if (virt_pos >= 80) virt_pos = 0;       // At end, wrap around to beginning 

  // After print, derive new x,y coordinates in our 20 by 4 matrix
  virt_x = virt_pos;
  for(virt_y = 0; virt_y < 4 && virt_x >= 20;virt_y++)         
  {
    virt_x -=20;
  }
}
void virt_lcd_print(const char *ch_in)
{
  //uint8_t virt_pos;
  uint8_t virt_len;
  
  // Print to the 20x4 virtual LCD
  virt_len = strlen(ch_in);
  for (uint8_t i = 0; i < virt_len; i++)
  {
    virt_lcd_write(ch_in[i]);
  }
}

//
//-----------------------------------------------------------------------------------------
//      SetCursor virtual LCD
//-----------------------------------------------------------------------------------------
//
void virt_lcd_setCursor(uint8_t x, uint8_t y)
{
  virt_x = x;
  virt_y = y;
}


//
//-----------------------------------------------------------------------------------------
//      Clear virtual LCD
//-----------------------------------------------------------------------------------------
//
void virt_lcd_clear(void)
{
  for (uint8_t i = 0; i < 80; i++) virt_lcd[i] = ' ';  // Print a lot of spaces to virt LCD
  virt_x = 0;
  virt_y = 0;
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
void lcdProgressBarPeak(uint16_t progress, uint16_t prog_peak, uint16_t maxprogress, uint8_t len)
{

  # define PROGRESSPIXELS_PER_CHAR 6                    // progress bar defines
  uint16_t i;
  uint16_t pixelprogress;
  uint8_t c;

  if (progress >= maxprogress) progress = maxprogress;  // Clamp the upper bound to prevent funky readings

  // draw a progress bar displaying (progress / maxprogress)
  // starting from the current cursor position
  // with a total length of "length" characters
  // ***note, LCD chars 0-6 must be programmed as the bar characters
  // char 0 = empty ... char 5 = full, char 6 = peak bar - disabled if maxprogress set as 0 (or lower than progress)

  // total pixel length of bargraph equals length*PROGRESSPIXELS_PER_CHAR;
  // pixel length of bar itself is
  pixelprogress = ((uint32_t) (progress*(len*PROGRESSPIXELS_PER_CHAR)/maxprogress));
	
  // print exactly "length" characters
  for(i=0; i<len; i++)
  {
    // check if this is a full block, or partial or empty
    if( ((i*PROGRESSPIXELS_PER_CHAR)+PROGRESSPIXELS_PER_CHAR) > pixelprogress )
    {
      // this is a partial or empty block
      if( ((i*PROGRESSPIXELS_PER_CHAR)) > pixelprogress )
      {
        // If an otherwise empty block contains previous "Peak", then print peak char
        // If this function is not desired, simply set prog_peak at 0 (or as equal to progress)
        if(i == ((uint32_t)len * prog_peak)/maxprogress)
        {
          c = 6;
        }				
        // othwerwise this is an empty block
        else
        {
          c = 0;
        }
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
    virt_lcd_write(c);
  }
}


//-----------------------------------------------------------------------------------------
// custom LCD characters for Bargraph
const uint8_t LcdCustomChar[7][8] =
      {
        // N8LP LP-100 look alike bargraph
        { 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00 }, // 0. 0/5 full progress block
        { 0x00, 0x10, 0x10, 0x15, 0x10, 0x10, 0x00, 0x00 }, // 1. 1/5 full progress block
        { 0x00, 0x18, 0x18, 0x1d, 0x18, 0x18, 0x00, 0x00 }, // 2. 2/5 full progress block
        { 0x00, 0x1c, 0x1c, 0x1d, 0x1C, 0x1c, 0x00, 0x00 }, // 3. 3/5 full progress block
        { 0x00, 0x1e, 0x1e, 0x1E, 0x1E, 0x1e, 0x00, 0x00 }, // 4. 4/5 full progress block
        { 0x00, 0x1f, 0x1f, 0x1F, 0x1F, 0x1f, 0x00, 0x00 }, // 5. 5/5 full progress block
        { 0x06, 0x06, 0x06, 0x16, 0x06, 0x06, 0x06, 0x06 }  // 6. Peak Bar
      };

//-----------------------------------------------------------------------------------------
// Initialize LCD for bargraph display - Load 6 custom bargraph symbols and a PeakBar symbol
//-----------------------------------------------------------------------------------------
void lcd_bargraph_Init(void)
{
  for (uint8_t i=0; i<7; i++)
  {
    lcd.createChar(i, (uint8_t*) LcdCustomChar[i]);
  }
}


//
//-----------------------------------------------------------------------------
//  Screensaver display
//-----------------------------------------------------------------------------
//
void screensaver(void)
{
  uint8_t x, y;
  static uint8_t count = SLEEPTIME-1;    // Force screensaver print when run first time

  count++;

  // Force screensaver reprint if flag
  if (flag.idle_refresh == TRUE)
  {
    flag.idle_refresh = FALSE;          // Clear flag
    count = SLEEPTIME;                  // and force a reprint
  }

  if (count == SLEEPTIME)               // We reprint the output every fiftieth time (approx 5 sec)
  {
    count = 0;
    virt_lcd_clear();
    y = rand() % 4;
    x = rand() % (21 - strlen(R.idle_disp));
    virt_lcd_setCursor(x,y);
    virt_lcd_print(R.idle_disp);
  }
}


//
//-----------------------------------------------------------------------------
//  Display Mode Intro
//-----------------------------------------------------------------------------
//
void lcd_display_mode_intro(const char *line1, const char *line2, const char *line3, const char *line4)
{
  virt_lcd_clear();
  virt_lcd_setCursor(0,0);
  virt_lcd_print(line1);
  virt_lcd_setCursor(0,1);
  virt_lcd_print(line2);
  virt_lcd_setCursor(0,2);
  virt_lcd_print(line3);
  virt_lcd_setCursor(0,3);
  virt_lcd_print(line4);
}


//
//-----------------------------------------------------------------------------------------
//  Scale Bargraph based on highest instantaneous power reading during last 10 seconds
//  (routine is called once every 1/10th of a second)
//
//  The scale ranges are user definable, up to 3 ranges per decade
//    e.g. 6, 12 and 24 gives:
//    ... 6mW, 12mW, 24mW, 60mW ... 1.2W, 2.4W, 6W 12W 24W 60W 120W ...
//    If all three values set as "2", then: 
//    ... 2W, 20W, 200W ...
//    The third and largest value has to be less than ten times the first value 
//-----------------------------------------------------------------------------------------
//
uint32_t scale_BAR(uint32_t pow /* power in milliwatts */)
{
#define	SCALE_BUFFER	100				// Keep latest 100 power readings in buffer

  // For measurement of peak and average power
  static uint32_t peak_buff[SCALE_BUFFER];// 10 second window
  uint32_t max;							// Find max value
  uint32_t decade = 1;					// Determine range decade
  uint32_t scale;							// Scale output

  static uint8_t a;						// Entry counter

  // Store PEP value in ring buffer
  peak_buff[a] = pow;
  a++;
  if (a == SCALE_BUFFER) a = 0;

  // Retrieve the max value out of the measured window
  max = 0;
  for (uint8_t b = 0; b < SCALE_BUFFER; b++)
  {
    if (max < peak_buff[b]) max = peak_buff[b];
  }

  // Determine range decade
  while ((decade * R.ScaleRange[2]) < max)
  {
    decade = decade * 10;
  }

  // And determine scale limit to use, within the decade previously determined
  if		(max >= (decade * R.ScaleRange[1]))	scale = decade * R.ScaleRange[2];
  else if	(max >= (decade * R.ScaleRange[0]))	scale = decade * R.ScaleRange[1];
  else 	scale = decade * R.ScaleRange[0];

  return scale;							// Return value is in mW
}


//
//-----------------------------------------------------------------------------------------
//  Scale Values to fit Power Bargraph (16 bit integer)
//-----------------------------------------------------------------------------------------
//
void scale_PowerBarInpValues(uint32_t fullscale, double bar, double peak_bar, 
uint16_t *out_fullscale, uint16_t *out_bar, uint16_t *out_peak)
{
  uint32_t baradj;
  uint32_t peakadj;

  // Convert variables to integer and scale them to fit 16bit integers
  // used in the input variables to lcdProgressBarPeak()

  // If we have 10 mW or more...	
  if (fullscale >= 10)
  {
    baradj = (uint32_t) bar;
    peakadj = (uint32_t) peak_bar;

    // Scale down the bar graph resolution to fit 16 bit integer
    while (fullscale >= 0x10000)
    {
      fullscale = fullscale/0x10;
      baradj = baradj/0x10;
      peakadj = peakadj/0x10;
    }
  }

  // If power is under 10mW, then crank the bar graph resolution up by 0x10
  else
  {
    fullscale = fullscale * 0x10;
    baradj = (uint32_t) (bar * 0x10);
    peakadj = (uint32_t) (peak_bar * 0x10);
  }
  // Copy adjusted values into 16 bit variables
  *out_fullscale = fullscale;
  *out_bar = baradj;
  *out_peak = peakadj;	
}




//
//-----------------------------------------------------------------------------
//  Display: Bargraph, Power in Watts, SWR & PEP Power
//  PEP Power always displayed and used for scale selection
//  power variable can be anything passed to function (power_mv_avg, power_mw_pk, etc...)
//-----------------------------------------------------------------------------
//
void lcd_display_clean(const char * introtext, const char * power_display_indicator, double power)
{
  static uint8_t mode_timer = 0;           // Used to time the Display Mode intro

  int32_t scale;                           // Progress bar scale

    // Used to scale/fit progressbar inputs into 16 bit variables, when needed
  uint16_t bar_power=0, bar_power_pep=0, bar_scale=0;	

  scale = scale_BAR((uint32_t) power_mw);  // Determine scale setting, also used to
  // determine if level above useful threshold

  //------------------------------------------
  // Display mode intro for a time
  if(flag.mode_display == TRUE)
  {
    if(flag.mode_change == TRUE)
    {
      flag.mode_change = FALSE;            // Clear display change mode
      mode_timer = 0;						// New mode, reset timer
      lcd_display_mode_intro("Mode:"," ",introtext,"and SWR Bargraph");
    }

    mode_timer++;
    if (mode_timer >= MODE_INTRO_TIME)     // MODE_INTRO_TIME in tenths of seconds
    {
      mode_timer = 0;
      flag.mode_display = FALSE;           // Clear display change mode
      flag.idle_refresh = TRUE;            // Force screensaver reprint upon exit, if screensaver mode
    }
  }

  //----------------------------------------------
  // Display Power if level is above useful threshold
  //
  // If input power above a minimum relevant value is detected (see PSWR_A.h)
  // or if scale indicates a value higher than 10mW
  // (= significant power within last 10 seconds)
  else if ((power_mw > R.idle_disp_thresh) || (scale > 10))
  {
    //------------------------------------------
    // Power indication and Bargraph
    virt_lcd_setCursor(0,0);

    //------------------------------------------
    // Scale variables to fit 16bit input to lcdProgressBarPeak()
    scale_PowerBarInpValues(scale, power, power_mw_pep, &bar_scale, &bar_power, &bar_power_pep);
    //------------------------------------------
    // Power Bargraph
    lcdProgressBarPeak(bar_power,bar_power_pep,bar_scale, 20);

    //------------------------------------------
    // SWR Bargraph
    virt_lcd_setCursor(0,1);
    lcdProgressBarPeak(swr_bar,0,1000, 20);

    //------------------------------------------
    // SWR Printout
    virt_lcd_setCursor(7,2);            // Clear junk in line, if any
    virt_lcd_print("   ");
    virt_lcd_setCursor(0,2);
    virt_lcd_print("SWR ");
    print_swr();                        // and print the "SWR value"
    virt_lcd_print(lcd_buf);

    //------------------------------------------
    // Scale Indication
    virt_lcd_setCursor(8,3);            // Ensure last couple of chars in line are clear
    virt_lcd_print("  ");
    virt_lcd_setCursor(0,3);
    virt_lcd_print("Scale");
    #if AD8307_INSTALLED
    print_p_reduced((double)scale);   // Scale Printout
    #else
    print_p_reduced(scale);           // Scale Printout
    #endif
    virt_lcd_print(lcd_buf);

    /* //------------------------------------------
       // Modulation Index
       virt_lcd_setCursor(8,3);         // Ensure last couple of chars in line are clear
       virt_lcd_setCursor("  ");
       virt_lcd_setCursor(0,3);
       sprintf(lcd_buf,"Index %2.02f", modulation_index);
       virt_lcd_setCursor(lcd_buf);
    */
    //------------------------------------------
    // Power Indication
    virt_lcd_setCursor(19,2);           // Clear junk in line, if any
    virt_lcd_print(" ");
    virt_lcd_setCursor(10,2);           // Clear junk in line, if any
    virt_lcd_print(" ");
    virt_lcd_print(power_display_indicator);
    if (Reverse)                        // If reverse power, then indicate
    {
      virt_lcd_setCursor(13,2);
      virt_lcd_print("-");
    }
    virt_lcd_setCursor(14,2);
    #if AD8307_INSTALLED
    print_p_mw(power);
    #else
    print_p_mw((int32_t) power);
    #endif
    
    virt_lcd_print(lcd_buf);

    //------------------------------------------
    // Power indication, 1 second PEP
    virt_lcd_setCursor(19,3);           // Clear junk in line, if any
    virt_lcd_print(" ");
    virt_lcd_setCursor(10,3);           // Clear junk in line, if any
    virt_lcd_print(" pep");
    virt_lcd_setCursor(14,3);
    print_p_mw(power_mw_pep);
    virt_lcd_print(lcd_buf);
  }

  else
    screensaver();                      // Screensaver display
}

//
//-----------------------------------------------------------------------------
//  Display: Bargraph, Power in dBm, SWR & PEP Power
//-----------------------------------------------------------------------------
//
void lcd_display_clean_dBm(void)
{
  static uint8_t mode_timer = 0;        // Used to time the Display Mode intro

  int32_t scale;                        // Progress bar scale

    // Used to scale/fit progressbar inputs into 16 bit variables, when needed
  uint16_t bar_power=0,bar_power_pep=0,bar_scale=0;	

  scale = scale_BAR((uint32_t) power_mw);  // Determine scale setting, also used to
  // determine if level above useful threshold

  //------------------------------------------
  // Display mode intro for a time
  if(flag.mode_display == TRUE)
  {
    if(flag.mode_change == TRUE)
    {
      flag.mode_change = FALSE;         // Clear display change mode
      mode_timer = 0;                   // New mode, reset timer
      lcd_display_mode_intro("Mode:"," ","dBm Meter,  Power","and SWR Bargraph");
    }

    mode_timer++;
    if (mode_timer >= MODE_INTRO_TIME)  // MODE_INTRO_TIME in tenths of seconds
    {
      mode_timer = 0;
      flag.mode_display = FALSE;        // Clear display change mode
      flag.idle_refresh = TRUE;         // Force screensaver reprint upon exit, if screensaver mode
    }
  }

  //----------------------------------------------
  // Display Power if level is above useful threshold
  //
  // If input power above a minimum relevant value is detected (see PSWR_A.h)
  // or if scale indicates a value higher than 10mW
  // (= significant power within last 10 seconds)
  else if ((power_mw > R.idle_disp_thresh) || (scale > 10))
  {
    //------------------------------------------
    // Power indication and Bargraph
    virt_lcd_setCursor(0,0);

    //------------------------------------------
    // Scale variables to fit 16bit input to lcdProgressBarPeak()
    scale_PowerBarInpValues(scale, power_mw, power_mw_pep, &bar_scale, &bar_power, &bar_power_pep);
    //------------------------------------------
    // Power Bargraph
    lcdProgressBarPeak(bar_power,bar_power_pep,bar_scale, 20);

    //------------------------------------------
    // SWR Bargraph
    virt_lcd_setCursor(0,1);
    lcdProgressBarPeak(swr_bar,0,1000, 20);

    //------------------------------------------
    // SWR Printout
    virt_lcd_setCursor(7,2);            // Clear junk in line, if any
    virt_lcd_print("   ");
    virt_lcd_setCursor(0,2);
    virt_lcd_print("SWR ");
    print_swr();                        // and print the "SWR value"
    virt_lcd_print(lcd_buf);

    //------------------------------------------
    // Scale Indication
    virt_lcd_setCursor(8,3);            // Ensure last couple of chars in line are clear
    virt_lcd_print("  ");
    virt_lcd_setCursor(0,3);
    virt_lcd_print("Scale");
    #if AD8307_INSTALLED
    print_p_reduced((double)scale);   // Scale Printout
    #else
    print_p_reduced(scale);           // Scale Printout
    #endif
    virt_lcd_print(lcd_buf);

    //------------------------------------------
    // Power Indication
    virt_lcd_setCursor(10,2);           // Clear junk in line, if any
    virt_lcd_print(" ");
    virt_lcd_setCursor(15,2);           // Clear junk in line, if any
    virt_lcd_print("     ");
    if (Reverse)                        // If reverse power, then indicate
    {
      virt_lcd_setCursor(19,2);
      virt_lcd_print("-");
    }
    virt_lcd_setCursor(10,2);
    print_dbm(power_db*10.0);
    virt_lcd_print(lcd_buf);

    //------------------------------------------
    // Power indication, PEP
    virt_lcd_setCursor(10,3);           // Clear junk in line, if any
    virt_lcd_print(" ");
    virt_lcd_setCursor(17,3);
    virt_lcd_print("  P");
    virt_lcd_setCursor(10,3);
    print_dbm(power_db_pep*10.0);
    virt_lcd_print(lcd_buf);
  }

  else
    screensaver();                      // Screensaver display
}


//
//-----------------------------------------------------------------------------
//  Display Forward and Reflected Power, SWR and R + jX
//  (input power is in mW)
//-----------------------------------------------------------------------------
//
void lcd_display_mixed()
{
  static uint8_t mode_timer = 0;        // Used to time the Display Mode intro

  int32_t scale;                        // Progress bar scale

    // Used to scale/fit progress bar inputs into 16 bit variables, when needed
  uint16_t fwd_bar_power=0, rev_bar_power=0, bar_scale=0;

  scale = scale_BAR((uint32_t) fwd_power_mw);  // Determine scale setting, also used to
  // determine if level above useful threshold

  //------------------------------------------
  // Display mode intro for a time
  if(flag.mode_display == TRUE)
  {
    if(flag.mode_change == TRUE)
    {
      flag.mode_change = FALSE;         // Clear display change mode
      mode_timer = 0;                   // New mode, reset timer
      lcd_display_mode_intro("Mode:","Forward and","     Reflected Power","Bargraph Meter");
    }

    mode_timer++;
    if (mode_timer >= MODE_INTRO_TIME)  // MODE_INTRO_TIME in tenths of seconds
    {
      mode_timer = 0;
      flag.mode_display = FALSE;        // Clear display change mode
      flag.idle_refresh = TRUE;         // Force screensaver reprint upon exit, if screensaver mode
    }
  }

  //----------------------------------------------
  // Display Power if level is above useful threshold
  //
  // If input power above a minimum relevant value is detected (see PSWR_A.h)
  // or if scale indicates a value higher than 10mW
  // (= significant power within last 10 seconds)
  #if AD8307_INSTALLED
  else if ((power_mw > R.idle_disp_thresh) || (scale > 10))
  #else  
  else if ((power_mw > MIN_PWR_FOR_METER) || (scale > 10))
  #endif
  {
    //------------------------------------------
    // Power indication and Bar graph

    //------------------------------------------
    // Scale variables to fit 16bit input to lcdProgressBarPeak()
    scale_PowerBarInpValues(scale, fwd_power_mw, ref_power_mw, &bar_scale, &fwd_bar_power, &rev_bar_power);

    // Forward Power Bargraph
    virt_lcd_setCursor(0,0);
    // As the higher power is always kept in the fwd variable, we need to make sure that the correct
    // variable is selected for display, based on direction.
    if (!Reverse) lcdProgressBarPeak(fwd_bar_power, 0 /* no PEP */, bar_scale, 14);
    else lcdProgressBarPeak(rev_bar_power, 0 /* no PEP */, bar_scale, 14);

    //------------------------------------------
    // Wattage Printout
    print_p_mw(fwd_power_mw);
    virt_lcd_print(lcd_buf);

    //------------------------------------------
    // Reverse Power Bargraph
    virt_lcd_setCursor(0,1);
    // As the higher power is always kept in the fwd variable, we need to make sure that the correct
    // variable is selected for display, based on direction.
    if (!Reverse) lcdProgressBarPeak(rev_bar_power, 0 /* no PEP */, bar_scale, 14);
    else lcdProgressBarPeak(fwd_bar_power, 0 /* no PEP */, bar_scale, 14);
    //------------------------------------------
    // Wattage Printout
    print_p_mw(ref_power_mw);
    virt_lcd_print(lcd_buf);

    //------------------------------------------
    // SWR Printout
    virt_lcd_setCursor(7,2);            // Clear junk in line, if any
    virt_lcd_print("   ");
    virt_lcd_setCursor(0,2);
    virt_lcd_print("SWR ");
    print_swr();                        // and print the "SWR value"
    virt_lcd_print(lcd_buf);

    //------------------------------------------
    // Scale Indication
    virt_lcd_setCursor(8,3);            // Ensure last couple of chars in line are clear
    virt_lcd_print("  ");
    virt_lcd_setCursor(0,3);
    virt_lcd_print("Scale");
    #if AD8307_INSTALLED
    print_p_reduced((double)scale);   // Scale Printout
    #else
    print_p_reduced(scale);           // Scale Printout
    #endif
    virt_lcd_print(lcd_buf);

    //------------------------------------------
    // Forward Power Indication, dBm
    virt_lcd_setCursor(10,2);           // Clear junk in line, if any
    virt_lcd_print(" ");
    virt_lcd_setCursor(15,2);           // Clear junk in line, if any
    virt_lcd_print("     ");
    /*
      if (Reverse)                      // If reverse power, then indicate
      {
        virt_lcd_setCursor(19,2);
        virt_lcd_print("-");
      }
    */
    virt_lcd_setCursor(10,2);
    print_dbm(fwd_power_db*10.0);
    virt_lcd_print(lcd_buf);

    //------------------------------------------
    // Reflected Power Indication, dBm
    virt_lcd_setCursor(10,3);           // Clear junk in line, if any
    virt_lcd_print(" ");
    virt_lcd_setCursor(15,3);           // Clear junk in line, if any
    virt_lcd_print("     ");
    /*
      if (Reverse)                      // If reverse power, then indicate
      {
     	virt_lcd_setCursor(19,3);
     	virt_lcd_print("-");
      }
    */
    virt_lcd_setCursor(10,3);
    print_dbm(ref_power_db*10.0);
    virt_lcd_print(lcd_buf);
  }	
  else
    screensaver();                      // Screensaver display
}


//
//-----------------------------------------------------------------------------
//  Display Config and measured input voltages etc...
//-----------------------------------------------------------------------------
//
void lcd_display_debug(void)
{
  double    output_voltage;
  uint16_t  power, power_sub;

  virt_lcd_clear();

  //lcd_gotoxy(0,0);
  //lcd_puts_P("Dbg:");

  //------------------------------------------
  // AD8307 forward indication
  virt_lcd_setCursor(0,0);
  output_voltage = fwd * 2.56 / 4096;
  power_sub = output_voltage * 1000;
  power = power_sub / 1000;
  power_sub = power_sub % 1000;
  sprintf(lcd_buf,"%4u %u.%03uV ", fwd, power, power_sub);
  virt_lcd_print(lcd_buf);
  //------------------------------------------
  // AD8307 reverse indication
  virt_lcd_setCursor(0,1);
  output_voltage = ref * 2.56 / 4096;
  power_sub = output_voltage * 1000;
  power = power_sub / 1000;
  power_sub = power_sub % 1000;
  sprintf(lcd_buf,"%4u %u.%03uV ", ref, power, power_sub);
  virt_lcd_print(lcd_buf);
  //------------------------------------------
  // Calibrate 1
  virt_lcd_setCursor(0,2);
  sprintf (lcd_buf,"%4d ",R.cal_AD[0].db10m);
  virt_lcd_print(lcd_buf);
  sprintf (lcd_buf,"%4u,%4u",R.cal_AD[0].Fwd,R.cal_AD[0].Rev);
  virt_lcd_print(lcd_buf);
  //------------------------------------------
  // Calibrate 2
  virt_lcd_setCursor(0,3);
  sprintf (lcd_buf,"%4d ",R.cal_AD[1].db10m);
  virt_lcd_print(lcd_buf);
  sprintf (lcd_buf,"%4u,%4u",R.cal_AD[1].Fwd,R.cal_AD[1].Rev);
  virt_lcd_print(lcd_buf);
}

