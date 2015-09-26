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
//	Power and SWR Meter Functions
//
//	Measure Forward and Reverse Power from a Tandem Match Coupler
//	to determine Power and SWR 
//
//-----------------------------------------------------------------------------------------
//


#include "PM.h"

#if !PHASE_DETECTOR							// >>>>>>>>>>>>>>> Simple Power and SWR Meter Code
//
//-----------------------------------------------------------------------------------------
// 			Convert Voltage Reading into Power
//-----------------------------------------------------------------------------------------
//
void pswr_determine_dBm(void)
{
	double 	delta_db;

	int16_t	delta_F, delta_R;
	double	delta_Fdb, delta_Rdb;
	double transfer;

	// Calculate the slope gradient between the two calibration points:
	//
	// (dB_Cal1 - dB_Cal2)/(V_Cal1 - V_Cal2) = slope_gradient
	//
	delta_db = (double)((R.cal_AD[1].db10m - R.cal_AD[0].db10m)/10.0);
	delta_F = R.cal_AD[1].Fwd - R.cal_AD[0].Fwd;
	delta_Fdb = delta_db/delta_F;
	delta_R = R.cal_AD[1].Rev - R.cal_AD[0].Rev;
	delta_Rdb = delta_db/delta_R;
	//
	// measured dB values are: (V - V_Cal1) * slope_gradient + dB_Cal1
	ad8307_FdBm = (ad8307_adF - R.cal_AD[0].Fwd) * delta_Fdb + R.cal_AD[0].db10m/10.0;
	ad8307_RdBm = (ad8307_adR - R.cal_AD[0].Rev) * delta_Rdb + R.cal_AD[0].db10m/10.0;

	// Test for direction of power - Always designate the higher power as "forward"
	// while setting the "Reverse" flag on reverse condition.
	if (ad8307_FdBm > ad8307_RdBm)	// Forward direction
	{
		Reverse = FALSE;
	}
	else							// Reverse direction
	{
		transfer = ad8307_RdBm;
		ad8307_RdBm = ad8307_FdBm;
		ad8307_FdBm = transfer;
		Reverse = TRUE;
	}
}

//
//-----------------------------------------------------------------------------------------
// 			Calculate SWR if we have sufficient input power
//-----------------------------------------------------------------------------------------
//
void pswr_calc_SWR(double v_fwd, double v_rev)
{
	// Only calculate SWR if meaningful power

	if ((power_mw > MIN_PWR_FOR_SWR_CALC) || (power_mw < -MIN_PWR_FOR_SWR_CALC))
	{
		// Calculate SWR
		swr = (1+(v_rev/v_fwd))/(1-(v_rev/v_fwd));

		// prepare SWR bargraph value as a logarithmic integer value between 0 and 1000
		if (swr < 10.0)
		swr_bar = 1000.0 * log10(swr);
		else
		swr_bar = 1000;		// Show as maxed out above SWR 10:1

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
		// Do nothing, in other words, swr and swr_bar remain the same
	}
	else
	{
		// No power present, set SWR to 1.0
		swr = 1.0;
		swr_bar = 0;
	}
}


//
//-----------------------------------------------------------------------------------------
// 			Calculate all kinds of power
//-----------------------------------------------------------------------------------------
//
void pswr_calc_Power(void)
{
	#define	PEP_BUFFER	1000				// PEP Buffer size, can hold up to 5 second PEP
	#define	BUF_SHORT	20					// Buffer size for 100ms Peak
	#define	AVG_BUF		200					// Buffer size for 1s Average measurement

	double f_inst;							// Calculated Forward voltage
	double r_inst;							// Calculated Reverse voltage
	
	// For measurement of peak and average power
	static int16_t db_buff[PEP_BUFFER];		// dB voltage information in a one second window
	static int16_t db_buff_short[BUF_SHORT];// dB voltage information in a 100 ms window
	static double	p_avg_buf[AVG_BUF];		// all instantaneous power measurements in 1s
	static double	p_plus;					// all power measurements within a 1s window added together
	static uint16_t a=0;					// PEP ring buffer counter
	static uint8_t b=0;						// 100ms ring buffer counter
	static uint8_t c=0;						// 1s average ring buffer counter
	int16_t max=-32767, pk=-32767;			// Keep track of Max (1s) and Peak (100ms) dB voltage

	// Instantaneous forward voltage and power, milliwatts and dBm
	f_inst = pow(10,ad8307_FdBm/20.0);		// (We use voltage later on, for SWR calc)
	fwd_power_mw = SQR(f_inst);				// P_mw = (V*V) (current and resistance have already been factored in
	fwd_power_db = 10 * log10(fwd_power_mw);
	
	// Instantaneous reverse voltage and power
	r_inst = pow(10,(ad8307_RdBm)/20.0);
	ref_power_mw = SQR(r_inst);
	ref_power_db = 10 * log10(ref_power_mw);
	
	// Instantaneous Real Power Output
	power_mw = fwd_power_mw - ref_power_mw;
	power_db = 10 * log10(power_mw);

	// Find peaks and averages
	// Multiply by 100 to make suitable for integer value
	// Store dB value in two ring buffers
	db_buff[a] = db_buff_short[b] = 100 * power_db;
	// Advance PEP (1, 2.5 or 5s) and 100ms ringbuffer counters
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

	// PEP
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
	
	pswr_calc_SWR(f_inst, r_inst);
}

#endif					// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection