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

//#include 	<math.h>
#include	"PM.h"


//
//-----------------------------------------------------------------------------
//			Print SWR, returns string in lcd_buf
//-----------------------------------------------------------------------------
//
void print_swr(void)
{
	if (swr < 2.0)
		sprintf(lcd_buf," %1.02f",swr);
	else if (swr < 10.0)
		sprintf(lcd_buf,"  %1.01f",swr);
	else if (swr < 10000.0)
		sprintf(lcd_buf,"%5u",(uint16_t) swr);
	else
		sprintf(lcd_buf," 9999");
}


//
//-----------------------------------------------------------------------------
//			Print dBm, accepts 10x dBm input value, 
//			returns string in lcd_buf
//-----------------------------------------------------------------------------
//
void print_dbm(int16_t db10m)
{
	int16_t pwrdb_tenths = db10m;

	if (pwrdb_tenths < 0) pwrdb_tenths *= -1;
	int16_t pwrdb = pwrdb_tenths / 10;
	pwrdb_tenths = pwrdb_tenths % 10;

	if (db10m <= -100)
	{
		sprintf(lcd_buf,"-%2u.%1udBm",pwrdb,pwrdb_tenths);
	}
	else if (db10m < 0)
	{
		sprintf(lcd_buf," -%1u.%1udBm",pwrdb,pwrdb_tenths);
	}
	else sprintf(lcd_buf,"%3u.%1udBm",pwrdb,pwrdb_tenths);
}


//
//-----------------------------------------------------------------------------
//			Print Power, input value is in milliWatts, 
//			returns string in lcd_buf
//-----------------------------------------------------------------------------
//
void print_p_mw(double mw)
{
	uint32_t p_calc;
	uint16_t power_sub, power;

	if(mw >= 1000000.0)				// 1kW
	{
		p_calc = mw;
		power = p_calc / 1000;
		sprintf(lcd_buf," %4uW",power);
	}
	else if(mw >= 100000.0)			// 100W
	{
		p_calc = mw;
		power = p_calc / 1000;
		sprintf(lcd_buf,"  %3uW",power);
	}
	else if(mw >= 10000.0)			// 10W
	{
		p_calc = mw;
		power = p_calc / 1000;
		power_sub = (p_calc % 1000)/100;
		sprintf(lcd_buf," %2u.%01uW",power, power_sub);
	}
	else if(mw >= 1000.0)			// 1W
	{
		p_calc = mw;
		power = p_calc / 1000;
		power_sub = (p_calc % 1000)/10;
		sprintf(lcd_buf," %1u.%02uW",power, power_sub);
	}
	else if(mw >= 100.0)			// 100mW
	{
		sprintf(lcd_buf," %3umW",(uint16_t)mw);
	}
	else if(mw >= 10.0)				// 10mW
	{
		p_calc = mw * 10;
		power = p_calc / 10;
		power_sub = p_calc % 10;
		sprintf(lcd_buf,"%2u.%01umW",power, power_sub);
	}
	else if(mw >= 1.0)				// 1mW
	{
		p_calc = mw * 100;
		power = p_calc / 100;
		power_sub = p_calc % 100;
		sprintf(lcd_buf,"%1u.%02umW",power, power_sub);
	}
	else if(mw >= 0.1)				// 100uW
	{
		power = mw * 1000;
		sprintf(lcd_buf," %3uuW",power);
	}
	else if(mw >= 0.01)				// 10uW
	{
		p_calc = mw * 10000;
		power = p_calc / 10;
		power_sub = p_calc % 10;
		sprintf(lcd_buf,"%2u.%01uuW",power, power_sub);
	}
	else if(mw >= 0.001)			// 1 uW
	{
		p_calc = mw * 100000;
		power = p_calc / 100;
		power_sub = p_calc % 100;
		sprintf(lcd_buf,"%1u.%02uuW",power, power_sub);
	}
	else if(mw >= 0.0001)			// 100nW
	{
		power = mw * 1000000;
		sprintf(lcd_buf," %3unW",power);
	}
	else if(mw >= 0.00001)			// 10nW
	{
		p_calc = mw * 10000000;
		power = p_calc / 10;
		power_sub = p_calc % 10;
		sprintf(lcd_buf,"%2u.%01unW",power, power_sub);
	}
	else if(mw >= 0.000001)			// 1nW
	{
		p_calc = mw * 100000000;
		power = p_calc / 100;
		power_sub = p_calc % 100;
		sprintf(lcd_buf,"%1u.%02unW",power, power_sub);
	}
	else if(mw >= 0.0000001)		// 100pW
	{
		power = mw * 1000000000;
		sprintf(lcd_buf," %3upW",power);
	}
	else if(mw >= 0.00000001)		// 10pW
	{
		p_calc = mw * 10000000000;
		power = p_calc / 10;
		power_sub = p_calc % 10;
		sprintf(lcd_buf,"%2u.%01upW",power, power_sub);
	}
	else if(mw >= 0.000000001)		// 1pW
	{
		p_calc = mw * 100000000000;
		power = p_calc / 100;
		power_sub = p_calc % 100;
		sprintf(lcd_buf,"%1u.%02upW",power, power_sub);
	}
	else if(mw >= 0.0000000001)		// 100fW
	{
		power = mw * 1000000000000;
		sprintf(lcd_buf," %3ufW",power);
	}
	else if(mw >= 0.00000000001)	// 10fW
	{
		p_calc = mw * 10000000000000;
		power = p_calc / 10;
		power_sub = p_calc % 10;
		sprintf(lcd_buf,"%2u.%01ufW",power, power_sub);
	}
	else							// 1fW
	{
		//p_calc = mw * 100000000000000;
		//power = p_calc / 100;
		//power_sub = p_calc % 100;
		//sprintf(lcd_buf,"%1u.%02ufW",power, power_sub);
		sprintf(lcd_buf," 0.00W");
	}
}


//
//-----------------------------------------------------------------------------
//			Print Power, reduced resolution, input value is in milliWatts, 
//			returns string in lcd_buf
//-----------------------------------------------------------------------------
//
void print_p_reduced(double mw)
{
	uint32_t p_calc;
	uint16_t power_sub, power;

	if(mw >= 1000000.0)				// 1kW
	{
		p_calc = mw;
		power = p_calc / 1000;
		sprintf(lcd_buf,"%4uW",power);
	}
	else if(mw >= 100000.0)			// 100W
	{
		p_calc = mw;
		power = p_calc / 1000;
		sprintf(lcd_buf,"%3uW",power);
	}
	else if(mw >= 10000.0)			// 10W
	{
		p_calc = mw;
		power = p_calc / 1000;
		sprintf(lcd_buf," %2uW",power);
	}
	else if(mw >= 1000.0)			// 1W
	{
		p_calc = mw;
		power = p_calc / 1000;
		power_sub = (p_calc % 1000)/10;
		sprintf(lcd_buf,"%1u.%01uW",power, power_sub);
	}
	else if(mw >= 100.0)			// 100mW
	{
		sprintf(lcd_buf,"%3umW",(uint16_t)mw);
	}
	else if(mw >= 10.0)				// 10mW
	{
		sprintf(lcd_buf," %2umW",(uint16_t)mw);
	}
	else 							// 1mW
	{
		sprintf(lcd_buf,"  %1umW",(uint16_t)mw);
	}
}
