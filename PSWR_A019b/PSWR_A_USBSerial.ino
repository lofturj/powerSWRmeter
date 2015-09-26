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

//#include <stdarg.h>
#include <stdio.h>
//
//-----------------------------------------------------------------------------------------
//                      Send AD8307 measurement data to the Computer
//
//                      Format of transmitted data is:
//                      Incident (real transmitted) power, VSWR
//-----------------------------------------------------------------------------------------
//
//------------------------------------------
// Instantaneous Power (unformatted) and SWR
void usb_poll_data(void)
{
  //------------------------------------------
  // Power indication, incident power
  if (Reverse)   Serial.print("-");
  Serial.print(power_mw/1000,8);
  Serial.print(F(", "));
  Serial.println(swr,2);
}

//------------------------------------------
// Instantaneous Power (formatted, mW - kW)
void usb_poll_inst(void)
{
  print_p_mw(power_mw);
  Serial.print(lcd_buf);
  //------------------------------------------
  // SWR indication
  Serial.print(F(", VSWR"));
  print_swr();
  Serial.println(lcd_buf);
}

//------------------------------------------
// Peak (100ms) Power (formatted, mW - kW)
void usb_poll_pk(void)
{
  print_p_mw(power_mw_pk);
  Serial.print(lcd_buf);
  //------------------------------------------
  // SWR indication
  Serial.print(F(", VSWR"));
  print_swr();
  Serial.println(lcd_buf);
}

//------------------------------------------
// PEP (1s) Power (formatted, mW - kW)
void usb_poll_pep(void)
{
  print_p_mw(power_mw_pep);
  Serial.print(lcd_buf);
  //------------------------------------------
  // SWR indication
  Serial.print(F(", VSWR"));
  print_swr();
  Serial.println(lcd_buf);
}

//------------------------------------------
// AVG (1s) Power (formatted, mW - kW)
void usb_poll_avg(void)
{
  //------------------------------------------
  // Power indication, 1s average power, formatted, pW-kW
  print_p_mw(power_mw_avg);
  Serial.print(lcd_buf);
  //------------------------------------------
  // SWR indication
  Serial.print(F(", VSWR"));
  print_swr();
  Serial.println(lcd_buf);
}

void usb_poll_instdb(void)
{
  //------------------------------------------
  // Power indication, instantaneous power, formatted, dB
  print_dbm((int16_t) (power_db*10.0));
  Serial.print(lcd_buf);
  //------------------------------------------
  // SWR indication
  Serial.print(F(", VSWR"));
  print_swr();
  Serial.println(lcd_buf);
}

void usb_poll_pkdb(void)
{
  //------------------------------------------
  // Power indication, 100ms peak power, formatted, dB
  print_dbm((int16_t) (power_db_pk*10.0));
  Serial.print(lcd_buf);
  //------------------------------------------
  // SWR indication
  Serial.print(F(", VSWR"));
  print_swr();
  Serial.println(lcd_buf);
}

void usb_poll_pepdb(void)
{
  //------------------------------------------
  // Power indication, PEP power, formatted, dB
  print_dbm((int16_t) (power_db_pep*10.0));
  Serial.print(lcd_buf);
  //------------------------------------------
  // SWR indication
  Serial.print(F(", VSWR"));
  print_swr();
  Serial.println(lcd_buf);
}

void usb_poll_avgdb(void)
{
  //------------------------------------------
  // Power indication, 1s average power, formatted, dB
  print_dbm((int16_t) (power_db_avg*10.0));
  Serial.print(lcd_buf);
  //------------------------------------------
  // SWR indication
  Serial.print(F(", VSWR"));
  print_swr();
  Serial.println(lcd_buf);
}

//
//-----------------------------------------------------------------------------------------
//                      Send AD8307 measurement data to the Computer
//
//                      Long and Human Readable Format
//-----------------------------------------------------------------------------------------
//
void usb_poll_long(void)
{
  //------------------------------------------
  // Power indication, inst, peak (100ms), pep (1s), average (1s)
  Serial.println(F("Power (inst, peak 100ms, pep 1s, avg 1s):"));
  if (Reverse) Serial.print(F("-"));
  print_p_mw(power_mw);
  Serial.print(lcd_buf);
  Serial.print(F(", "));
  if (Reverse) Serial.print(F("-"));
  print_p_mw(power_mw_pk);
  Serial.print(lcd_buf);
  Serial.print(F(", "));
  if (Reverse) Serial.print(F("-"));
  print_p_mw(power_mw_pep);
  Serial.print(lcd_buf);
  Serial.print(F(", "));
  if (Reverse) Serial.print(F("-"));
  print_p_mw(power_mw_avg);
  Serial.println(lcd_buf);
	
  //------------------------------------------
  // Forward and Reflected Power indication, instantaneous only
  sprintf(lcd_buf, "Forward and Reflected Power (inst):\r\n");
  Serial.print(lcd_buf);
  if (Reverse) Serial.print(F("-"));
  print_p_mw(fwd_power_mw);
  Serial.print(lcd_buf);
  Serial.print(F(", "));
  if (Reverse) Serial.print(F("-"));
  print_p_mw(ref_power_mw);
  Serial.println(lcd_buf);

  //------------------------------------------
  // SWR indication
  Serial.print(F("VSWR"));
  print_swr();
  Serial.println(lcd_buf);
  Serial.println();
}

//------------------------------------------
// Prints the selected PSWR report type on a continuous basis, once every 100 milliseconds
void usb_cont_report(void)
{
  if (R.usb_report_cont == TRUE)
  {
    if (R.usb_report_type == REPORT_DATA) usb_poll_data();

    else if (R.usb_report_type == REPORT_INST) usb_poll_inst();
    else if (R.usb_report_type == REPORT_PK) usb_poll_pk();
    else if (R.usb_report_type == REPORT_PEP) usb_poll_pep();

    else if (R.usb_report_type == REPORT_INSTDB) usb_poll_instdb();
    else if (R.usb_report_type == REPORT_PKDB) usb_poll_pkdb();
    else if (R.usb_report_type == REPORT_PEPDB) usb_poll_pepdb();

    else if (R.usb_report_type == REPORT_LONG) usb_poll_long();
  }
}

//
//-----------------------------------------------------------------------------------------
//      Parse and act upon an incoming USB command
//
//      Commands are:
//
//        $ppoll              Poll for one single USB serial report, inst power (unformatted)
//        $pinst              Poll for one single USB serial report, inst power (human readable)
//        $ppk                Poll for one single USB serial report, 100ms peak power (human readable)
//        $ppep               Poll for one single USB serial report, pep power (human readable)
//        $pavg               Poll for one single USB serial report, avg power (human readable)
//        $pinstdb            Poll for one single USB serial report, inst power in dB (human readable)
//        $ppkdb              Poll for one single USB serial report, 100ms peak power in dB (human readable)
//        $ppepdb             Poll for one single USB serial report, pep power in dB (human readable)
//        $pavgdb             Poll for one single USB serial report, avg power in dB (human readable)
//        $plong              Poll for one single USB serial report, actual power (inst, pep and avg)
//                            as well as fwd power, reflected power and SWR (long form)
//
//        $pcont              USB serial reporting in a continuous mode, 10 times per second
//
//                            $ppoll, $pinst, $ppk, $ppep, $pavg$ or plong entered after $pcont will
//                            switch back to single shot mode
//
//        $sleepmsg=abcdefg   Where abcdefg is a free text string to be displayed when
//                            in screensaver mode, up to 20 characters max
//        $sleeppwrset x      Power below the level defined here will put display into screensaver mode.
//                            x = 0.001, 0.01, 0.1, 1 or 10 mW (milliWatts)
//        $sleeppwrget        Return current value
//
//        $alarmset x         x = 1.5 to 3.9. 4 will inactivate SWR Alarm function
//        $alarmget           Return current value
//        $alarmpowerset x    x = 1, 10, 100, 1000 or 10000 mW (milliwatts)
//        $alarmpowerget      Return current value
//
//        $pepperiodget x     x = 1, 2.5 or 5 seconds.  PEP sampling period
//        $pepperiodset       Return current value
//
//        $calset cal1 AD1-1 AD2-1 cal2 AD1-2 AD2-2
//                            Write new calibration values to the meter
//        $calget             Retrieve calibration values
//
//                            Format of calibration values is:
//                            cal1 AD1-1 AD2-1 cal2 AD1-2 AD2-2
//                                          where:
//                            cal1 and cal2 are calibration setpoints 1 and 2 in 10x dBm
//                                          and
//                            ADx-1 and ADx-2 are the corresponding AD values for
//                            AD1 (forward direction) and AD2 (reverse direction).
//                            (
//                              normally the AD1 and AD2 values for each setpoint would be the same, 
//                              however by using the $swrcalset command it is possible to balance any
//                              small differences there might be between the two AD8307 outputs.
//                              Note that I have not found this to be necessary at all :)
//                            )
//
//        $scaleget           Retrieve user definable scale ranges
//        $scaleset           Write new  scale ranges
//
//                            The scale ranges are user definable, up to 3 ranges per decade
//                            e.g. 6, 12 and 24 gives:
//                            ... 6mW, 12mW, 24mW, 60mW ... 1.2W, 2.4W, 6W 12W 24W 60W 120W ...
//                            If all three values set as "2", then
//                            ... 2W, 20W, 200W ...
//                            The third and largest value has to be less than ten times the first value
//
//        $version            Report version and date of firmware
//
//-----------------------------------------------------------------------------------------
//
char incoming_command_string[50];                       // Input from USB Serial
void usb_parse_incoming(void)
{
  char *pEnd;
  uint16_t inp_val;
  double inp_double;

  if (!strcmp("ppoll",incoming_command_string))         // Poll, if Continuous mode, then switch into Polled Mode
  {
    // Disable continuous USB report mode ($pcont) if previously set
    // and Write report mode to EEPROM, if changed
    if ((R.usb_report_type != REPORT_DATA)||(R.usb_report_cont == TRUE))
    {
      EEPROM_readAnything(1,R);
      R.usb_report_type = REPORT_DATA;
      R.usb_report_cont = FALSE;
      EEPROM_writeAnything(1,R);      
    }
    usb_poll_data();                                    // Send data over USB
  }
  else if (!strcmp("pinst",incoming_command_string))    // Poll for one single Human Readable report
  {
    // Disable continuous USB report mode ($pcont) if previously set
    // and Write report mode to EEPROM, if changed 
    if ((R.usb_report_type != REPORT_INST)||(R.usb_report_cont == TRUE))
    {
      EEPROM_readAnything(1,R);
      R.usb_report_type = REPORT_INST;
      R.usb_report_cont = FALSE;
      EEPROM_writeAnything(1,R);        
    }
    usb_poll_inst();
  }
  else if (!strcmp("ppk",incoming_command_string))      // Poll for one single Human Readable report
  {
    // Disable continuous USB report mode ($pcont) if previously set
    // and Write report mode to EEPROM, if changed
    if ((R.usb_report_type != REPORT_PK)||(R.usb_report_cont == TRUE))
    {
      EEPROM_readAnything(1,R);
      R.usb_report_type = REPORT_PK;
      R.usb_report_cont = FALSE;
      EEPROM_writeAnything(1,R);    
    }
    usb_poll_pk();
  }
  else if (!strcmp("ppep",incoming_command_string))      // Poll for one single Human Readable report
  {
    // Disable continuous USB report mode ($pcont) if previously set
    // and Write report mode to EEPROM, if changed
    if ((R.usb_report_type != REPORT_PEP)||(R.usb_report_cont == TRUE))
    {
      EEPROM_readAnything(1,R);
      R.usb_report_type = REPORT_PEP;
      R.usb_report_cont = FALSE;
      EEPROM_writeAnything(1,R); 
    }
    usb_poll_pep();
  }
  else if (!strcmp("pavg",incoming_command_string))      // Poll for one single Human Readable report
  {
    // Disable continuous USB report mode ($pcont) if previously set
    // and Write report mode to EEPROM, if changed
    if ((R.usb_report_type != REPORT_PEP)||(R.usb_report_cont == TRUE))
    {
      EEPROM_readAnything(1,R);
      R.usb_report_type = REPORT_AVG;
      R.usb_report_cont = FALSE;
      EEPROM_writeAnything(1,R); 
    }
    usb_poll_avg();
  }
  else if (!strcmp("pinstdb",incoming_command_string))   // Poll for one single Human Readable report
  {
    // Disable continuous USB report mode ($pcont) if previously set
    // and Write report mode to EEPROM, if changed
    if ((R.usb_report_type != REPORT_INSTDB)||(R.usb_report_cont == TRUE))
    {
      EEPROM_readAnything(1,R);
      R.usb_report_type = REPORT_INSTDB;
      R.usb_report_cont = FALSE;
      EEPROM_writeAnything(1,R);
    }
    usb_poll_instdb();
  }
  else if (!strcmp("ppkdb",incoming_command_string))     // Poll for one single Human Readable report
  {
    // Disable continuous USB report mode ($pcont) if previously set
    // and Write report mode to EEPROM, if changed
    if ((R.usb_report_type != REPORT_PKDB)||(R.usb_report_cont == TRUE))
    {
      EEPROM_readAnything(1,R);
      R.usb_report_type = REPORT_PKDB;
      R.usb_report_cont = FALSE;
      EEPROM_writeAnything(1,R);
    }
    usb_poll_pkdb();
  }
  else if (!strcmp("ppepdb",incoming_command_string))    // Poll for one single Human Readable report
  {
    // Disable continuous USB report mode ($pcont) if previously set
    // and Write report mode to EEPROM, if changed
    if ((R.usb_report_type != REPORT_PEPDB)||(R.usb_report_cont == TRUE))
    {
      EEPROM_readAnything(1,R);
      R.usb_report_type = REPORT_PEPDB;
      R.usb_report_cont = FALSE;
      EEPROM_writeAnything(1,R);
    }
    usb_poll_pepdb();
  }
  else if (!strcmp("pavgdb",incoming_command_string))    // Poll for one single Human Readable report
  {
    // Disable continuous USB report mode ($pcont) if previously set
    // and Write report mode to EEPROM, if changed
    if ((R.usb_report_type != REPORT_AVGDB)||(R.usb_report_cont == TRUE))
    {
      EEPROM_readAnything(1,R);
      R.usb_report_type = REPORT_AVGDB;
      R.usb_report_cont = FALSE;
      EEPROM_writeAnything(1,R);
    }
    usb_poll_avgdb();
  }
  else if (!strcmp("plong",incoming_command_string))    // Poll for one single Human Readable report
  {
    // Disable continuous USB report mode ($pcont) if previously set
    // and Write report mode to EEPROM, if changed
    if ((R.usb_report_type != REPORT_LONG)||(R.usb_report_cont == TRUE))
    {
      EEPROM_readAnything(1,R);
      R.usb_report_type = REPORT_LONG;
      R.usb_report_cont = FALSE;
      EEPROM_writeAnything(1,R);
    }
    usb_poll_long();
  }
  else if (!strcmp("pcont",incoming_command_string))    // Switch into Continuous Mode
  {
    // Enable continuous USB report mode ($pcont), and write to EEPROM, if previously disabled
    if (R.usb_report_cont == FALSE)
    {
      EEPROM_readAnything(1,R);
      R.usb_report_cont = TRUE;
      EEPROM_writeAnything(1,R);
    }
  }

  #if AD8307_INSTALLED
  else if (!strcmp("calget",incoming_command_string))   // Retrieve calibration values
  {
    Serial.print(F("AD8307 Cal: "));
    sprintf(lcd_buf,"%4d,%4d,%4d,%4d,%4d,%4d", 
        R.cal_AD[0].db10m,R.cal_AD[0].Fwd,R.cal_AD[0].Rev,
        R.cal_AD[1].db10m,R.cal_AD[1].Fwd,R.cal_AD[1].Rev);
    Serial.println(lcd_buf);
  }
  else if (!strncmp("calset",incoming_command_string,6)) // Write new calibration values
  {
    EEPROM_readAnything(1,R);
    R.cal_AD[0].db10m = strtol(incoming_command_string+6,&pEnd,10);
    R.cal_AD[0].Fwd = strtol(pEnd,&pEnd,10);
    R.cal_AD[0].Rev = strtol(pEnd,&pEnd,10);
    R.cal_AD[1].db10m = strtol(pEnd,&pEnd,10);
    R.cal_AD[1].Fwd = strtol(pEnd,&pEnd,10);
    R.cal_AD[1].Rev = strtol(pEnd,&pEnd,10);
    EEPROM_writeAnything(1,R);
  }
  #else
  else if (!strcmp("calget",incoming_command_string))   // Retrieve calibration values
  {
    Serial.print(F("Meter Cal: "));
    Serial.println(R.meter_cal/100.0,2);
  }
  else if (!strncmp("calset",incoming_command_string,6)) // Write new calibration values
  {
    inp_val = strtod(incoming_command_string+6,&pEnd) *100;
    if ((inp_val >= 10) && (inp_val < 251))
    {
      EEPROM_readAnything(1,R);
      R.meter_cal = inp_val;
      EEPROM_writeAnything(1,R);
    }
  }  
  #endif

  else if (!strcmp("scaleget",incoming_command_string))  // Retrieve scale limits
  {
    Serial.print(F("Scale: "));
    sprintf(lcd_buf,"%4u,%4u,%4u",
        R.ScaleRange[0],R.ScaleRange[1],R.ScaleRange[2]);
    Serial.println(lcd_buf);
  }
  else if (!strncmp("scaleset",incoming_command_string,8))// Write new scale limits
  {
    uint8_t r1, r2, r3;	
	
    r1 = strtol(incoming_command_string+8,&pEnd,10);
    r2 = strtol(pEnd,&pEnd,10);
    r3 = strtol(pEnd,&pEnd,10);
    // Bounds dependencies check and adjust
    //
    // Scales 2 and 3 cannot ever be larger than 9.9 times Scale 1
    // Scale 2 is equal to or larger than Scale 1
    // Scale 3 is equal to or larger than Scale 2
    // If two scales are equal, then only two Scale Ranges in effect
    // If all three scales are equal, then only one Scale Range is in effect
    // If Scale 1 is being adjusted, Scales 2 and 3 can be pushed up or down as a consequence
    // If Scale 2 is being adjusted up, Scale 3 can be pushed up
    // If Scale 3 is being adjusted down, Scale 2 can be pushed down
    if (r2 >= r1*10) r2 = r1*10 - 1;
    if (r3 >= r1*10) r3 = r1*10 - 1;
    // Ranges 2 and 3 cannot be smaller than range 1
    if (r2 < r1) r2 = r1;
    if (r3 < r1) r3 = r1;
    // Range 2 cannot be larger than range 3
    if (r2 > r3) r3 = r2;

    EEPROM_readAnything(1,R);
    R.ScaleRange[0] = r1;
    R.ScaleRange[1] = r2;
    R.ScaleRange[2] = r3;
    EEPROM_writeAnything(1,R);
  }

  else if (!strcmp("version",incoming_command_string))    // Poll for one single Human Readable report
  {
    #if defined(__MK20DX256__)                            // If Teensy 3.1 ARM Cortex M4 microcontroller
    Serial.println(F("TF3LJ/VE2LJX Teensy 3.1 based Power & SWR Meter"));   
    #else
    Serial.println(F("TF3LJ/VE2LJX AT90USB1286 based Power & SWR Meter"));
    #endif
    Serial.print(F("Version "));
    Serial.print(VERSION);
    Serial.print(F(" "));
    Serial.println(DATE);
    Serial.println();
  }

  else if (!strncmp("sleepmsg=",incoming_command_string,9))// A new "sleep message" string was received
  {
    EEPROM_readAnything(1,R);
    // Copy up to 20 characters of the received string
    strncpy(R.idle_disp, incoming_command_string+9,20);
    // and store in EEPROM
    EEPROM_writeAnything(1,R);
  }
  //
  // The below are a bit redundant, as they are fully manageable by the rotary encoder:
  //
  //    $sleeppwrset x    Power below the level defined here will put display into screensaver mode.
  //                      x = 0.001, 0.01, 0.1, 1 or 10 mW (milliwatts)
  //    $sleeppwrget      Return current value	
  else if (!strncmp("sleeppwrset",incoming_command_string,11))
  {
    // Write value if valid
    inp_double = strtod(incoming_command_string+11,&pEnd);
    if ((inp_double==0)||(inp_double==0.001)||(inp_double==0.01)||(inp_double==0.1)||(inp_double==1)||(inp_double==10))
    {
      EEPROM_readAnything(1,R);
      R.idle_disp_thresh = (float) inp_double;
      EEPROM_writeAnything(1,R);
    }
  }
  else if (!strcmp("sleeppwrget",incoming_command_string))
  {
    Serial.print(F("IdleDisplayThreshold (mW): "));
    Serial.println(R.idle_disp_thresh,3);
  }

  //    $alarmset x       x = 1.5 to 3.9. 4 will inactivate SWR Alarm function
  //    $alarmget         Return current value
  else if (!strncmp("alarmset",incoming_command_string,8))
  {
    // Write value if valid
    inp_double = strtod(incoming_command_string+8,&pEnd);
    inp_val = inp_double * 10;
    if ((inp_val>14) && (inp_val<=40))
    {
      EEPROM_readAnything(1,R);
      R.SWR_alarm_trig = inp_val;
      EEPROM_writeAnything(1,R);
    }
  }
  else if (!strcmp("alarmget",incoming_command_string))
  {
    Serial.print(F("SWR_Alarm_Trigger: "));
    Serial.println(R.SWR_alarm_trig/10.0,1);
  }

  //    $alarmpowerset x    x = 1, 10, 100, 1000 or 10000 mW (milliwatts)
  //    $alarmpowerget      Return current value
  else if (!strncmp("alarmpowerset",incoming_command_string,13))
  {
    // Write value if valid
    inp_val = strtol(incoming_command_string+13,&pEnd,10);
    if ((inp_val==1)||(inp_val==10)||(inp_val==100)||(inp_val==1000)||(inp_val==10000))
    {
      EEPROM_readAnything(1,R);
      R.SWR_alarm_pwr_thresh = inp_val;
      EEPROM_writeAnything(1,R);
    }
  }
  else if (!strcmp("alarmpowerget",incoming_command_string))
  {
    Serial.print(F("SWR_Alarm_Power_Threshold (mW): "));
    Serial.println(R.SWR_alarm_pwr_thresh);
  }

  //    $pepperiodget x    x = 1, 2.5 or 5 seconds.  PEP sampling period
  //    $pepperiodset      Return current value
  else if (!strncmp("pepperiodset",incoming_command_string,12))
  {
    // Write value if valid
    inp_double = strtod(incoming_command_string+12,&pEnd);
    inp_val = inp_double*200;
    if ((inp_val==200)||(inp_val==500)||(inp_val==1000))
    {
      EEPROM_readAnything(1,R);
      R.PEP_period = inp_val;
      EEPROM_writeAnything(1,R);
    }
  }
  else if (!strcmp("pepperiodget",incoming_command_string))
  {
    Serial.print(F("PEP_period (seconds): "));
    Serial.println(R.PEP_period/200.0,1);
  }
}	
	

//
//-----------------------------------------------------------------------------------------
// 			Monitor USB Serial port for an incoming USB command
//-----------------------------------------------------------------------------------------
//
void usb_read_serial(void)
{
  static uint8_t a;             // Indicates number of chars received in an incoming command
  static uint8_t Incoming;      // BOOL, TRUE or FALSE
  uint8_t ReceivedChar;
  uint8_t waiting;              // Number of chars waiting in receive buffer

  //int16_t r;
  //uint8_t count=0;

  // Find out how many characters are waiting to be read. 
  waiting = Serial.available();

  // Scan for a command attention symbol -> '$'
  if (waiting && (Incoming == FALSE))
  {
    ReceivedChar = Serial.read();
    // A valid incoming message starts with an "attention" symbol = '$'.
    // in other words, any random garbage received on USB port is ignored.
    if (ReceivedChar == '$')    // Start command symbol was received,
    {                           // we can begin processing input command
      Incoming = TRUE;
      a = 0;
      waiting--;
    }
  }

  // Input command is on its way.  One or more characters are waiting to be read
  // and Incoming flag has been set. Read any available bytes from the USB OUT endpoint
  while (waiting && Incoming)
  {
    ReceivedChar = Serial.read();
    waiting--;

    if (a == sizeof(incoming_command_string)-1)  // Line is too long, discard input
    {
      Incoming = FALSE;
      a = 0;
    }
    // Check for End of line
    else if ((ReceivedChar=='\r') || (ReceivedChar=='\n'))
    {
      incoming_command_string[a] = 0;            // Terminate line
      usb_parse_incoming();                      // Parse the command
      Incoming = FALSE;
      a = 0;
    }
    else                                         // Receive message, char by char
    {
      incoming_command_string[a] = ReceivedChar;
    }
    a++;                                         // String length count++
  }
}
