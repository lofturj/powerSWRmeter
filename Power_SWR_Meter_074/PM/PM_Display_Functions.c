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

#include 	<math.h>
#include	<stdlib.h>
#include	"PM.h"


//
//-----------------------------------------------------------------------------
//		Screensaver display
//-----------------------------------------------------------------------------
//
void screensaver(void)
{
	uint8_t x, y;
	static uint8_t count = SLEEPTIME-1;	// Force screensaver print when run first time
		
	count++;

	// Force screensaver reprint if flag
	if (Status&IDLE_REFRESH)
	{
		Status &= ~IDLE_REFRESH;	// Clear flag
		count = SLEEPTIME;			// and force a reprint
	}

	if (count == SLEEPTIME)	// We reprint the output every fiftieth time (approx 5 sec)
	{
		count = 0;
		lcdClear();
		y = rand() % 4;
		x = rand() % (21 - strlen(R.idle_disp));
		lcdGotoXY(x,y);
		lcdPrintData(R.idle_disp,strlen(R.idle_disp));
	}
}
	

//
//-----------------------------------------------------------------------------
//			Display Mode Intro
//-----------------------------------------------------------------------------
//
void lcd_display_mode_intro(char *line1, char *line2, char *line3, char *line4)
{
	lcdClear();
	lcdGotoXY(0,0);
	lcdPrintData(line1, strlen(line1));
	lcdGotoXY(0,1);
	lcdPrintData(line2, strlen(line2));
	lcdGotoXY(0,2);
	lcdPrintData(line3, strlen(line3));
	lcdGotoXY(0,3);
	lcdPrintData(line4, strlen(line4));
}


//
//-----------------------------------------------------------------------------------------
// 			Scale Bargraph based on highest instantaneous power reading during last 10 seconds
//          (routine is called once every 1/10th of a second
//
//			The scale ranges are user definable, up to 3 ranges per decade
//			e.g. 6, 12 and 24 gives:
//			... 6mW, 12mW, 24mW, 60mW ... 1.2W, 2.4W, 6W 12W 24W 60W 120W ...
//			If all three values set as "2", then: 
//			... 2W, 20W, 200W ...
//			The third and largest value has to be less than ten times the first value 
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
// 			Scale Values to fit Power Bargraph (16 bit integer)
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


