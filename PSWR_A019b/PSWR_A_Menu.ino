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



// First Level Menu Items

#if AD8307_INSTALLED
const uint8_t level0_menu_size = 9;
#else
const uint8_t level0_menu_size = 8;
#endif
const char *level0_menu_items[] =
          {  "1 SWR Alarm",
             "2 SWR Alarm Power",
             "3 PEP Period",
             #if AD8307_INSTALLED
             "4 Screensaver",
             "5 Scale Ranges",					
             "6 Calibrate",
             "7 Debug Display",
             "8 Reset to Default",
             #else
             "4 Scale Ranges",					
             "5 Calibrate",
             "6 Debug Display",
             "7 Reset to Default",             
             #endif
             "0 Exit"  };

// Flags for SWR Alarm & Threshold menu
#define SWR_ALARM_THRESHOLD      1
#define	SWR_ALARM_PWR_THRESHOLD  2
// SWR Alarm Power Threshold menu Items
const uint8_t swr_alarm_pwr_thresh_menu_size = 5;
const char *swr_alarm_pwr_thresh_menu_items[] =
          {  "1   1mW",
             "2  10mW",
             "3  0.1W",
             "4    1W",
             "5   10W"  };

// Flag for PEP Sample Period select
#define PEP_MENU                 3
// PEP Sample Period select menu Items
const uint8_t pep_menu_size = 3;
const char *pep_menu_items[] =
          {  "1    1s",
             "2  2.5s",
             "3    5s"  };

// Flag for Encoder Resolution Change
//#define ENCODER_MENU	8


#if AD8307_INSTALLED
// Flag for Screensaver Threshold select menu
#define SCREENTHRESH_MENU	5
// Screensaver Threshold select menu Items
const uint8_t screenthresh_menu_size = 6;
const char *screenthresh_menu_items[] =
          {  "1    Off",
             "2    1uW",
             "3   10uW",
             "4  100uW",
             "5    1mW",
             "6   10mW"  };
#endif					

// Flag for Scale Range menu
#define SCALERANGE_MENU         6

// Flags for Scale Range Submenu functions
#define SCALE_SET0_MENU	600
#define SCALE_SET1_MENU	601
#define SCALE_SET2_MENU	602

// Flag for Serial Data Out
#define SERIAL_MENU             7
// Serial menu Items
const uint8_t serial_menu_size = 2;
const char *serial_menu_items[] =
          {  "1  Off",
             "2  On"  };

// Flag for Calibrate menu
#define CAL_MENU                8
#if AD8307_INSTALLED
const uint8_t calibrate_menu_size = 5;
const char *calibrate_menu_items[] =
          {  "1 OneLevelCal(dBm)",
             "2  1st level (dBm)",
             "3  2nd level (dBm)",
             "9 Go Back",
             "0 Exit"  };

// Flags for Calibrate Submenu functions
#define CAL_SET0_MENU         800  // Single level calibration
#define CAL_SET1_MENU         801  // 1st level
#define CAL_SET2_MENU         802  // 2nd level
#endif

// Flag for Debug Screen
#define DEBUG_MENU              9

// Flag for Factory Reset
#define FACTORY_MENU           11
// Factory Reset menu Items
const uint8_t factory_menu_size = 3;
const char *factory_menu_items[] =
          {  "1 No  - Go back",
             "2 Yes - Reset",
             "0 No  - Exit"  };


uint16_t  menu_level = 0;          // Keep track of which menu we are in
uint8_t   menu_data = 0;           // Pass data to lower menu

int8_t    gain_selection;          // keep track of which GainPreset is currently selected

var_t     eeprom_R;                // Used to read and compare EEPROM R with current R
//
//--------------------------------------------------------------------
// Multipurpose Pushbutton 
//
// Returns 0 for no push,  1 for short push and 2 for long push
// (routine should be called once every 100ms)
//--------------------------------------------------------------------
//
uint8_t multipurpose_pushbutton(void)
{
  static uint8_t pushcount=0;      // Measure push button time (max 2.5s)
  uint8_t        retval=0;         // 1 for short push, 2 for long push\
  
  //-------------------------------------------------------------------    
  // SW1: Enact Long Push (pushbutton has been held down for a certain length of time):
  //
  if (pushcount >= ENACT_MAX)      // "Long Push", goto Configuration Mode
  {
    mode.conf = TRUE;              // Switch into Configuration Menu, while		
                                   // retaining memory of runtime function
    flag.long_push = TRUE;         // Used with Configuraton Menu functions	
    pushcount = 0;                 // Initialize push counter for next time
    retval = 2;
  }
    
  //-------------------------------------------------------------------    
  // Enact Short Push (react on release if only short push)
  //
  else if (digitalRead(EnactSW) == HIGH)   // Pin high = just released, or not pushed
  {
    // Do nothing if this is a release after Long Push
    if (flag.long_push == TRUE)            // Is this a release following a long push?
    {
      flag.long_push = FALSE;              // Clear pushbutton status
    }
    // Do stuff on command
    else if (pushcount >= ENACT_MIN)       // Check if this is more than a short spike
    {	
      flag.short_push = TRUE;              // Used with Configuraton Menu functions
      retval = 1;
    }
    pushcount = 0;                         // Initialize push counter for next time
  }  

  //-------------------------------------------------------------------
  // Determine whether Long Push or Short Push (while pushbutton is down)
  //
  else if (flag.long_push == FALSE)       // Button Pushed, count up the push timer
  {                                       // (unless this is tail end of a long push,
    pushcount++;                          //  then do nothing)
  }
  return retval;
}

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
  virt_lcd_setCursor(begin_col, begin_row);
  for (a = begin_col; a < 20; a++)
    virt_lcd_print(" ");
  if (lines > 1)
  {
    virt_lcd_setCursor(begin_col, begin_row+1);
    for (a = begin_col; a < 20; a++)
      virt_lcd_print(" ");
  }
  if (lines > 2)
  {
    virt_lcd_setCursor(begin_col, begin_row+2);
    for (a = begin_col; a < 20; a++)
      virt_lcd_print(" ");
  }
  // Using Menu list pointed to by **menu, preformat for print:
  // First line contains previous choice, secon line contains
  // current choice preceded with a '->', and third line contains
  // next choice
  if (current_choice == 0) x = menu_size - 1;
  else x = current_choice - 1;
  if (lines > 1)
  {
    virt_lcd_setCursor(begin_col + 2, begin_row);
    sprintf(lcd_buf,"%s", *(menu + x));
    virt_lcd_print(lcd_buf);
	
    virt_lcd_setCursor(begin_col, begin_row + 1);
    sprintf(lcd_buf,"->%s", *(menu + current_choice));
    virt_lcd_print(lcd_buf);
    if (current_choice == menu_size - 1) x = 0;
    else x = current_choice + 1;
  
    virt_lcd_setCursor(begin_col + 2, begin_row + 2);
    sprintf(lcd_buf,"%s", *(menu + x));
    virt_lcd_print(lcd_buf);
  }
  else
  {
    virt_lcd_setCursor(begin_col, begin_row);
    sprintf(lcd_buf,"->%s", *(menu + current_choice));
    virt_lcd_print(lcd_buf);
  }
  // LCD print lines 1 to 3
  
  // 4 line display.  Preformat and print the fourth line as well
  if (lines == 4)
  {
    if (current_choice == menu_size-1) x = 1;
    else if (current_choice == menu_size - 2 ) x = 0;
    else x = current_choice + 2;
    virt_lcd_setCursor(begin_col, begin_row+3);
    for (a = begin_col; a < 20; a++)
    virt_lcd_print(" ");
    virt_lcd_setCursor(begin_col + 2, begin_row + 3);
    sprintf(lcd_buf,"  %s", *(menu + x));
    virt_lcd_print(lcd_buf);
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
  lcd_display_debug();                  // Display Config and measured input voltages etc...

  // Exit on Button Push
  if (flag.short_push == TRUE)
  {
    flag.short_push = FALSE;            // Clear pushbutton status

    virt_lcd_clear();
    virt_lcd_setCursor(0,1);				
    virt_lcd_print("Nothing Changed");
    Menu_disp_timer = 30;               // Show on LCD for 3 seconds
    mode.conf = FALSE;                  // We're done, EXIT
    menu_level = 0;                     // We are done with this menu level
  }
}


//--------------------------------------------------------------------
// SWR Alarm Threshold Set Menu
//--------------------------------------------------------------------
void swr_alarm_threshold_menu(void)
{
  // Selection modified by encoder.  We remember last selection, even if exit and re-entry
  if (Enc.read()/ENC_RESDIVIDE != 0)
  {
    if ((Enc.read()/ENC_RESDIVIDE > 0) && (R.SWR_alarm_trig < 40) )  // SWR 4:1 is MAX value, and equals no alarm
    {
      R.SWR_alarm_trig++;
    }
    if ((Enc.read()/ENC_RESDIVIDE < 0) && (R.SWR_alarm_trig > 15) )  // SWR of 1.5:1 is MIN value for SWR alarm
    {
      R.SWR_alarm_trig--;
    }
    // Reset data from Encoder
    Enc.write(0);

    // Indicate that an LCD update is needed
    flag.menu_lcd_upd = FALSE;            // Keep track of LCD update requirements
  }

  // If LCD update is needed
  if (flag.menu_lcd_upd == FALSE)
  {
    flag.menu_lcd_upd = TRUE;            // We have serviced LCD

    virt_lcd_clear();

    virt_lcd_setCursor(0,0);
    virt_lcd_print("SWR Alarm Threshold:");
    virt_lcd_setCursor(0,1);
    virt_lcd_print("Adjust->   ");
    if (R.SWR_alarm_trig == 40)
    virt_lcd_print("Off");		
    else
    {
      sprintf(lcd_buf,"%1u.%01u",R.SWR_alarm_trig/10, R.SWR_alarm_trig%10);
      virt_lcd_print(lcd_buf);
    }					
    virt_lcd_setCursor(0,2);
    virt_lcd_print("Range is 1.5 to 3.9");
    virt_lcd_setCursor(0,3);
    virt_lcd_print("4.0 = SWR Alarm Off");
  }	
  // Enact selection by saving in EEPROM
  if (flag.short_push == TRUE)
  {
    flag.short_push = FALSE;             // Clear pushbutton status

    virt_lcd_clear();
    virt_lcd_setCursor(1,1);
    EEPROM_readAnything(1,eeprom_R);
    if (eeprom_R.SWR_alarm_trig != R.SWR_alarm_trig)  // New Value
    {
      EEPROM_writeAnything(1,R);
      virt_lcd_print("Value Stored");
    }
    else virt_lcd_print("Nothing Changed");

    Menu_disp_timer = 30;                // Show on LCD for 3 seconds
    mode.conf = FALSE;                   // We're done, EXIT
    menu_level = 0;                      // We are done with this menu level
    flag.menu_lcd_upd = FALSE;           // Make ready for next time
  }
}


//--------------------------------------------------------------------
// SWR Alarm Power Threshold Menu
//--------------------------------------------------------------------
void swr_alarm_power_threshold_menu(void)
{
  static int8_t	current_selection;

  // Get Current value
  // 1mW=>0, 10mW=>1, 100mW=>2, 1000mW=>3, 10000mW=>4
  // current_selection = log10(R.SWR_alarm_pwr_thresh); // This should have worked :(
  if      (R.SWR_alarm_pwr_thresh == 1) current_selection = 0;
  else if (R.SWR_alarm_pwr_thresh == 10) current_selection = 1;
  else if (R.SWR_alarm_pwr_thresh == 100) current_selection = 2;
  else if (R.SWR_alarm_pwr_thresh == 1000) current_selection = 3;
  else if (R.SWR_alarm_pwr_thresh == 10000) current_selection = 4;
	
  // Selection modified by encoder.  We remember last selection, even if exit and re-entry
  if (Enc.read()/ENC_RESDIVIDE != 0)
  {
    if (Enc.read()/ENC_RESDIVIDE > 0)
    {
      current_selection++;
    }
    else if (Enc.read()/ENC_RESDIVIDE < 0)
    {
      current_selection--;
    }
    // Reset data from Encoder
    Enc.write(0);;

    // Indicate that an LCD update is needed
    flag.menu_lcd_upd = FALSE;
  }

  // If LCD update is needed
  if (flag.menu_lcd_upd == FALSE)
  {
    flag.menu_lcd_upd = TRUE;            // We have serviced LCD

    // Keep Encoder Selection Within Bounds of the Menu Size
    uint8_t menu_size = swr_alarm_pwr_thresh_menu_size;
    while(current_selection >= menu_size)
      current_selection -= menu_size;
    while(current_selection < 0)
      current_selection += menu_size;

    // Update with currently selected value
    // R.SWR_alarm_pwr_thresh = pow(10,current_selection);  // Has roundoff error
    if      (current_selection == 0) R.SWR_alarm_pwr_thresh = 1;
    else if (current_selection == 1) R.SWR_alarm_pwr_thresh = 10;
    else if (current_selection == 2) R.SWR_alarm_pwr_thresh = 100;
    else if (current_selection == 3) R.SWR_alarm_pwr_thresh = 1000;
    else if (current_selection == 4) R.SWR_alarm_pwr_thresh = 10000;

    virt_lcd_clear();
    virt_lcd_setCursor(0,0);
    virt_lcd_print("SWR Alarm Power:");
    virt_lcd_setCursor(0,1);
    virt_lcd_print("Select");

    // Print the Rotary Encoder scroll Menu
    lcd_scroll_Menu((char**)swr_alarm_pwr_thresh_menu_items, menu_size, current_selection, 1, 6,1);

    virt_lcd_setCursor(0,2);
    virt_lcd_print("Available thresholds");
    virt_lcd_setCursor(0,3);
    virt_lcd_print("1mW 10mW 0.1W 1W 10W");
  }
	
  // Enact selection
  if (flag.short_push == TRUE)
  {
    virt_lcd_clear();
    virt_lcd_setCursor(0,1);

    flag.short_push = FALSE;             // Clear pushbutton status

    // Check if selected threshold is not same as previous
    EEPROM_readAnything(1,eeprom_R);
    if (eeprom_R.SWR_alarm_pwr_thresh != R.SWR_alarm_pwr_thresh)// New Value
    {
      EEPROM_writeAnything(1,R);
      virt_lcd_print("Value Stored");
    }
    else virt_lcd_print("Nothing Changed");

    Menu_disp_timer = 30;                // Show on LCD for 3 seconds
    mode.conf = FALSE;                   // We're done, EXIT
    menu_level = 0;                      // We are done with this menu level
    flag.menu_lcd_upd = FALSE;           // Make ready for next time
  }
}


//--------------------------------------------------------------------
// Peak Envelope Power (PEP) period selection Menu
//--------------------------------------------------------------------
void pep_menu(void)
{
  static int8_t	current_selection;

  // Get Current value
  if (R.PEP_period == 2500/SAMPLE_TIME) current_selection = 1;       // 2.5 seconds
  else if (R.PEP_period == 5000/SAMPLE_TIME) current_selection = 2;  // 5 seconds
  else current_selection = 0;            // Any other value, other than 1s, is invalid

  // Selection modified by encoder.  We remember last selection, even if exit and re-entry
  if (Enc.read()/ENC_RESDIVIDE != 0)
  {
    if (Enc.read()/ENC_RESDIVIDE > 0)
    {
      current_selection++;
    }
    else if (Enc.read()/ENC_RESDIVIDE < 0)
    {
      current_selection--;
    }
    // Reset data from Encoder
    Enc.write(0);

    // Indicate that an LCD update is needed
    flag.menu_lcd_upd = FALSE;
  }

  // If LCD update is needed
  if (flag.menu_lcd_upd == FALSE)
  {
    flag.menu_lcd_upd = TRUE;					// We have serviced LCD

    // Keep Encoder Selection Within Bounds of the Menu Size
    uint8_t menu_size = pep_menu_size;
    while(current_selection >= menu_size)
      current_selection -= menu_size;
    while(current_selection < 0)
      current_selection += menu_size;

    if      (current_selection == 1) R.PEP_period = 2500/SAMPLE_TIME;
    else if (current_selection == 2) R.PEP_period = 5000/SAMPLE_TIME;
    else R.PEP_period = 1000/SAMPLE_TIME;			

    virt_lcd_clear();
    virt_lcd_setCursor(0,0);
    virt_lcd_print("PEP sampling period:");
    virt_lcd_setCursor(0,1);
    virt_lcd_print("Select");

    // Print the Rotary Encoder scroll Menu
    lcd_scroll_Menu((char**)pep_menu_items, menu_size, current_selection, 1, 6,1);

    virt_lcd_setCursor(0,2);
    virt_lcd_print("Available periods");
    virt_lcd_setCursor(0,3);
    virt_lcd_print("1, 2.5 or 5 seconds");
  }

  // Enact selection
  if (flag.short_push == TRUE)
  {
    virt_lcd_clear();
    virt_lcd_setCursor(0,1);

    flag.short_push = FALSE;                    // Clear pushbutton status

    // Check if selected threshold is not same as previous
    EEPROM_readAnything(1,eeprom_R);
    if (eeprom_R.PEP_period != R.PEP_period)
    {
      EEPROM_writeAnything(1,R);
      virt_lcd_print("Value Stored");
    }
    else virt_lcd_print("Nothing Changed");

    Menu_disp_timer = 30;                       // Show on LCD for 3 seconds
    mode.conf = FALSE;                          // We're done with Menu, EXIT
    menu_level = 0;                             // We are done with this menu level
    flag.menu_lcd_upd = FALSE;                  // Make ready for next time
  }
}

#if AD8307_INSTALLED
//--------------------------------------------------------------------
// Screensaver Threshold Sensitivity selection Menu
//--------------------------------------------------------------------
void screenthresh_menu(void)
{
  static int8_t	current_selection;
	
  // Get Current value
  // 0=0, 1uW=0.001=>1, 10uW=0.01=>2... 100uW=>3, 1mW=>4, 10mW=>5
  if (R.idle_disp_thresh == 0) current_selection = 0;
  else current_selection = log10(R.idle_disp_thresh) + 4;

  // Selection modified by encoder.  We remember last selection, even if exit and re-entry
  if (Enc.read()/ENC_RESDIVIDE != 0)
  {
    if (Enc.read()/ENC_RESDIVIDE > 0)
    {
      current_selection++;
    }
    else if (Enc.read()/ENC_RESDIVIDE < 0)
    {
      current_selection--;
    }
    // Reset data from Encoder
    Enc.write(0);

    // Indicate that an LCD update is needed
    flag.menu_lcd_upd = FALSE;
  }

  // If LCD update is needed
  if (flag.menu_lcd_upd == FALSE)
  {
    flag.menu_lcd_upd = TRUE;                   // We have serviced LCD

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
	
    virt_lcd_clear();
    virt_lcd_setCursor(0,0);
    virt_lcd_print("ScreensaverThreshld:");
    virt_lcd_setCursor(0,1);
    virt_lcd_print("Select");

    // Print the Rotary Encoder scroll Menu
    lcd_scroll_Menu((char**)screenthresh_menu_items, menu_size, current_selection, 1, 6,1);

    virt_lcd_setCursor(0,2);
    virt_lcd_print("Available thresholds");
    virt_lcd_setCursor(0,3);
    virt_lcd_print("1uW-10mW,  10x steps");
  }

  // Enact selection
  if (flag.short_push == TRUE)
  {
    virt_lcd_clear();
    virt_lcd_setCursor(0,1);

    flag.short_push = FALSE;                    // Clear pushbutton status

    // Check if selected threshold is not same as previous
    EEPROM_readAnything(1,eeprom_R);
    if (eeprom_R.idle_disp_thresh != R.idle_disp_thresh)// New Value
    {
      EEPROM_writeAnything(1,R);
      virt_lcd_print("Value Stored");
    }
    else virt_lcd_print("Nothing Changed");

    Menu_disp_timer = 30;                       // Show on LCD for 3 seconds
    mode.conf = FALSE;                          // We're done, EXIT
    menu_level = 0;                             // We are done with this menu level
    flag.menu_lcd_upd = FALSE;                  // Make ready for next time
  }
}
#endif

//--------------------------------------------------------------------
// Scale Range Setup Submenu functions
//--------------------------------------------------------------------
void scalerange_menu_level2(void)
{
  static int16_t	current_selection;      // Keep track of current LCD menu selection

  uint8_t scale_set;                            // Determine whether CAL_SET0, CAL_SET1 or CAL_SET2

  if (menu_level == SCALE_SET0_MENU) scale_set = 0;      // SCALE_SET0_MENU
  else if (menu_level == SCALE_SET1_MENU) scale_set = 1; // SCALE_SET1_MENU
  else scale_set = 2;                                    // SCALE_SET2_MENU

  // Get Current value
  current_selection = R.ScaleRange[scale_set];

  // Selection modified by encoder.  We remember last selection, even if exit and re-entry
  if (Enc.read()/ENC_RESDIVIDE != 0)
  {
    if (Enc.read()/ENC_RESDIVIDE > 0)
    {
      current_selection++;
    }
    else if (Enc.read()/ENC_RESDIVIDE < 0)
    {
      current_selection--;
    }
    // Reset data from Encoder
    Enc.write(0);
    // Indicate that an LCD update is needed
    flag.menu_lcd_upd = FALSE;
  }

  // If LCD update is needed
  if (flag.menu_lcd_upd == FALSE)
  {
    flag.menu_lcd_upd = TRUE;                            // We are about to have serviced LCD

    // Keep Encoder Selection Within Scale Range Bounds
    int16_t max_range = R.ScaleRange[0] * 10;            // Highest permissible Scale Range for ranges 2 and 3,
    if (max_range > 99) max_range = 99;                  // never larger than 10x range 1 and never larger than 99
    int16_t min_range = R.ScaleRange[0];                 // Lowest permissible Scale Range for ranges 2 and 3,
                                                         // never smaller than range 1
    if (scale_set==0)
    {
      // Set bounds for Range 1 adjustments
      if(current_selection > 99) current_selection = 99; // Range 1 can take any value between 1 and 99
      if(current_selection < 1) current_selection = 1;			
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

    virt_lcd_clear();

    // Populate the Display - including current values selected for scale ranges
    virt_lcd_setCursor(0,0);
    virt_lcd_print("Adjust, Push to Set:");

    virt_lcd_setCursor(6,1);
    sprintf(lcd_buf,"1st Range = %2u",R.ScaleRange[0]);
    virt_lcd_print(lcd_buf);
    virt_lcd_setCursor(6,2);
    sprintf(lcd_buf,"2nd Range = %2u",R.ScaleRange[1]);
    virt_lcd_print(lcd_buf);
    virt_lcd_setCursor(6,3);
    sprintf(lcd_buf,"3rd Range = %2u",R.ScaleRange[2]);
    virt_lcd_print(lcd_buf);

    // Place "===>" in front of the "ScaleRange" currently being adjusted
    virt_lcd_setCursor(0,scale_set+1);
    virt_lcd_print("===>");
  }

  // Enact selection by saving in EEPROM
  if (flag.short_push == TRUE)
  {
    flag.short_push = FALSE;                             // Clear pushbutton status
    virt_lcd_clear();
    virt_lcd_setCursor(1,1);

    // Save modified value
    // There are so many adjustable values that it is simplest just to assume
    // a value has always been modified.  Save all 3
    EEPROM_writeAnything(1,R);
    virt_lcd_print("Value Stored");
    Menu_disp_timer = 30;                                // Show on LCD for 3 seconds
    mode.conf = TRUE;                                    // We're NOT done, just backing off
    menu_level = SCALERANGE_MENU;                        // We are done with this menu level
    flag.menu_lcd_upd = FALSE;                           // Make ready for next time
  }
}



//--------------------------------------------------------------------
// Scale Range Menu functions
//--------------------------------------------------------------------
void scalerange_menu(void)
{
  static int8_t	current_selection;                       // Keep track of current LCD menu selection

  // Selection modified by encoder.  We remember last selection, even if exit and re-entry
  if (Enc.read()/ENC_RESDIVIDE != 0)
  {
    if (Enc.read()/ENC_RESDIVIDE > 0)
    {
      current_selection++;
    }
    else if (Enc.read()/ENC_RESDIVIDE < 0)
    {
      current_selection--;
    }
    // Reset data from Encoder
    Enc.write(0);
    // Indicate that an LCD update is needed
    flag.menu_lcd_upd = FALSE;
  }

  // If LCD update is needed
  if (flag.menu_lcd_upd == FALSE)
  {
    flag.menu_lcd_upd = TRUE;                            // We have serviced LCD

    // Keep Encoder Selection Within Bounds of the Menu Size
    uint8_t menu_size = 4;
    while(current_selection >= menu_size)
    current_selection -= menu_size;
    while(current_selection < 0)
    current_selection += menu_size;

    virt_lcd_clear();

    // Populate the Display - including current values selected for scale ranges
    virt_lcd_setCursor(0,0);
    if (current_selection<3)
      virt_lcd_print("Select Scale Range:");
    else
      virt_lcd_print("Turn or Push to Exit");

    virt_lcd_setCursor(6,1);
    sprintf(lcd_buf,"1st Range = %2u",R.ScaleRange[0]);
    virt_lcd_print(lcd_buf);
    virt_lcd_setCursor(6,2);
    sprintf(lcd_buf,"2nd Range = %2u",R.ScaleRange[1]);
    virt_lcd_print(lcd_buf);
    virt_lcd_setCursor(6,3);
    sprintf(lcd_buf,"3rd Range = %2u",R.ScaleRange[2]);
    virt_lcd_print(lcd_buf);

    // Place "->" in front of the relevant "ScaleRange" to be selected with a push
    if (current_selection<3)
    {
      virt_lcd_setCursor(4,current_selection+1);
      virt_lcd_print("->");
    }
  }

  // Enact selection
  if (flag.short_push == TRUE)
  {
    flag.short_push = FALSE;                             // Clear pushbutton status

    switch (current_selection)
    {
      case 0:
        menu_level = SCALE_SET0_MENU;
        flag.menu_lcd_upd = FALSE;                       // force LCD reprint
        break;
      case 1:
        menu_level = SCALE_SET1_MENU;
        flag.menu_lcd_upd = FALSE;                       // force LCD reprint
        break;
      case 2:
        menu_level = SCALE_SET2_MENU;
        flag.menu_lcd_upd = FALSE;                       // force LCD reprint
        break;
      default:
        virt_lcd_clear();
        virt_lcd_setCursor(0,1);
        virt_lcd_print("Done w. Scale Ranges");
        Menu_disp_timer = 30;                            // Show on LCD for 3 seconds
        mode.conf = FALSE;                               // We're done
        menu_level = 0;                                  // We are done with this menu level
        flag.menu_lcd_upd = FALSE;                       // Make ready for next time
    }
  }
}

#if AD8307_INSTALLED
//--------------------------------------------------------------------
// Calibrate Submenu functions
//--------------------------------------------------------------------
void calibrate_menu_level2(void)
{
  static int16_t  current_selection;                     // Keep track of current LCD menu selection

  uint8_t cal_set;                                       // Determine whether CAL_SET0, CAL_SET1 or CAL_SET2

  if (menu_level == CAL_SET2_MENU) cal_set = 1;	         // CAL_SET2_MENU
  else cal_set = 0;                                      // CAL_SET0_MENU or CAL_SET1_MENU

  // These defines to aid readability of code
  #define CAL_BAD	0                                // Input signal of insufficient quality for calibration
  #define CAL_FWD	1                                // Good input signal detected, forward direction
  #define CAL_REV	2                                // Good input signal detected, reverse direction (redundant)
  // Below variable can take one of the three above defined values, based on the
  // detected input "calibration" signal
  static uint8_t cal_sig_direction_quality;
  
  // Get Current value
  current_selection = R.cal_AD[cal_set].db10m;

  // Selection modified by encoder.  We remember last selection, even if exit and re-entry
  if (Enc.read()/ENC_RESDIVIDE != 0)
  {
    if (Enc.read()/ENC_RESDIVIDE > 0)
    {
      current_selection++;
    }
    else if (Enc.read()/ENC_RESDIVIDE < 0)
    {
      current_selection--;
    }
    // Reset data from Encoder
    Enc.write(0);
    // Indicate that an LCD update is needed
    flag.menu_lcd_upd = FALSE;
  }

  // Determine direction and level of calibration signal input
  // Check forward direction and sufficient level
  if (((fwd - ref) > CAL_INP_QUALITY) &&
      (cal_sig_direction_quality != CAL_FWD))
  {
    cal_sig_direction_quality = CAL_FWD;
    flag.menu_lcd_upd = FALSE;                           // Indicate that an LCD update is needed
  }
  // Check reverse direction and sufficient level
  else if (((ref - fwd) > CAL_INP_QUALITY) &&
           (cal_sig_direction_quality != CAL_REV))
  {
    cal_sig_direction_quality = CAL_REV;
    flag.menu_lcd_upd = FALSE;                           // Indicate that an LCD update is needed
  }
  // Check insufficient level
  else if ((ABS((fwd - ref)) <= CAL_INP_QUALITY) &&
           (cal_sig_direction_quality != CAL_BAD))
  {
    cal_sig_direction_quality = CAL_BAD;
    flag.menu_lcd_upd = FALSE;                           // Indicate that an LCD update is needed
  }

  // If LCD update is needed
  if(flag.menu_lcd_upd == FALSE)  
  {
    flag.menu_lcd_upd = TRUE;                            // We have serviced LCD

    // Keep Encoder Selection Within Bounds of the Menu Size
    int16_t max_value = 530;                             // Highest permissible Calibration value in dBm * 10
    int16_t min_value = -100;                            // Lowest permissible Calibration value in dBm * 10
    if(current_selection > max_value) current_selection = max_value;
    if(current_selection < min_value) current_selection = min_value;

    // Store Current value in running storage
    R.cal_AD[cal_set].db10m = current_selection;

    virt_lcd_clear();
    virt_lcd_setCursor(0,0);	

    if (menu_level == CAL_SET0_MENU)                     // equals cal_set == 0
    {
      virt_lcd_print("Single Level Cal:");
    }
    else if (menu_level == CAL_SET1_MENU)                // equals cal_set == 1
    {
      virt_lcd_print("First Cal SetPoint:");
    }
    else if (menu_level == CAL_SET2_MENU)
    {
      virt_lcd_print("Second Cal SetPoint:");
    }

    virt_lcd_setCursor(0,1);
    virt_lcd_print("Adjust (dBm)->");
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
    virt_lcd_print(lcd_buf);

    if (cal_sig_direction_quality == CAL_FWD)
    {
      virt_lcd_setCursor(0,2);
      virt_lcd_print(">Push to set<");
      virt_lcd_setCursor(0,3);
      virt_lcd_print("Signal detected");
    }
    else if (cal_sig_direction_quality == CAL_REV)
    {
      virt_lcd_setCursor(0,2);
      virt_lcd_print(">Push to set<");
      virt_lcd_setCursor(0,3);
      virt_lcd_print("Reverse detected");
    }
    else                                                 // cal_sig_direction_quality == CAL_BAD
    {
      virt_lcd_setCursor(0,2);
      virt_lcd_print(">Push to exit<");
      virt_lcd_setCursor(0,3);
      virt_lcd_print("Poor signal quality");
    }
  }

  // Enact selection by saving in EEPROM
  if (flag.short_push == TRUE)
  {
    flag.short_push = FALSE;                             // Clear pushbutton status
    virt_lcd_clear();
    virt_lcd_setCursor(1,1);

    // Save modified value
    // If forward direction, then we calibrate for both, using the measured value for
    // in the forward direction only
    if (cal_sig_direction_quality == CAL_FWD)
    {
      if (CAL_SET0_MENU)
      {
        uint16_t thirty_dB;                              // Used for single shot calibration
        if (ad7991_addr) thirty_dB = 1165;               // If AD7991 was detected during init
        else thirty_dB = 1200;
        
        R.cal_AD[0].Fwd = fwd;
        R.cal_AD[0].Rev = fwd;
        // Set second calibration point at 30 dB less, assuming 25mV per dB
        R.cal_AD[1].db10m = R.cal_AD[0].db10m - 300;
        R.cal_AD[1].Fwd = R.cal_AD[0].Fwd - thirty_dB;
        R.cal_AD[1].Rev = R.cal_AD[0].Fwd - thirty_dB;
      }
      else
      {
        R.cal_AD[cal_set].Fwd = fwd;
        R.cal_AD[cal_set].Rev = fwd;
      }
      EEPROM_writeAnything(1,R);
      virt_lcd_print("Value Stored");
    }
    // If reverse, then we calibrate for reverse direction only
    else if (cal_sig_direction_quality == CAL_REV)
    {
      if (CAL_SET0_MENU)
      {
        R.cal_AD[0].Rev = ref;
        // Set second calibration point at 30 dB less, assuming 25mV per dB
        R.cal_AD[1].Rev = R.cal_AD[0].Fwd - 1200;
      }
      else
      {
        R.cal_AD[cal_set].Rev = ref;
      }
      EEPROM_writeAnything(1,R);
      virt_lcd_print("Value Stored");
    }
    else                                                 // cal_sig_direction_quality == CAL_BAD
    {
      virt_lcd_print("Nothing changed");
    }

    Menu_disp_timer = 30;                                // Show on LCD for 3 seconds
    mode.conf = TRUE;                                    // We're NOT done, just backing off
    menu_level = CAL_MENU;                               // We are done with this menu level
    flag.menu_lcd_upd = FALSE;                           // Make ready for next time
  }
}



//--------------------------------------------------------------------
// Calibrate Menu functions
//--------------------------------------------------------------------
void calibrate_menu(void)
{
  static int8_t	current_selection;                       // Keep track of current LCD menu selection

  // Selection modified by encoder.  We remember last selection, even if exit and re-entry
  if (Enc.read()/ENC_RESDIVIDE != 0)
  {
    if (Enc.read()/ENC_RESDIVIDE > 0)
    {
      current_selection++;
    }
    else if (Enc.read()/ENC_RESDIVIDE < 0)
    {
      current_selection--;
    }
    // Reset data from Encoder
    Enc.write(0);
    // Indicate that an LCD update is needed
    flag.menu_lcd_upd = FALSE;
  }

  // If LCD update is needed
  if (flag.menu_lcd_upd == FALSE)
  {
    flag.menu_lcd_upd = TRUE;                            // We have serviced LCD

    // Keep Encoder Selection Within Bounds of the Menu Size
    uint8_t menu_size = calibrate_menu_size;
    while(current_selection >= menu_size)
      current_selection -= menu_size;
    while(current_selection < 0)
      current_selection += menu_size;

    virt_lcd_clear();

    // Print the Rotary Encoder scroll Menu
    lcd_scroll_Menu((char**)calibrate_menu_items, menu_size, current_selection,1, 0,1);

    switch (current_selection)
    {
      case 0:
        virt_lcd_setCursor(0,2);
        virt_lcd_print("Calibrate using one");
        virt_lcd_setCursor(6,3);
        virt_lcd_print("accurate level");
        break;
      case 1:
        virt_lcd_setCursor(0,2);
        virt_lcd_print("Set higher of two");
        virt_lcd_setCursor(5,3);
        virt_lcd_print("accurate levels");
        break;
      case 2:
        virt_lcd_setCursor(0,2);
        virt_lcd_print("Set lower of two");
        virt_lcd_setCursor(5,3);
        virt_lcd_print("accurate levels");
        break;
    }

    // Indicate Current value stored under the currently selected GainPreset
    // The "stored" value indication changes according to which GainPreset is currently selected.
    virt_lcd_setCursor(0,0);				
    virt_lcd_print("Calibrate");
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
      virt_lcd_setCursor(16,0);
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
      virt_lcd_print(lcd_buf);
    }
    else
    {
      virt_lcd_setCursor(16,0);
      virt_lcd_print(" --");
    }
  }

  // Enact selection
  if (flag.short_push == TRUE)
  {
    flag.short_push = FALSE;                             // Clear pushbutton status

    switch (current_selection)
    {
      case 0:
        menu_level = CAL_SET0_MENU;
        flag.menu_lcd_upd = FALSE;                       // force LCD reprint
        break;
      case 1:
        menu_level = CAL_SET1_MENU;
        flag.menu_lcd_upd = FALSE;                       // force LCD reprint
        break;
      case 2:
        menu_level = CAL_SET2_MENU;
        flag.menu_lcd_upd = FALSE;                       // force LCD reprint
        break;
      case 3:
        virt_lcd_clear();
        virt_lcd_setCursor(1,1);				
        virt_lcd_print("Done w. Cal");
        Menu_disp_timer = 30;                            // Show on LCD for 3 seconds
        mode.conf = TRUE;                                // We're NOT done, just backing off
        menu_level = 0;                                  // We are done with this menu level
        flag.menu_lcd_upd = FALSE;                       // Make ready for next time
        break;
      default:
        virt_lcd_clear();
        virt_lcd_setCursor(1,1);				
        virt_lcd_print("Done w. Cal");
        Menu_disp_timer = 30;                            // Show on LCD for 3 seconds
        mode.conf = FALSE;                               // We're done
        menu_level = 0;                                  // We are done with this menu level
        flag.menu_lcd_upd = FALSE;                       // Make ready for next time
        break;
    }
  }
}
#else
//--------------------------------------------------------------------
// Calibrate Menu functions
//--------------------------------------------------------------------
void calibrate_menu(void)
{
  static uint8_t   current_selection;

  // We want LCD update every time - to show Power measurement
  flag.menu_lcd_upd = FALSE;
 
  // Calibration multiplier for diode detector type Power/SWR meter, 100 = 1.0
  current_selection = R.meter_cal;

  // Selection modified by encoder.  We remember last selection, even if exit and re-entry
  if (Enc.read()/ENC_RESDIVIDE != 0)
  {
    if ((Enc.read()/ENC_RESDIVIDE > 0) && (current_selection < 250) )
    {
      current_selection++;
    }
    if ((Enc.read()/ENC_RESDIVIDE < 0) && (current_selection > 10) )
    {
      current_selection--;
    }
    // Reset data from Encoder
    Enc.write(0);

    R.meter_cal = current_selection;   
  }

  virt_lcd_clear();

  virt_lcd_setCursor(0,0);
  virt_lcd_print("Meter Calibrate:");
  virt_lcd_setCursor(0,1);
  virt_lcd_print("Adjust->   ");

  sprintf(lcd_buf,"%1u.%02u",current_selection/100, current_selection%100);
  virt_lcd_print(lcd_buf);
 					
  virt_lcd_setCursor(0,2);
  virt_lcd_print("Range is 0.10 - 2.50");
  
  //measure_power_and_swr();
  virt_lcd_setCursor(0,3);
  virt_lcd_print("MeasuredPower:");
  print_p_mw(power_mw);
  virt_lcd_print(lcd_buf);
  	
  // Enact selection by saving in EEPROM
  if (flag.short_push == TRUE)
  {
    flag.short_push = FALSE;             // Clear pushbutton status

    virt_lcd_clear();
    virt_lcd_setCursor(1,1);
    EEPROM_readAnything(1,R);
    if (R.meter_cal != current_selection)// New Value
    {
      R.meter_cal = current_selection;
      EEPROM_writeAnything(1,R);
      virt_lcd_print("Value Stored");
    }
    else virt_lcd_print("Nothing Changed");
    
    Menu_disp_timer = 30;                // Show on LCD for 3 seconds
    mode.conf = FALSE;                   // We're done
    menu_level = 0;                      // We are done with this menu level
  }
}
#endif


//--------------------------------------------------------------------
// Factory Reset with all default values
//--------------------------------------------------------------------
void factory_menu(void)
{
  static int8_t	current_selection;

  // Selection modified by encoder.  We remember last selection, even if exit and re-entry
  if (Enc.read()/ENC_RESDIVIDE != 0)
  {
    if (Enc.read()/ENC_RESDIVIDE > 0)
    {
      current_selection++;
    }
    else if (Enc.read()/ENC_RESDIVIDE < 0)
    {
      current_selection--;
    }
    // Reset data from Encoder
    Enc.write(0);

    // Indicate that an LCD update is needed
    flag.menu_lcd_upd = FALSE;
  }

  // If LCD update is needed
  if (flag.menu_lcd_upd == FALSE)
  {
    flag.menu_lcd_upd = TRUE;                            // We have serviced LCD

    // Keep Encoder Selection Within Bounds of the Menu Size
    uint8_t menu_size = factory_menu_size;
    while(current_selection >= menu_size)
      current_selection -= menu_size;
    while(current_selection < 0)
      current_selection += menu_size;

    virt_lcd_clear();
    virt_lcd_setCursor(0,0);
    virt_lcd_print("All to Default?");

    // Print the Rotary Encoder scroll Menu
    lcd_scroll_Menu((char**)factory_menu_items, menu_size, current_selection, 1, 0,1);

    virt_lcd_setCursor(0,3);
    virt_lcd_print("Factory Reset Menu");
  }

  // Enact selection
  if (flag.short_push == TRUE)
  {
    flag.short_push = FALSE;                             // Clear pushbutton status

    switch (current_selection)
    {
      case 0:
        virt_lcd_clear();
        virt_lcd_setCursor(1,1);
        virt_lcd_print("Nothing Changed");
        Menu_disp_timer = 30;                            // Show on LCD for 3 seconds
        mode.conf = TRUE;                                // We're NOT done, just backing off
        menu_level = 0;                                  // We are done with this menu level
        flag.menu_lcd_upd = FALSE;                       // Make ready for next time
        break;
      case 1:                                            // Factory Reset
        // Force an EEPROM update upon reboot by storing 0xfe in the first address
        EEPROM.write(0,0xfe);
        virt_lcd_clear();
        virt_lcd_setCursor(0,0);				
        virt_lcd_print("Factory Reset");
        virt_lcd_setCursor(0,1);
        virt_lcd_print("All default");
        Menu_disp_timer = 30;                            // Show on LCD for 3 seconds
        SOFT_RESET();
      default:
        virt_lcd_clear();
        virt_lcd_setCursor(1,1);				
        virt_lcd_print("Nothing Changed");
        Menu_disp_timer = 30;                            // Show on LCD for 3 seconds
        mode.conf = FALSE;                               // We're done
        menu_level = 0;                                  // We are done with this menu level
        flag.menu_lcd_upd = FALSE;                       // Make ready for next time
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

  // Selection modified by encoder.  We remember last selection, even if exit and re-entry
  if (Enc.read()/ENC_RESDIVIDE != 0)
  {
    if (Enc.read()/ENC_RESDIVIDE > 0)
    {
      current_selection++;
    }
    else if (Enc.read()/ENC_RESDIVIDE < 0)
    {
      current_selection--;
    }
    // Reset data from Encoder
    Enc.write(0);
    // Indicate that an LCD update is needed
    flag.menu_lcd_upd = FALSE;
  }

  if (flag.menu_lcd_upd == FALSE)				// Need to update LCD
  {
    flag.menu_lcd_upd = TRUE;					// We have serviced LCD

    // Keep Encoder Selection Within Bounds of the Menu Size
    uint8_t menu_size = level0_menu_size;
    while(current_selection >= menu_size)
      current_selection -= menu_size;
    while(current_selection < 0)
      current_selection += menu_size;

    virt_lcd_clear();
    virt_lcd_setCursor(0,0);
    virt_lcd_print("Config Menu:");

    // Print the Menu
    lcd_scroll_Menu((char**)level0_menu_items, menu_size, current_selection,1, 0,3);
  }

  if (flag.short_push == TRUE)
  {
    flag.short_push = FALSE;                                    // Clear pushbutton status
    flag.menu_lcd_upd = FALSE;                                  //  force LCD reprint
    switch (current_selection)
    {
      case 0: // SWR Alarm Threshold Set
        menu_level = SWR_ALARM_THRESHOLD;
        break;

      case 1: // SWR Alarm Power Threshold Set
        menu_level = SWR_ALARM_PWR_THRESHOLD;
        break;

      case 2: // PEP sampling period select
        menu_level = PEP_MENU;
        break;

      #if AD8307_INSTALLED
      case 3: // Screensaver Threshold
        menu_level = SCREENTHRESH_MENU;
        break;
      
      case 4: // Scale Range Set
        menu_level = SCALERANGE_MENU;
        break;

      case 5: // Calibrate
        menu_level = CAL_MENU;
        break;

      case 6: // Display Debug stuff
        menu_level = DEBUG_MENU;
        break;

      case 7: // Factory Reset
        menu_level = FACTORY_MENU;
        break;
      
      #else
      case 3: // Scale Range Set
        menu_level = SCALERANGE_MENU;
        break;

      case 4: // Calibrate
        menu_level = CAL_MENU;
        break;

      case 5: // Display Debug stuff
        menu_level = DEBUG_MENU;
        break;

      case 6: // Factory Reset
        menu_level = FACTORY_MENU;
        break;      
      #endif
      
      default:
        // Exit
        virt_lcd_clear();
        virt_lcd_setCursor(1,1);
        virt_lcd_print("Return from Menu");
        mode.conf = FALSE;                                      // We're done
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

  #if AD8307_INSTALLED
  else if (menu_level == SCREENTHRESH_MENU) screenthresh_menu();
  #endif

  else if (menu_level == SCALERANGE_MENU) scalerange_menu();
  else if (menu_level == SCALE_SET0_MENU) scalerange_menu_level2();
  else if (menu_level == SCALE_SET1_MENU) scalerange_menu_level2();
  else if (menu_level == SCALE_SET2_MENU) scalerange_menu_level2();

  else if (menu_level == CAL_MENU) calibrate_menu();
  #if AD8307_INSTALLED
  else if (menu_level == CAL_SET0_MENU) calibrate_menu_level2();
  else if (menu_level == CAL_SET1_MENU) calibrate_menu_level2();
  else if (menu_level == CAL_SET2_MENU) calibrate_menu_level2();
  #endif

  else if (menu_level == DEBUG_MENU) debug_menu();

  else if (menu_level == FACTORY_MENU) factory_menu();
}

