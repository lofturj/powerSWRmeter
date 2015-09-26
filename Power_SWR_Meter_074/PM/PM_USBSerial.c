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
//*********************************************************************************

#include <stdlib.h>

#include "PM.h"

char incoming_command_string[50];			// Input from USB Serial
		

//
//-----------------------------------------------------------------------------------------
// 			Send AD8307 measurement data to the Computer
//
//			Format of transmitted data is:
//			Incident (real transmitted) power, VSWR, [R, jX, Phase]
//			(R, jX and Phase only available if Phase Meter
//-----------------------------------------------------------------------------------------
//
void usb_poll_data(void)
{
	//------------------------------------------
	// Power indication, incident power
	if (Reverse) usb_serial_write("-",1);
	//print_p_mw(power_mw);
	sprintf(lcd_buf,"%1.8f, ", power_mw/1000);
	usb_serial_write(lcd_buf,strlen(lcd_buf));

	//------------------------------------------
	// SWR indication
	//print_swr();
	sprintf(lcd_buf,"%1.02f",swr);
	usb_serial_write(lcd_buf,strlen(lcd_buf));

	#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
	usb_serial_write(", ",2);
	//------------------------------------------
	// R + jX indication
	sprintf(lcd_buf,"%4.1f, %4.1f", imp_R*50, imp_jX*50);
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	#endif

	sprintf(lcd_buf, "\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
}

void usb_poll_inst(void)
{
	//------------------------------------------
	// Power indication, instantaneous power, formatted, pW-kW
	print_p_mw(power_mw);
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	//------------------------------------------
	// SWR indication
	usb_serial_write(", VSWR",6);
	print_swr();
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	
	sprintf(lcd_buf, "\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
}
void usb_poll_pk(void)
{
	//------------------------------------------
	// Power indication, 100ms peak power, formatted, pW-kW
	print_p_mw(power_mw_pk);
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	//------------------------------------------
	// SWR indication
	usb_serial_write(", VSWR",6);
	print_swr();
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	
	sprintf(lcd_buf, "\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
}
void usb_poll_pep(void)
{
	//------------------------------------------
	// Power indication, PEP power, formatted, pW-kW
	print_p_mw(power_mw_pep);
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	//------------------------------------------
	// SWR indication
	usb_serial_write(", VSWR",6);
	print_swr();
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	
	sprintf(lcd_buf, "\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
}
void usb_poll_avg(void)
{
	//------------------------------------------
	// Power indication, 1s average power, formatted, pW-kW
	print_p_mw(power_mw_avg);
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	//------------------------------------------
	// SWR indication
	usb_serial_write(", VSWR",6);
	print_swr();
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	
	sprintf(lcd_buf, "\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
}

void usb_poll_instdb(void)
{
	//------------------------------------------
	// Power indication, instantaneous power, formatted, dB
	print_dbm((int16_t) (power_db*10.0));
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	//------------------------------------------
	// SWR indication
	usb_serial_write(", VSWR",6);
	print_swr();
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	
	sprintf(lcd_buf, "\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
}
void usb_poll_pkdb(void)
{
	//------------------------------------------
	// Power indication, 100ms peak power, formatted, dB
	print_dbm((int16_t) (power_db_pk*10.0));
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	//------------------------------------------
	// SWR indication
	usb_serial_write(", VSWR",6);
	print_swr();
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	
	sprintf(lcd_buf, "\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
}
void usb_poll_pepdb(void)
{
	//------------------------------------------
	// Power indication, PEP power, formatted, dB
	print_dbm((int16_t) (power_db_pep*10.0));
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	//------------------------------------------
	// SWR indication
	usb_serial_write(", VSWR",6);
	print_swr();
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	
	sprintf(lcd_buf, "\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
}
void usb_poll_avgdb(void)
{
	//------------------------------------------
	// Power indication, 1s average power, formatted, dB
	print_dbm((int16_t) (power_db_avg*10.0));
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	//------------------------------------------
	// SWR indication
	usb_serial_write(", VSWR",6);
	print_swr();
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	
	sprintf(lcd_buf, "\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
}

//
//-----------------------------------------------------------------------------------------
// 			Send AD8307 measurement data to the Computer
//
//			Long and Human Readable Format
//-----------------------------------------------------------------------------------------
//
void usb_poll_long(void)
{
	//------------------------------------------
	// Power indication, inst, peak (100ms), pep (1s), average (1s)
	sprintf(lcd_buf, "Power (inst, peak 100ms, pep 1s, avg 1s):\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	if (Reverse) usb_serial_write("-",1);
	print_p_mw(power_mw);
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	usb_serial_write(", ",2);
	if (Reverse) usb_serial_write("-",1);
	print_p_mw(power_mw_pk);
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	usb_serial_write(", ",2);
	if (Reverse) usb_serial_write("-",1);
	print_p_mw(power_mw_pep);
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	usb_serial_write(", ",2);
	if (Reverse) usb_serial_write("-",1);
	print_p_mw(power_mw_avg);
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	sprintf(lcd_buf, "\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
		
	//------------------------------------------
	// Forward and Reflected Power indication, instantaneous only
	sprintf(lcd_buf, "Forward and Reflected Power (inst):\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	if (Reverse) usb_serial_write("-",1);
	print_p_mw(fwd_power_mw);
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	usb_serial_write(", ",2);
	if (Reverse) usb_serial_write("-",1);
	print_p_mw(ref_power_mw);
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	sprintf(lcd_buf, "\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));

	//------------------------------------------
	// SWR indication
	usb_serial_write("VSWR",4);
	print_swr();
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	sprintf(lcd_buf, "\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
		
	#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
	//------------------------------------------
	// R + jX indication
	sprintf(lcd_buf, "R + jX:\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	sprintf(lcd_buf,"%4.1f, %4.1f", imp_R*50, imp_jX*50);
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	sprintf(lcd_buf, "\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
	#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection

	sprintf(lcd_buf, "\r\n");
	usb_serial_write(lcd_buf,strlen(lcd_buf));
}


//
//-----------------------------------------------------------------------------------------
// 			Parse and act upon an incoming USB command
//
//			Commands are:
//
//			$ppoll				Poll for one single USB serial report, inst power (unformatted)
//			$pinst				Poll for one single USB serial report, inst power (human readable)
//			$ppk				Poll for one single USB serial report, 100ms peak power (human readable)
//			$ppep				Poll for one single USB serial report, pep power (human readable)
//			$pavg				Poll for one single USB serial report, avg power (human readable)
//			$pinstdb			Poll for one single USB serial report, inst power in dB (human readable)
//			$ppkdb				Poll for one single USB serial report, 100ms peak power in dB (human readable)
//			$ppepdb				Poll for one single USB serial report, pep power in dB (human readable)
//			$pavgdb				Poll for one single USB serial report, avg power in dB (human readable)
//			$plong				Poll for one single USB serial report, actual power (inst, pep and avg)
//								as well as fwd power, reflected power and SWR (long form)
//
//			$pcont				USB serial reporting in a continuous mode, 10 times per second
//
//								$ppoll, $pinst, $ppk, $ppep, $pavg$ or plong entered after $pcont will
//								switch back to single shot mode
//
//			$sleepmsg=abcdefg	Where abcdefg is a free text string to be displayed when
//			                    in screensaver mode, up to 20 characters max
//			$sleeppwrset x		Power below the level defined here will put display into screensaver mode.
//								x = 0.001, 0.01, 0.1, 1 or 10 mW (milliWatts)
//			$sleeppwrget		Return current value
//
//			$alarmset x			x = 1.5 to 3.9. 4 will inactivate SWR Alarm function
//			$alarmget			Return current value
//			$alarmpowerset x	x = 1, 10, 100, 1000 or 10000 mW (milliwatts)
//			$alarmpowerget		Return current value
//
//			$pepperiodget x		x = 1, 2.5 or 5 seconds.  PEP sampling period
//			$pepperiodset		Return current value
//
//			$calset cal1 AD1-1 AD2-1 cal2 AD1-2 AD2-2
//								Write new calibration values to the meter
//			$calget				Retrieve calibration values
//
//								Format of calibration values is:
//								cal1 AD1-1 AD2-1 cal2 AD1-2 AD2-2
//									where:
//								cal1 and cal2 are calibration setpoints 1 and 2 in 10x dBm
//									and
//								ADx-1 and ADx-2 are the corresponding AD values for
//								AD1 (forward direction) and AD2 (reverse direction).
//								(
//								 normally the AD1 and AD2 values for each setpoint would be the same, 
//								 however by using the $swrcalset command it is possible to balance any
//								 small differences there might be between the two AD8307 outputs.
//								 Note that I have not found this to be necessary at all :)
//								)
//
//			$scaleget			Retrieve user definable scale ranges
//			$scaleset			Write new  scale ranges
//
//								The scale ranges are user definable, up to 3 ranges per decade
//								e.g. 6, 12 and 24 gives:
//								... 6mW, 12mW, 24mW, 60mW ... 1.2W, 2.4W, 6W 12W 24W 60W 120W ...
//								If all three values set as "2", then
//								... 2W, 20W, 200W ...
//								The third and largest value has to be less than ten times the first value
//
//			$encset x			x = Rotary Encoder Resolution, integer number, 2 - 16.
//			$encget				Return current value
//
//			$version			Report version and date of firmware
//
//	In addition, the Power & Impedance Meter version of the code supports the following commands:
//
//			$phasesetu a.aaaa b.bbbb c.cccc		Commands to enter a set of calibrated MCK12140+MC100ELT23
//				and								phase detector reference voltages at +/-90 degrees and 0 degrees.
//			$phasesetd d.dddd e.eeee f.ffff		Separate commands for each Phase Voltage Output Pin (U and D).
//
//
//			$phasegetu			These two commands are used to retrieve the current phase detector 
//				and				calibration settings.  Separate commands for each Phase Voltage
//			$phasegetd			Output Pin (U and D).
//
//
//								As the phase detector has two possible solutions for every situation, 
//								both solutions need to be established.  First value is voltage at
//								+90 degrees.  Second value is voltage at 0 degrees.  Third value is
//								voltage at -90 degrees.  Typical values for a, b, c and d, e, f are:
//								0.9054 1.7737 2.6520
// 								2.6828 1.7726 0.9127
//
//-----------------------------------------------------------------------------------------
//
void usb_parse_incoming(void)
{
	char *pEnd;
	uint16_t inp_val;
	double inp_double;
	
	if (!strcmp("ppoll",incoming_command_string))			// Poll, if Continuous mode, then switch into Polled Mode
	{
		// Disable continuous USB report mode ($pcont) if previously set
		// and Write report mode to EEPROM, if changed
		if (!(R.USB_Flags & USBPPOLL)||(R.USB_Flags & USBPCONT))
		{
			R.USB_Flags = USBPPOLL;
			eeprom_write_block(&R.USB_Flags, &E.USB_Flags, sizeof(R.USB_Flags));
		}
		usb_poll_data();									// Send data over USB
	}
	else if (!strcmp("pinst",incoming_command_string))	// Poll for one single Human Readable report
	{
		// Disable continuous USB report mode ($pcont) if previously set
		// and Write report mode to EEPROM, if changed 
		if (!(R.USB_Flags & USBPINST)||(R.USB_Flags & USBPCONT))
		{
			R.USB_Flags = USBPINST;
			eeprom_write_block(&R.USB_Flags, &E.USB_Flags, sizeof(R.USB_Flags));
		}
		usb_poll_inst();
	}
	else if (!strcmp("ppk",incoming_command_string))	// Poll for one single Human Readable report
	{
		// Disable continuous USB report mode ($pcont) if previously set
		// and Write report mode to EEPROM, if changed
		if (!(R.USB_Flags & USBPPK)||(R.USB_Flags & USBPCONT))
		{
			R.USB_Flags = USBPPK;
			eeprom_write_block(&R.USB_Flags, &E.USB_Flags, sizeof(R.USB_Flags));
		}
		usb_poll_pep();
	}
	else if (!strcmp("ppep",incoming_command_string))	// Poll for one single Human Readable report
	{
		// Disable continuous USB report mode ($pcont) if previously set
		// and Write report mode to EEPROM, if changed
		if (!(R.USB_Flags & USBPPEP)||(R.USB_Flags & USBPCONT))
		{
			R.USB_Flags = USBPPEP;
			eeprom_write_block(&R.USB_Flags, &E.USB_Flags, sizeof(R.USB_Flags));
		}
		usb_poll_pep();
	}
	else if (!strcmp("pavg",incoming_command_string))	// Poll for one single Human Readable report
	{
		// Disable continuous USB report mode ($pcont) if previously set
		// and Write report mode to EEPROM, if changed
		if (!(R.USB_Flags & USBPAVG)||(R.USB_Flags & USBPCONT))
		{
			R.USB_Flags = USBPAVG;
			eeprom_write_block(&R.USB_Flags, &E.USB_Flags, sizeof(R.USB_Flags));
		}
		usb_poll_avg();
	}
	else if (!strcmp("pinstdb",incoming_command_string))	// Poll for one single Human Readable report
	{
		// Disable continuous USB report mode ($pcont) if previously set
		// and Write report mode to EEPROM, if changed
		if (((R.USB_Flags&(USBPINST|USBP_DB))  !=(USBPINST|USBP_DB)) || (R.USB_Flags & USBPCONT))
		{
			R.USB_Flags = USBPINST|USBP_DB;
			eeprom_write_block(&R.USB_Flags, &E.USB_Flags, sizeof(R.USB_Flags));
		}
		usb_poll_instdb();
	}
	else if (!strcmp("ppkdb",incoming_command_string))	// Poll for one single Human Readable report
	{
		// Disable continuous USB report mode ($pcont) if previously set
		// and Write report mode to EEPROM, if changed
		if (((R.USB_Flags&(USBPPK|USBP_DB))!=(USBPPK|USBP_DB)) || (R.USB_Flags & USBPCONT))
		{
			R.USB_Flags = USBPPK|USBP_DB;
			eeprom_write_block(&R.USB_Flags, &E.USB_Flags, sizeof(R.USB_Flags));
		}
		usb_poll_pkdb();
	}
	else if (!strcmp("ppepdb",incoming_command_string))	// Poll for one single Human Readable report
	{
		// Disable continuous USB report mode ($pcont) if previously set
		// and Write report mode to EEPROM, if changed
		if (((R.USB_Flags&(USBPPEP|USBP_DB))!=(USBPPEP|USBP_DB)) || (R.USB_Flags & USBPCONT))
		{
			R.USB_Flags = USBPPEP|USBP_DB;
			eeprom_write_block(&R.USB_Flags, &E.USB_Flags, sizeof(R.USB_Flags));
		}
		usb_poll_pepdb();
	}
	else if (!strcmp("pavgdb",incoming_command_string))	// Poll for one single Human Readable report
	{
		// Disable continuous USB report mode ($pcont) if previously set
		// and Write report mode to EEPROM, if changed
		if (((R.USB_Flags&(USBPAVG|USBP_DB))!=(USBPAVG|USBP_DB)) || (R.USB_Flags & USBPCONT))
		{
			R.USB_Flags = USBPAVG|USBP_DB;
			eeprom_write_block(&R.USB_Flags, &E.USB_Flags, sizeof(R.USB_Flags));
		}
		usb_poll_avgdb();
	}

	else if (!strcmp("plong",incoming_command_string))	// Poll for one single Human Readable report
	{
		// Disable continuous USB report mode ($pcont) if previously set
		// and Write report mode to EEPROM, if changed
		if (!(R.USB_Flags & USBPLONG)||(R.USB_Flags & USBPCONT))
		{
			R.USB_Flags = USBPLONG;
			eeprom_write_block(&R.USB_Flags, &E.USB_Flags, sizeof(R.USB_Flags));
		}
		usb_poll_long();
	}
	else if (!strcmp("pcont",incoming_command_string))		// Switch into Continuous Mode
	{
		// Enable continuous USB report mode ($pcont), and write to EEPROM, if previously disabled
		if ((R.USB_Flags & USBPCONT) == 0)
		{
			R.USB_Flags |= USBPCONT;
			eeprom_write_block(&R.USB_Flags, &E.USB_Flags, sizeof(R.USB_Flags));
		}
	}

	else if (!strcmp("calget",incoming_command_string))	// Retrieve calibration values
	{
		#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
		sprintf(lcd_buf,"Cal: %4d,%4d,%4d,%4d,%4d,%4d\r\n",
		R.cal_AD[0].db10m,R.cal_AD[0].V,R.cal_AD[0].I,
		R.cal_AD[1].db10m,R.cal_AD[1].V,R.cal_AD[1].I);
		usb_serial_write(lcd_buf,strlen(lcd_buf));
		#else				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
		sprintf(lcd_buf,"Cal: %4d,%4d,%4d,%4d,%4d,%4d\r\n", 
			R.cal_AD[0].db10m,R.cal_AD[0].Fwd,R.cal_AD[0].Rev,
			R.cal_AD[1].db10m,R.cal_AD[1].Fwd,R.cal_AD[1].Rev);
		usb_serial_write(lcd_buf,strlen(lcd_buf));
		#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
	}
	else if (!strncmp("calset",incoming_command_string,6))	// Write new calibration values
	{
		#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
		R.cal_AD[0].db10m = strtol(incoming_command_string+6,&pEnd,10);
		R.cal_AD[0].V = strtol(pEnd,&pEnd,10);
		R.cal_AD[0].I = strtol(pEnd,&pEnd,10);
		R.cal_AD[1].db10m = strtol(pEnd,&pEnd,10);
		R.cal_AD[1].V = strtol(pEnd,&pEnd,10);
		R.cal_AD[1].I = strtol(pEnd,&pEnd,10);
		#else				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
		R.cal_AD[0].db10m = strtol(incoming_command_string+6,&pEnd,10);
		R.cal_AD[0].Fwd = strtol(pEnd,&pEnd,10);
		R.cal_AD[0].Rev = strtol(pEnd,&pEnd,10);
		R.cal_AD[1].db10m = strtol(pEnd,&pEnd,10);
		R.cal_AD[1].Fwd = strtol(pEnd,&pEnd,10);
		R.cal_AD[1].Rev = strtol(pEnd,&pEnd,10);
		#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection

		eeprom_write_block(&R.cal_AD[0], &E.cal_AD[0], sizeof (R.cal_AD[0]));
		eeprom_write_block(&R.cal_AD[1], &E.cal_AD[1], sizeof (R.cal_AD[1]));
	}

	else if (!strcmp("scaleget",incoming_command_string))		// Retrieve scale limits
	{
		sprintf(lcd_buf,"Scale: %4u,%4u,%4u\r\n",
			R.ScaleRange[0],R.ScaleRange[1],R.ScaleRange[2]);
		usb_serial_write(lcd_buf,strlen(lcd_buf));
	}
	else if (!strncmp("scaleset",incoming_command_string,8))	// Write new scale limits
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

		R.ScaleRange[0] = r1;
		R.ScaleRange[1] = r2;
		R.ScaleRange[2] = r3;
		eeprom_write_block(&R.ScaleRange, &E.ScaleRange, sizeof (R.ScaleRange));
	}

	else if (!strcmp("version",incoming_command_string))		// Poll for one single Human Readable report
	{
		#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
		sprintf(lcd_buf,"TF3LJ/VE2LJX AT90USB1286 based Power & Impedance Meter\r\n");
		#else				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
		sprintf(lcd_buf,"TF3LJ/VE2LJX AT90USB1286 based Power & SWR Meter\r\n");
		#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
		usb_serial_write(lcd_buf,strlen(lcd_buf));
		sprintf(lcd_buf,"Version "VERSION" "DATE"\r\n");
		usb_serial_write(lcd_buf,strlen(lcd_buf));
	}

	else if (!strncmp("sleepmsg=",incoming_command_string,9))// A new "sleep message" string was received
	{
		// Copy up to 20 characters of the received string
		strncpy(R.idle_disp, incoming_command_string+9,20);
		// and store in EEPROM
		eeprom_write_block(&R.idle_disp, &E.idle_disp, sizeof (R.idle_disp));
	}
	//
	// The below are a bit redundant, as they are fully manageable by the rotary encoder:
	//
	//	$sleeppwrset x		Power below the level defined here will put display into screensaver mode.
	//								x = 0.001, 0.01, 0.1, 1 or 10 mW (milliwatts)
	//	$sleeppwrget		Return current value	
	else if (!strncmp("sleeppwrset",incoming_command_string,11))
	{
		// Write value if valid
		inp_double = strtod(incoming_command_string+11,&pEnd);
		if ((inp_double==0)||(inp_double==0.001)||(inp_double==0.01)||(inp_double==0.1)||(inp_double==1)||(inp_double==10))
		{
			R.idle_disp_thresh = (float) inp_double;
			eeprom_write_block(&R.idle_disp_thresh, &E.idle_disp_thresh, sizeof (R.idle_disp_thresh));
		}
	}
	else if (!strcmp("sleeppwrget",incoming_command_string))
	{
		sprintf(lcd_buf,"IdleDisplayThreshold (mW): %1.03f\r\n",(double)R.idle_disp_thresh);
		usb_serial_write(lcd_buf,strlen(lcd_buf));
	}

	//	$alarmset x			x = 1.5 to 3.9. 4 will inactivate SWR Alarm function
	//	$alarmget			Return current value
	else if (!strncmp("alarmset",incoming_command_string,8))
	{
		// Write value if valid
		inp_double = strtod(incoming_command_string+8,&pEnd);
		inp_val = inp_double * 10;
		if ((inp_val>14) && (inp_val<=40))
		{
			R.SWR_alarm_trig = inp_val;
			eeprom_write_block(&R.SWR_alarm_trig, &E.SWR_alarm_trig, sizeof (R.SWR_alarm_trig));
		}
	}
	else if (!strcmp("alarmget",incoming_command_string))
	{
		sprintf(lcd_buf,"SWR_Alarm_Trigger: %1.01f\r\n",R.SWR_alarm_trig/10.0);
		usb_serial_write(lcd_buf,strlen(lcd_buf));
	}

	// $alarmpowerset x		x = 1, 10, 100, 1000 or 10000 mW (milliwatts)
	// $alarmpowerget		Return current value
	else if (!strncmp("alarmpowerset",incoming_command_string,13))
	{
		// Write value if valid
		inp_val = strtol(incoming_command_string+13,&pEnd,10);
		if ((inp_val==1)||(inp_val==10)||(inp_val==100)||(inp_val==1000)||(inp_val==10000))
		{
			R.SWR_alarm_pwr_thresh = inp_val;
			eeprom_write_block(&R.SWR_alarm_pwr_thresh, &E.SWR_alarm_pwr_thresh, sizeof (R.SWR_alarm_pwr_thresh));
		}
	}
	else if (!strcmp("alarmpowerget",incoming_command_string))
	{
		sprintf(lcd_buf,"SWR_Alarm_Power_Threshold: %u\r\n",R.SWR_alarm_pwr_thresh);
		usb_serial_write(lcd_buf,strlen(lcd_buf));
	}

	//	$pepperiodget x		x = 1, 2.5 or 5 seconds.  PEP sampling period
	//	$pepperiodset		Return current value
	else if (!strncmp("pepperiodset",incoming_command_string,12))
	{
		// Write value if valid
		inp_double = strtod(incoming_command_string+12,&pEnd);
		inp_val = inp_double*200;
		if ((inp_val==200)||(inp_val==500)||(inp_val==1000))
		{
			R.PEP_period = inp_val;
			eeprom_write_block(&R.PEP_period, &E.PEP_period, sizeof (R.PEP_period));
		}
	}
	else if (!strcmp("pepperiodget",incoming_command_string))
	{
		sprintf(lcd_buf,"PEP_period: %1.01f\r\n",R.PEP_period/200.0);
		usb_serial_write(lcd_buf,strlen(lcd_buf));
	}

	//	$pencset x		x = Rotary Encoder Resolution, integer number, 1 to 8
	//	$pencget		Return current value
	else if (!strncmp("encset",incoming_command_string,6))
	{
		// Write value if valid
		inp_val = strtol(incoming_command_string+6,&pEnd,10);
		if ((inp_val>0) && (inp_val<=8))
		{
			R.encoderRes = inp_val;
			eeprom_write_block(&R.encoderRes, &E.encoderRes, sizeof (R.encoderRes));
		}
	}
	else if (!strcmp("encget",incoming_command_string))
	{
		sprintf(lcd_buf,"Rotary_Encoder_Resolution: %u\r\n",R.encoderRes);
		usb_serial_write(lcd_buf,strlen(lcd_buf));
	}

	#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
	else if (!strcmp("phasegetu",incoming_command_string))	// Retrieve Phase calibration values
	{
		sprintf(lcd_buf,"PhaseU: %1.04f %1.04f %1.04f\r\n",
		R.U.pos90deg, R.U.zerodeg, R.U.neg90deg);
		usb_serial_write(lcd_buf,strlen(lcd_buf));
	}
	else if (!strcmp("phasegetd",incoming_command_string))	// Retrieve Phase calibration values
	{
		sprintf(lcd_buf,"PhaseD: %1.04f %1.04f %1.04f\r\n",
		R.D.pos90deg, R.D.zerodeg, R.D.neg90deg);
		usb_serial_write(lcd_buf,strlen(lcd_buf));
	}
	else if (!strncmp("phasesetu",incoming_command_string,9))	// Write new calibration values
	{
		R.U.pos90deg = strtod(incoming_command_string+9,&pEnd);
		R.U.zerodeg = strtod(pEnd,&pEnd);
		R.U.neg90deg = strtod(pEnd,&pEnd);
		eeprom_write_block(&R.U, &E.U, sizeof (R.U));
	}
	else if (!strncmp("phasesetd",incoming_command_string,9))	// Write new calibration values
	{
		R.D.pos90deg = strtod(incoming_command_string+9,&pEnd);
		R.D.zerodeg = strtod(pEnd,&pEnd);
		R.D.neg90deg = strtod(pEnd,&pEnd);
		eeprom_write_block(&R.D, &E.D, sizeof (R.D));
	}
	#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
}	
	

//
//-----------------------------------------------------------------------------------------
// 			Monitor USB Serial port for an incoming USB command
//-----------------------------------------------------------------------------------------
//
void usb_read_serial(void)
{
	static uint8_t a;		// Indicates number of chars received in an incoming command
	static BOOL Incoming;
	uint8_t ReceivedChar;
	uint8_t waiting;		// Number of chars waiting in receive buffer
	
	//int16_t r;
	//uint8_t count=0;

	// Find out how many characters are waiting to be read. 
	waiting = usb_serial_available();

	// Scan for a command attention symbol -> '$'
	if (waiting && (Incoming == FALSE))
	{
		ReceivedChar = usb_serial_getchar();
		// A valid incoming message starts with an "attention" symbol = '$'.
		// in other words, any random garbage received on USB port is ignored.
		if (ReceivedChar == '$')			// Start command symbol was received,
		{									// we can begin processing input command
			Incoming = TRUE;
			a = 0;
			waiting--;
		}
	}
	
	// Input command is on its way.  One or more characters are waiting to be read
	// and Incoming flag has been set. Read any available bytes from the USB OUT endpoint
	while (waiting && Incoming)
	{
		ReceivedChar = usb_serial_getchar();
		waiting--;

		if (a == sizeof(incoming_command_string)-1)	// Line is too long, discard input
		{
			Incoming = FALSE;
			a = 0;
		}
		// Check for End of line
		else if ((ReceivedChar=='\r') || (ReceivedChar=='\n'))
		{
			incoming_command_string[a] = 0;	// Terminate line
			usb_parse_incoming();			// Parse the command
			Incoming = FALSE;
			a = 0;
		}
		else								// Receive message, char by char
		{
			incoming_command_string[a] = ReceivedChar;
		}
		a++;								// String length count++
	}
}
