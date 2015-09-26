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

//
//-----------------------------------------------------------------------------------------
//
//	Power and Impedance Meter Functions
//
//	Measure Voltage, Current and Phase of input signals 
//	from transformers in a V and I configuration
//	to determine Power, Impedance and SWR 
//
//-----------------------------------------------------------------------------------------
//


#include "PM.h"

#if PHASE_DETECTOR							// >>>>>>>>>>>>>>> Power & Phase Detector Code
//
//-----------------------------------------------------------------------------------------
// 			Determine Phase between input signals, 
//			using the MCK12140+MC100ELT23 Phase-Frequency Detector.
//			Output is in radians, +/-PI.
//
//			The Phase Detector is flat bottomed at 0 degrees. phase differences under
//			+/- 9 degrees around the 0 point are not measurable.
//			This problem is even worse at the 360 degree point.  There are significant
//			errors above 270 degrees, overestimating the phase.
//
//			However, the MCK12140 Phase-Frequency Detector has excellent linearity
//			in the region 180 +/- 90 degrees.  Hence, use with V and I  180 degrees out of phase.
//-----------------------------------------------------------------------------------------
void imp_determine_phase(void)
{
	// Voltage and Current are 180 degrees out of phase when terminated into 50 ohm.  In other words,
	// everything has been transposed by 180 degrees to get inaccuracies near 0 and 360 degrees
	// out of the way.
	//
	// A set of calibrated MCK12140+MC100ELT23 Phase detector output voltages at 0 and +/-90 degrees
	// are determined (defined in PM.h) by doing a measurement campaign, using a signal generator for
	// a number of phase measurements evenly spaced between 1 and 30 MHz.  The "Debug Display" is used
	// to read the phase voltages.  As the phase detector has two possible solutions for every situation,
	// both solutions need to be established during the measurement campaign.  Measurements are done 
	// using a 50 ohm terminator at the output to establish voltages at 0 degrees. Then a fixed length of
	// Coax is inserted/added; at the V input first, and then the I input, to generate frequency dependent
	// phase differences.  I used 1.85m of Coax, which gives a 90 degree phase difference (1/4 wavelength)
	// at 26.5 MHz.  The phase voltage readouts at each frequency are extrapolated towards the 90 deg
	// voltage for each measured frequency, and averages are determined using an Excel tool - see writeup
	// (ToDo)
	// New calibration values can be input using the USB Commands: 
	//	$phasesetu A B C	and
	//	$phasesetd D E F    where
	//  A B C and D E F are values similar to:
	//			0.9054 1.7737 2.6520 for the U pos90deg, zerodeg and neg90deg; and
	// 			2.6828 1.7726 0.9127 for the D pos90deg, zerodeg and neg90deg, consecutively.
	// Current calibration values can be retrieved using the USB commands $phasegetu and $phasegetd
	//
	// It may appear a bit inappropriate to use double precision floating point arithmetic with an 8 bit
	// microcontroller, but I was lazy and it works without any apparent penalty or problems :)
	
	// Positive (U output) and Negative (D output) phase voltages
	double v_U, v_D;
	
	// Convert AD input (12 bit precision, even if 10bit AD was used) to actual voltage reading
	v_U = (double) mck12140pos * 5.0/4096;
	v_D = (double) mck12140neg * 5.0/4096;
	
	// Determine which signal contains the phase info and determine phase angle
	if (v_D > v_U)
	{
		// 0 to 180 degrees
		if (v_D > R.D.zerodeg)
		{
			phase = 3.14159/2.0 * (v_D - R.D.zerodeg)/(R.D.pos90deg - R.D.zerodeg);
		}
		// 0 to -180 degrees
		else
		{
			phase = -3.14159/2.0 * (R.D.zerodeg - v_D)/(R.D.zerodeg - R.D.neg90deg);
		}
	}
	else
	{
		// 0 to -180 degrees
		if (v_U > R.U.zerodeg)
		{
			phase = -3.14159/2.0 * (v_U - R.U.zerodeg)/(R.U.neg90deg - R.U.zerodeg);
		}
		// 0 to 180 degrees
		else
		{
			phase = 3.14159/2.0 * (R.U.zerodeg - v_U)/(R.U.zerodeg - R.U.pos90deg);
		}
	}
}


//
//-----------------------------------------------------------------------------------------
// 			Convert Voltage and Current into Power
//-----------------------------------------------------------------------------------------
//
void imp_determine_dBm(void)
{
	double 	delta_db;

	int16_t	delta_V, delta_I;
	double	delta_Vdb, delta_Idb;
	// Calculate the slope gradient between the two calibration points:
	//
	// (dB_Cal1 - dB_Cal2)/(V_Cal1 - V_Cal2) = slope_gradient
	//
	delta_db = (double)((R.cal_AD[1].db10m - R.cal_AD[0].db10m)/10.0);
	delta_V = R.cal_AD[1].V - R.cal_AD[0].V;
	delta_Vdb = delta_db/delta_V;
	delta_I = R.cal_AD[1].I - R.cal_AD[0].I;
	delta_Idb = delta_db/delta_I;
	//
	// measured dB values are: (V - V_Cal1) * slope_gradient + dB_Cal1
	//
	ad8307_VdBm = (ad8307_adV - R.cal_AD[0].V) * delta_Vdb + R.cal_AD[0].db10m/10.0;
	ad8307_IdBm = (ad8307_adI - R.cal_AD[0].I) * delta_Idb + R.cal_AD[0].db10m/10.0;
}

//
//-----------------------------------------------------------------------------------------
// 			Calculate SWR if we have sufficient input power
//-----------------------------------------------------------------------------------------
//
void imp_calc_SWR(void)
{
	// Only calculate SWR if enough power to drive the Phase Detector
	// 1mW is enough at 30 MHz, set at 5mW for good margin up to 50 MHz
	if ((power_mw > MIN_PWR_FOR_SWR_CALC) || (power_mw < -MIN_PWR_FOR_SWR_CALC))
	{
		// Calculate SWR
		swr = (1 + Gamma) / (1 - Gamma);

		// prepare SWR bargraph value as a logarithmic integer value between 0 and 1000
		if (swr < 10.0)
		swr_bar = 1000.0 * log10(swr);
		else
		swr_bar = 1000;

		// Check for high SWR and set alarm flag if trigger value is exceeded
		// If trigger is 40 (4:1), then Alarm function is Off
		if ((R.SWR_alarm_trig != 40) && ((swr*10) >= R.SWR_alarm_trig) && (power_mw > R.SWR_alarm_pwr_thresh))
		{
			if (Status & SWR_FLAG) // This is the second time in a row, raise the SWR ALARM
			{
				EXTLED_PORT |= EXT_R_LED;
			}
			Status |= SWR_FLAG;		// Set SWR flag - may be a one shot fluke
		}
		else if (Status & SWR_FLAG)	// Clear SWR Flag if High SWR condition does not exist
		{
			Status &= ~SWR_FLAG;
		}
	}

	// If some power present, but not enough for an accurate SWR reading, then use
	// recent measured value
	else if ((power_mw > MIN_PWR_FOR_SWR_SHOW) || (power_mw < -MIN_PWR_FOR_SWR_SHOW))
	{
		// Do nothing, keep last calculated value while inadequate power
	}
	else
	{
		// No power present, set to 1.0
		swr = 1.0;
		swr_bar = 0;
	}
}