#if PHASE_DETECTOR			// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
//
//-----------------------------------------------------------------------------
//			Display Bargraph, Power, SWR and R + jX
//			(input power is in mW)
//-----------------------------------------------------------------------------
//
void lcd_display_rjx(char * introtext, double power)
{
	static uint8_t mode_timer = 0;			// Used to time the Display Mode intro
	
	uint32_t scale;							// Progress bar scale

	// Used to scale/fit progressbar inputs into 16 bit variables, when needed
	uint16_t bar_power=0, bar_power_pep=0, bar_scale=0;
											
	scale = scale_BAR((uint32_t) power_mw);	// Determine scale setting, also used to
											// determine if level above useful threshold
											
	//------------------------------------------
	// Display mode intro for a time
	if(Status & MODE_DISPLAY)
	{
		if(Status & MODE_CHANGE)
		{
			Status &= ~MODE_CHANGE;			// Clear display change mode
			mode_timer = 0;					// New mode, reset timer
			lcd_display_mode_intro("Mode:"," ",introtext,"Bargraph & Impedance");
		}

		mode_timer++;
		if (mode_timer >= MODE_INTRO_TIME)	// MODE_INTRO_TIME in tenths of seconds
		{
			mode_timer = 0;
			Status &= ~MODE_DISPLAY;		// Clear display change mode
			Status |= IDLE_REFRESH;			// Force screensaver reprint upon exit, if screensaver mode
			lcdGotoXY(0,3);					// Clear junk in line 4, in case nothing to print
			lcdPrintData("                    ",20);

		}
	}
	
	//----------------------------------------------
	// Display Power if level is above useful threshold
	//
	// If input power above a minimum relevant value is detected (see PM.h)
	// or if scale indicates a value higher than 10mW
	// (= significant power within last 10 seconds)
	else if ((power > R.idle_disp_thresh) || (scale > 10))
	{
		//------------------------------------------
		// Power indication and Bargraph
		lcdGotoXY(0,0);
		
		//------------------------------------------
		// Scale variables to fit 16bit input to lcdProgressBarPeak()
		scale_PowerBarInpValues(scale, power, power_mw_pep, &bar_scale, &bar_power, &bar_power_pep);
		//------------------------------------------
		// Power Bargraph
		lcdProgressBarPeak(bar_power,bar_power_pep,bar_scale, 14);

		//------------------------------------------
		// Wattage Printout
		print_p_mw(power);
		lcdPrintData(lcd_buf,strlen(lcd_buf));

		//------------------------------------------
		// SWR Bargraph
		lcdGotoXY(0,1);
		lcdProgressBarPeak(swr_bar, 0, 1000, 14);

		//------------------------------------------
		// SWR Printout
		lcdPrintData(" ",1);
		print_swr();						// and print the "SWR value"
		lcdPrintData(lcd_buf,strlen(lcd_buf));
		
		//------------------------------------------
		// Scale Indication
		lcdGotoXY(8,2);						// Ensure last couple of chars in line are clear
		lcdPrintData("  ",2);
		lcdGotoXY(0,2);
		lcdPrintData("Scale",5);
		print_p_reduced((double) scale);	// Scale Printout
		lcdPrintData(lcd_buf,strlen(lcd_buf));

		//------------------------------------------
		// Power indication, 1 second PEP
		lcdGotoXY(19,2);					// Clear junk in line, if any
		lcdPrintData(" ",1);
		lcdGotoXY(10,2);					// Clear junk in line, if any
		lcdPrintData(" pep",4);
		lcdGotoXY(14,2);
		print_p_mw(power_mw_pep);
		lcdPrintData(lcd_buf,strlen(lcd_buf));

		//------------------------------------------
		// Impedance as R and jX
		// Only show if input power is reaching a useful level
		if (power > 0.1)
		{
			lcdGotoXY(7,3);					// Clear junk in line, if any
			lcdPrintData("    ",4);
			lcdGotoXY(17,3);				// Clear junk in line, if any
			lcdPrintData("   ",3);
			lcdGotoXY(0,3);
			sprintf(lcd_buf,"R %4.1f   ", imp_R*50);
			lcdPrintData(lcd_buf,strlen(lcd_buf));
			lcdGotoXY(11,3);
			sprintf(lcd_buf,"jX %4.1f", imp_jX*50);
			lcdPrintData(lcd_buf,strlen(lcd_buf));
		}
	}
	
	else
		screensaver();						// Screensaver display
}
#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection


//
//-----------------------------------------------------------------------------
//			Display: Bargraph, Power in Watts, SWR & PEP Power
//			PEP Power always displayed and used for scale selection
//			power variable can be anything passed to function (power_mv_avg, power_mw_pk, etc...)
//-----------------------------------------------------------------------------
//
//void lcd_display_clean(void)
void lcd_display_clean(char * introtext, char * power_display_indicator, double power)

{
	static uint8_t mode_timer = 0;				// Used to time the Display Mode intro
	
	uint32_t scale;								// Progress bar scale

	// Used to scale/fit progressbar inputs into 16 bit variables, when needed
	uint16_t bar_power=0, bar_power_pep=0, bar_scale=0;	

	scale = scale_BAR((uint32_t) power_mw);		// Determine scale setting, also used to
												// determine if level above useful threshold

	//------------------------------------------
	// Display mode intro for a time
	if(Status & MODE_DISPLAY)
	{
		if(Status & MODE_CHANGE)
		{
			Status &= ~MODE_CHANGE;				// Clear display change mode
			mode_timer = 0;						// New mode, reset timer
			lcd_display_mode_intro("Mode:"," ",introtext,"and SWR Bargraph");
		}

		mode_timer++;
		if (mode_timer >= MODE_INTRO_TIME)		// MODE_INTRO_TIME in tenths of seconds
		{
			mode_timer = 0;
			Status &= ~MODE_DISPLAY;			// Clear display change mode
			Status |= IDLE_REFRESH;				// Force screensaver reprint upon exit, if screensaver mode
		}
	}
	
	//----------------------------------------------
	// Display Power if level is above useful threshold
	//
	// If input power above a minimum relevant value is detected (see PM.h)
	// or if scale indicates a value higher than 10mW
	// (= significant power within last 10 seconds)
	else if ((power_mw > R.idle_disp_thresh) || (scale > 10))
	{
		//------------------------------------------
		// Power indication and Bargraph
		lcdGotoXY(0,0);

		//------------------------------------------
		// Scale variables to fit 16bit input to lcdProgressBarPeak()
		scale_PowerBarInpValues(scale, power, power_mw_pep, &bar_scale, &bar_power, &bar_power_pep);
		//------------------------------------------
		// Power Bargraph
		lcdProgressBarPeak(bar_power,bar_power_pep,bar_scale, 20);

		//------------------------------------------
		// SWR Bargraph
		lcdGotoXY(0,1);
		lcdProgressBarPeak(swr_bar,0,1000, 20);

		//------------------------------------------
		// SWR Printout
		lcdGotoXY(7,2);						// Clear junk in line, if any
		lcdPrintData("   ",3);
		lcdGotoXY(0,2);
		lcdPrintData("SWR ",4);
		print_swr();						// and print the "SWR value"
		lcdPrintData(lcd_buf,strlen(lcd_buf));
		
		//------------------------------------------
		// Scale Indication
		lcdGotoXY(8,3);						// Ensure last couple of chars in line are clear
		lcdPrintData("  ",2);
		lcdGotoXY(0,3);
		lcdPrintData("Scale",5);
		print_p_reduced((double) scale);	// Scale Printout
		lcdPrintData(lcd_buf,strlen(lcd_buf));

/*		//------------------------------------------
		// Modulation Index
		lcdGotoXY(8,3);						// Ensure last couple of chars in line are clear
		lcdPrintData("  ",2);
		lcdGotoXY(0,3);
		sprintf(lcd_buf,"Index %2.02f", modulation_index);
		lcdPrintData(lcd_buf,strlen(lcd_buf));
*/
		//------------------------------------------
		// Power Indication
		lcdGotoXY(19,2);					// Clear junk in line, if any
		lcdPrintData(" ",1);
		lcdGotoXY(10,2);					// Clear junk in line, if any
		lcdPrintData(" ",1);
		lcdPrintData(power_display_indicator,3);
		if (Reverse)						// If reverse power, then indicate
		{
			lcdGotoXY(13,2);
			lcdPrintData("-",1);
		}
		lcdGotoXY(14,2);
		print_p_mw(power);
		lcdPrintData(lcd_buf,strlen(lcd_buf));

		//------------------------------------------
		// Power indication, 1 second PEP
		lcdGotoXY(19,3);					// Clear junk in line, if any
		lcdPrintData(" ",1);
		lcdGotoXY(10,3);					// Clear junk in line, if any
		lcdPrintData(" pep",4);
		lcdGotoXY(14,3);
		print_p_mw(power_mw_pep);
		lcdPrintData(lcd_buf,strlen(lcd_buf));
	}
	
	else
		screensaver();						// Screensaver display
}

//
//-----------------------------------------------------------------------------
//			Display: Bargraph, Power in dBm, SWR & PEP Power
//-----------------------------------------------------------------------------
//
void lcd_display_clean_dBm(void)
{
	static uint8_t mode_timer = 0;				// Used to time the Display Mode intro
	
	uint32_t scale;								// Progress bar scale

	// Used to scale/fit progressbar inputs into 16 bit variables, when needed
	uint16_t bar_power=0,bar_power_pep=0,bar_scale=0;	
	
	scale = scale_BAR((uint32_t) power_mw);		// Determine scale setting, also used to
												// determine if level above useful threshold

	//------------------------------------------
	// Display mode intro for a time
	if(Status & MODE_DISPLAY)
	{
		if(Status & MODE_CHANGE)
		{
			Status &= ~MODE_CHANGE;			// Clear display change mode
			mode_timer = 0;					// New mode, reset timer
			lcd_display_mode_intro("Mode:"," ","dBm Meter,  Power","and SWR Bargraph");
		}

		mode_timer++;
		if (mode_timer >= MODE_INTRO_TIME)	// MODE_INTRO_TIME in tenths of seconds
		{
			mode_timer = 0;
			Status &= ~MODE_DISPLAY;		// Clear display change mode
			Status |= IDLE_REFRESH;			// Force screensaver reprint upon exit, if screensaver mode
		}
	}
	
	//----------------------------------------------
	// Display Power if level is above useful threshold
	//
	// If input power above a minimum relevant value is detected (see PM.h)
	// or if scale indicates a value higher than 10mW
	// (= significant power within last 10 seconds)
	else if ((power_mw > R.idle_disp_thresh) || (scale > 10))
	{
		//------------------------------------------
		// Power indication and Bargraph
		lcdGotoXY(0,0);

		//------------------------------------------
		// Scale variables to fit 16bit input to lcdProgressBarPeak()
		scale_PowerBarInpValues(scale, power_mw, power_mw_pep, &bar_scale, &bar_power, &bar_power_pep);
		//------------------------------------------
		// Power Bargraph
		lcdProgressBarPeak(bar_power,bar_power_pep,bar_scale, 20);

		//------------------------------------------
		// SWR Bargraph
		lcdGotoXY(0,1);
		lcdProgressBarPeak(swr_bar,0,1000, 20);

		//------------------------------------------
		// SWR Printout
		lcdGotoXY(7,2);						// Clear junk in line, if any
		lcdPrintData("   ",3);
		lcdGotoXY(0,2);
		lcdPrintData("SWR ",4);
		print_swr();						// and print the "SWR value"
		lcdPrintData(lcd_buf,strlen(lcd_buf));
		
		//------------------------------------------
		// Scale Indication
		lcdGotoXY(8,3);			// Ensure last couple of chars in line are clear
		lcdPrintData("  ",2);
		lcdGotoXY(0,3);
		lcdPrintData("Scale",5);
		print_p_reduced((double) scale);	// Scale Printout
		lcdPrintData(lcd_buf,strlen(lcd_buf));

		//------------------------------------------
		// Power Indication
		lcdGotoXY(10,2);					// Clear junk in line, if any
		lcdPrintData(" ",1);
		lcdGotoXY(15,2);					// Clear junk in line, if any
		lcdPrintData("     ",5);
		if (Reverse)						// If reverse power, then indicate
		{
			lcdGotoXY(19,2);
			lcdPrintData("-",1);
		}
		lcdGotoXY(10,2);
		print_dbm(power_db*10.0);
		lcdPrintData(lcd_buf,strlen(lcd_buf));

		//------------------------------------------
		// Power indication, PEP
		lcdGotoXY(10,3);					// Clear junk in line, if any
		lcdPrintData(" ",1);
		lcdGotoXY(17,3);
		lcdPrintData("  P",3);
		lcdGotoXY(10,3);
		print_dbm(power_db_pep*10.0);
		lcdPrintData(lcd_buf,strlen(lcd_buf));
	}
	
	else
		screensaver();						// Screensaver display
}



//
//-----------------------------------------------------------------------------
//			Display Forward and Reflected Power, SWR and R + jX
//			(input power is in mW)
//-----------------------------------------------------------------------------
//
void lcd_display_mixed()
{
	static uint8_t mode_timer = 0;			// Used to time the Display Mode intro

	#if PHASE_DETECTOR			// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
	//------------------------------------------
	// Display mode intro for a time
	if(Status & MODE_DISPLAY)
	{
		if(Status & MODE_CHANGE)
		{
			Status &= ~MODE_CHANGE;			// Clear display change mode
			mode_timer = 0;					// New mode, reset timer
			lcd_display_mode_intro("Mode:"," ","Fwd & Reflected PWR","Return Loss & R + jX");
		}
	
		mode_timer++;
		if (mode_timer >= MODE_INTRO_TIME)	// MODE_INTRO_TIME in tenths of seconds
		{
			mode_timer = 0;
			Status &= ~MODE_DISPLAY;		// Clear display change mode
			Status |= IDLE_REFRESH;			// Force screensaver reprint upon exit, if screensaver mode
		}
	}

	//----------------------------------------------
	// Display Forward (not incident) Power if level is above useful threshold
	//
	// If input power above a minimum relevant value is detected (see PM.h)
	else if (fwd_power_mw > R.idle_disp_thresh)
	{
		//------------------------------------------
		// Forward Power indication
		lcdGotoXY(10,0);					// Clear junk in line, if any
		lcdPrintData("   ",3);
		lcdGotoXY(0,0);
		lcdPrintData("Fwd ",4);
		print_p_mw(fwd_power_mw);
		lcdPrintData(lcd_buf,strlen(lcd_buf));
		lcdGotoXY(12,0);
		print_dbm(fwd_power_db*10.0);
		lcdPrintData(lcd_buf,strlen(lcd_buf));
	
		//------------------------------------------
		// Reflected Power indication
		lcdGotoXY(10,1);					// Clear junk in line, if any
		lcdPrintData("   ",3);
		lcdGotoXY(0,1);
		lcdPrintData("Rfl ",4);
		print_p_mw(ref_power_mw);
		lcdPrintData(lcd_buf,strlen(lcd_buf));
		lcdGotoXY(12,1);
		print_dbm(ref_power_db*10.0);
		lcdPrintData(lcd_buf,strlen(lcd_buf));

		// Print Phase info in degrees, not radians
		//lcdGotoXY(17,2);					// Clear junk in line, if any
		//lcdPrintData("deg",3);
		//lcdGotoXY(12,2);
		//sprintf(lcd_buf," %4d",(int16_t) (phase*360.0/6.283));
		//lcdPrintData(lcd_buf,strlen(lcd_buf));
		lcdGotoXY(0,2);
		sprintf(lcd_buf,"Phs%4ddeg  ",(int16_t) (phase*360.0/6.283));
		lcdPrintData(lcd_buf,strlen(lcd_buf));
	
		//------------------------------------------
		// Return Loss indication
		lcdGotoXY(10,2);
		int16_t retloss = (fwd_power_db-ref_power_db)*10;
		sprintf(lcd_buf," Rl %2u.%1udB",retloss/10,retloss%10);
		lcdPrintData(lcd_buf,strlen(lcd_buf));

		//------------------------------------------
		// Impedance as R and jX
		// Only show if input power is reaching a useful level
		if (power_mw > R.idle_disp_thresh)
		{
			//lcdGotoXY(7,3);					// Clear junk in line, if any
			//lcdPrintData("   ",3);
			lcdGotoXY(17,3);				// Clear junk in line, if any
			lcdPrintData("   ",3);
			lcdGotoXY(0,3);
			sprintf(lcd_buf,"R %4.1f    ", imp_R*50);
			lcdPrintData(lcd_buf,strlen(lcd_buf));
			lcdGotoXY(10,3);
			sprintf(lcd_buf," jX %4.1f", imp_jX*50);
			lcdPrintData(lcd_buf,strlen(lcd_buf));
		}
	}
	#else				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
	uint32_t scale;								// Progress bar scale

	// Used to scale/fit progress bar inputs into 16 bit variables, when needed
	uint16_t fwd_bar_power=0, rev_bar_power=0, bar_scale=0;
	
	scale = scale_BAR((uint32_t) fwd_power_mw);	// Determine scale setting, also used to
												// determine if level above useful threshold
	
	//------------------------------------------
	// Display mode intro for a time
	if(Status & MODE_DISPLAY)
	{
		if(Status & MODE_CHANGE)
		{
			Status &= ~MODE_CHANGE;			// Clear display change mode
			mode_timer = 0;					// New mode, reset timer
			lcd_display_mode_intro("Mode:","Forward and","     Reflected Power","Bargraph Meter");
		}

		mode_timer++;
		if (mode_timer >= MODE_INTRO_TIME)	// MODE_INTRO_TIME in tenths of seconds
		{
			mode_timer = 0;
			Status &= ~MODE_DISPLAY;		// Clear display change mode
			Status |= IDLE_REFRESH;			// Force screensaver reprint upon exit, if screensaver mode
		}
	}
	
	//----------------------------------------------
	// Display Power if level is above useful threshold
	//
	// If input power above a minimum relevant value is detected (see PM.h)
	// or if scale indicates a value higher than 10mW
	// (= significant power within last 10 seconds)
	else if ((power_mw > R.idle_disp_thresh) || (scale > 10))
	{
		//------------------------------------------
		// Power indication and Bar graph
		
		//------------------------------------------
		// Scale variables to fit 16bit input to lcdProgressBarPeak()
		scale_PowerBarInpValues(scale, fwd_power_mw, ref_power_mw, &bar_scale, &fwd_bar_power, &rev_bar_power);

		// Forward Power Bargraph
		lcdGotoXY(0,0);
		// As the higher power is always kept in the fwd variable, we need to make sure that the correct
		// variable is selected for display, based on direction.
		if (!Reverse) lcdProgressBarPeak(fwd_bar_power, 0 /* no PEP */, bar_scale, 14);
		else lcdProgressBarPeak(rev_bar_power, 0 /* no PEP */, bar_scale, 14);

		//------------------------------------------
		// Wattage Printout
		print_p_mw(fwd_power_mw);
		lcdPrintData(lcd_buf,strlen(lcd_buf));

		//------------------------------------------
		// Reverse Power Bargraph
		lcdGotoXY(0,1);
		// As the higher power is always kept in the fwd variable, we need to make sure that the correct
		// variable is selected for display, based on direction.
		if (!Reverse) lcdProgressBarPeak(rev_bar_power, 0 /* no PEP */, bar_scale, 14);
		else lcdProgressBarPeak(fwd_bar_power, 0 /* no PEP */, bar_scale, 14);
		//------------------------------------------
		// Wattage Printout
		print_p_mw(ref_power_mw);
		lcdPrintData(lcd_buf,strlen(lcd_buf));

		//------------------------------------------
		// SWR Printout
		lcdGotoXY(7,2);						// Clear junk in line, if any
		lcdPrintData("   ",3);
		lcdGotoXY(0,2);
		lcdPrintData("SWR ",4);
		print_swr();						// and print the "SWR value"
		lcdPrintData(lcd_buf,strlen(lcd_buf));
	
		//------------------------------------------
		// Scale Indication
		lcdGotoXY(8,3);			// Ensure last couple of chars in line are clear
		lcdPrintData("  ",2);
		lcdGotoXY(0,3);
		lcdPrintData("Scale",5);
		print_p_reduced((double) scale);	// Scale Printout
		lcdPrintData(lcd_buf,strlen(lcd_buf));

		//------------------------------------------
		// Forward Power Indication, dBm
		lcdGotoXY(10,2);					// Clear junk in line, if any
		lcdPrintData(" ",1);
		lcdGotoXY(15,2);					// Clear junk in line, if any
		lcdPrintData("     ",5);
		/*
		if (Reverse)						// If reverse power, then indicate
		{
			lcdGotoXY(19,2);
			lcdPrintData("-",1);
		}
		*/
		lcdGotoXY(10,2);
		print_dbm(fwd_power_db*10.0);
		lcdPrintData(lcd_buf,strlen(lcd_buf));

		//------------------------------------------
		// Reflected Power Indication, dBm
		lcdGotoXY(10,3);					// Clear junk in line, if any
		lcdPrintData(" ",1);
		lcdGotoXY(15,3);					// Clear junk in line, if any
		lcdPrintData("     ",5);
		/*
		if (Reverse)						// If reverse power, then indicate
		{
			lcdGotoXY(19,3);
			lcdPrintData("-",1);
		}
		*/
		lcdGotoXY(10,3);
		print_dbm(ref_power_db*10.0);
		lcdPrintData(lcd_buf,strlen(lcd_buf));
	}	
	#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
	else
		screensaver();						// Screensaver display
}


//
//-----------------------------------------------------------------------------
//			Display Config and measured input voltages etc...
//-----------------------------------------------------------------------------
//
void lcd_display_debug(void)
{
	double 	output_voltage;
	uint16_t power, power_sub;
	
	lcdClear();

	//lcd_gotoxy(0,0);
	//lcd_puts_P("Dbg:");

	#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code			
	//------------------------------------------
	//------------------------------------------
	// AD8307 voltage indication, in Raw A/D value
	lcdGotoXY(0,0);
	//output_voltage = ad8307_adV * 5.0 / 4096;
	//power_sub = output_voltage * 1000;
	//power = power_sub / 1000;
	//power_sub = power_sub % 1000;
	//sprintf(lcd_buf,"%u.%03uV ", power, power_sub);
	sprintf(lcd_buf,"%04u ", ad8307_adV);
	lcdPrintData(lcd_buf,strlen(lcd_buf));

	//------------------------------------------
	// AD8307 current indication, in Raw A/D value
	lcdGotoXY(0,1);
	//output_voltage = ad8307_adI * 5.0 / 4096;
	//power_sub = output_voltage * 1000;
	//power = power_sub / 1000;
	//power_sub = power_sub % 1000;
	//sprintf(lcd_buf,"%u.%03uV  ", power, power_sub);
	sprintf(lcd_buf,"%04u ", ad8307_adI);
	lcdPrintData(lcd_buf,strlen(lcd_buf));
	
	// Calibrate 1
	lcdGotoXY(6,0);
	sprintf (lcd_buf,"%04d ",R.cal_AD[0].db10m);
	lcdPrintData(lcd_buf,strlen(lcd_buf));
	sprintf (lcd_buf,"%04u %04u",R.cal_AD[0].V,R.cal_AD[0].I);
	lcdPrintData(lcd_buf,strlen(lcd_buf));
	//------------------------------------------
	// Calibrate 2
	lcdGotoXY(6,1);
	sprintf (lcd_buf,"%04d ",R.cal_AD[1].db10m);
	lcdPrintData(lcd_buf,strlen(lcd_buf));
	sprintf (lcd_buf,"%04u %04u",R.cal_AD[1].V,R.cal_AD[1].I);
	lcdPrintData(lcd_buf,strlen(lcd_buf));
	
	//------------------------------------------
	// MCK12140 pos voltage indication
	lcdGotoXY(0,2);
	output_voltage = mck12140pos * 5000.0 / 4096.0;
	power_sub = output_voltage;
	power = power_sub / 1000;
	power_sub = power_sub % 1000;
	sprintf(lcd_buf,"%04u, %u.%03uV ", mck12140pos, power, power_sub);
	lcdPrintData(lcd_buf,strlen(lcd_buf));
	//------------------------------------------
	// MCK12140 neg voltage indication
	lcdGotoXY(0,3);
	output_voltage = mck12140neg * 5000.0 / 4096.0;
	power_sub = output_voltage;
	power = power_sub / 1000;
	power_sub = power_sub % 1000;
	sprintf(lcd_buf,"%04u, %u.%03uV ", mck12140neg, power, power_sub);
	lcdPrintData(lcd_buf,strlen(lcd_buf));
	//------------------------------------------
	// MCK12140 phase indication
	lcdGotoXY(15,2);
	lcdPrintData("Phase",5);
	lcdGotoXY(13,3);
	sprintf(lcd_buf,"%4ddeg",(int16_t) (phase*360.0/6.283));
	lcdPrintData(lcd_buf,strlen(lcd_buf));
	
	#else	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
	//------------------------------------------
	// AD8307 forward indication
	lcdGotoXY(0,0);
	output_voltage = ad8307_adF * 2.56 / 4096;
	power_sub = output_voltage * 1000;
	power = power_sub / 1000;
	power_sub = power_sub % 1000;
	sprintf(lcd_buf,"%4u %u.%03uV ", ad8307_adF, power, power_sub);
	lcdPrintData(lcd_buf,strlen(lcd_buf));
	//------------------------------------------
	// AD8307 reverse indication
	lcdGotoXY(0,1);
	output_voltage = ad8307_adR * 2.56 / 4096;
		power_sub = output_voltage * 1000;
	power = power_sub / 1000;
	power_sub = power_sub % 1000;
	sprintf(lcd_buf,"%4u %u.%03uV ", ad8307_adR, power, power_sub);
	lcdPrintData(lcd_buf,strlen(lcd_buf));
	//------------------------------------------
	// Calibrate 1
	lcdGotoXY(0,2);
	sprintf (lcd_buf,"%4d ",R.cal_AD[0].db10m);
	lcdPrintData(lcd_buf,strlen(lcd_buf));
	sprintf (lcd_buf,"%4u,%4u",R.cal_AD[0].Fwd,R.cal_AD[0].Rev);
	lcdPrintData(lcd_buf,strlen(lcd_buf));
	//------------------------------------------
	// Calibrate 2
	lcdGotoXY(0,3);
	sprintf (lcd_buf,"%4d ",R.cal_AD[1].db10m);
	lcdPrintData(lcd_buf,strlen(lcd_buf));
	sprintf (lcd_buf,"%4u,%4u",R.cal_AD[1].Fwd,R.cal_AD[1].Rev);
	lcdPrintData(lcd_buf,strlen(lcd_buf));
	#endif	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection
}