//
//-----------------------------------------------------------------------------------------
// 			Calculate all kinds of power
//-----------------------------------------------------------------------------------------
//
void imp_calc_Power(void)
{
	#define	PEP_BUFFER	1000				// PEP Buffer size, can hold up to 5 second PEP
	#define	BUF_SHORT	20					// Buffer size for 100ms Peak
	#define	AVG_BUF		200					// Buffer size for 1s Average measurement

	double v_inst;							// dB Voltage (normalized into 50 ohms)
	double i_inst;							// dB Current (normalized into 50 ohms)
	double imp_R_unsigned;					// Handle negative values of R (power in opposite direction)

	// For measurement of peak and average power
	static int16_t db_buff[PEP_BUFFER];		// dB voltage information in a one second window
	static int16_t db_buff_short[BUF_SHORT];// dB voltage information in a 100 ms window
	static double  p_avg_buf[AVG_BUF];			// all instantaneous power measurements in 1s
	static double  p_plus;					// all power measurements within a 1s window added together
	static uint16_t a=0;					// 1s ring buffer counter
	static uint8_t b=0;						// 100ms ring buffer counter
	static uint8_t c=0;						// average ring buffer counter
	int16_t max=-32767, pk=-32767;			// Keep track of Max (1s) and Peak (100ms) dB voltage

	// Calculate voltage and current (normalized) in absolute values
	v_inst = pow(10,ad8307_VdBm/20.0);
	i_inst = pow(10,ad8307_IdBm/20.0);

	// Calculate R and jX (normalized, need to multiply by 50 for actual value)
	imp_R  = v_inst/i_inst * cos(phase);
	imp_jX = v_inst/i_inst * sin(phase);
	
	// Take care of power in opposite direction
	if (imp_R < 0)
	{
		imp_R_unsigned = -1 * imp_R;
		Reverse = TRUE;
	}
	else
	{
		imp_R_unsigned = imp_R;
		Reverse = FALSE;
	}
	
	// Determine incident Power
	power_mw = SQR(i_inst) * imp_R_unsigned;
	power_db = 10 * log10(power_mw);
	
	// Determine Gamma
	Gamma = sqrt( ( SQR(imp_R_unsigned-1)+SQR(imp_jX) )
								/
				( SQR(imp_R_unsigned+1)+SQR(imp_jX) ) );
	
	// Reflected Power
	ref_power_mw = SQR(Gamma) * power_mw;
	ref_power_db = 10 * log10(ref_power_mw);

	// Forward Power
	fwd_power_mw = power_mw + ref_power_mw;
	fwd_power_db = 10 * log10(fwd_power_mw);

	// Find PEP and 100ms peak
	// Multiply dB value by 100 to make suitable for an integer value
	// Store dB value in two ring buffers
	db_buff[a] = db_buff_short[b] = 100 * power_db;
	// Advance 1s and 100ms ring buffer counters
	a++, b++;
	if (a >= R.PEP_period) a = 0;
	if (b == BUF_SHORT) b = 0;

	// Retrieve Max Value within a 1 second sliding window
	for (uint16_t x = 0; x < R.PEP_period; x++)
	{
		if (max < db_buff[x]) max = db_buff[x];
	}

	// Find Peak value within a 100ms sliding window
	for (uint8_t x = 0; x < BUF_SHORT; x++)
	{
		if (pk < db_buff_short[x]) pk = db_buff_short[x];
	}

	// PEP (1 second)
	power_db_pep = max / 100.0;
	power_mw_pep = pow(10,power_db_pep/10.0);

	// Peak (100 milliseconds)
	power_db_pk = pk / 100.0;
	power_mw_pk = pow(10,power_db_pk/10.0);

	// Average power (1 second), milliwatts and dBm
	p_avg_buf[c] = power_mw;				// Add the newest value onto ring buffer
	p_plus = p_plus + power_mw;				// Add latest value to the total sum of all measurements in 1s
	if (c < AVG_BUF-1)						// and subtract the oldest value in the ring buffer from the total sum
	p_plus = p_plus - p_avg_buf[c+1];
	else
	p_plus = p_plus - p_avg_buf[0];
	c++;									// Rotate window by advancing ring buffer counter
	if (c == AVG_BUF) c = 0;
	power_mw_avg = p_plus / (AVG_BUF);		// And finally, find the average
	power_db_avg = 10 * log10(power_mw_avg);

	//// Modulation index in a 1s window
	//double v1, v2;
	//v1 = sqrt(ABS(power_mw_pep));
	//v2 = sqrt(ABS(power_mw_avg));
	//modulation_index = (v1-v2) / v2;
	
	imp_calc_SWR();
}

#endif					// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection