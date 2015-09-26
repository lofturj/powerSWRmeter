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


#include "PM.h"
#include "Lufa_SPI_TWI\twi.h"
#include "analog.h"

#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
const uint8_t aref = ADC_REF_POWER;				// 5V reference is connected
#else				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
const uint8_t aref = ADC_REF_INTERNAL;			// Internal 2.56V reference used
#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection

#define AD7991_0	0x50		// I2C Address of AD7991-0
#define AD7991_1	0x52		// I2C Address of AD7991-1

uint8_t		ad7991_addr;		// Address of AD7991 I2C connected A/D, 0 if none detected

//
//-----------------------------------------------------------------------------------------
// 			Read Microcontroller builtin ADC Mux
//-----------------------------------------------------------------------------------------
//
int16_t adc_Read(uint8_t mux)
{
	uint8_t low;

	ADCSRA = (1<<ADEN) | ADC_PRESCALER;				// enable ADC
	ADCSRB = (1<<ADHSM) | (mux & 0x20);				// high speed mode
	ADMUX = aref | (mux & 0x1F);					// configure mux input
	ADCSRA = (1<<ADEN) | ADC_PRESCALER | (1<<ADSC); // start the conversion
	while (ADCSRA & (1<<ADSC)) ;                    // wait for result
	low = ADCL;										// must read LSB first
	//return (ADCH << 8) | low;						// must read MSB only once!
	// Return as an 12 bit variable relative to 2.56V, to be on relatively equal level basis
	// with output from AD7991 12bit A/D which is referenced to 2.6V 
	// ** If Phase&Power Meter, then both A/D options are referenced to 5.0V
	return (ADCH << 10) | (low << 2);				// must read MSB only once!
}


//
//-----------------------------------------------------------------------------------------
// 			Scan I2C bus for AD7991
//
// Return which type detected (AD7991-0 or AD7991-1) or 0 if nothing found
//-----------------------------------------------------------------------------------------
//
uint8_t I2C_Init(void)
{
	uint8_t found;					// Returns status of I2C bus scan
	
	// TWI Init at 400kb/s
	TWI_Init(TWI_BIT_PRESCALE_1, TWI_BITLENGTH_FROM_FREQ(1,400000));
	
	// Scan for AD7991-0 or AD7991-1
	ad7991_addr = AD7991_1;			// We start testing for an AD7991-1
	found = 2;						//  Assume it will be found

	if (TWI_StartTransmission(ad7991_addr | TWI_ADDRESS_WRITE, 10) != TWI_ERROR_NoError)
	{
		ad7991_addr = AD7991_0;		// AD7991-0 was not detected
		found = 1;					// We may have an AD7991-0	
	}
    TWI_StopTransmission();

	if (TWI_StartTransmission(ad7991_addr | TWI_ADDRESS_WRITE, 10) != TWI_ERROR_NoError)
	{
		ad7991_addr = 0;			// AD7991-1 was not detected
		found = 0;					// No I2C capable device was detected
	}
    TWI_StopTransmission();

	#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
	// Default settings for AD8991 is to read all four ADCs consecutively and use Vdd for reference
	// therefore, nothing to set up...
	//
	#else				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
	// If Power & SWR Code and AD7991 12 bit ADC, the program it to use a 2.6V reference connected
	// to ADC channel 4 and only read ADC channels 1 and 2 consecutively.
	//
	if (found)			// If AD7991 ADC was found, then configure it...
	{
		uint8_t writePacket = 0x38;	// Set ADCs 1 and 2 for consecutive reads and REF_SEL = external
		TWI_WritePacket(ad7991_addr,10,&writePacket,0,&writePacket,1);		
	}
	#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection

	return found;
}


#if PHASE_DETECTOR	// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Power & Phase Detector Code
//-----------------------------------------------------------------------------------------
// 							Poll the AD7991 4 x ADC chip
//									or alternately
//				use built-in 10 bit A/D converters if I2C device not present
//-----------------------------------------------------------------------------------------
//
// This function reads all four A/D inputs and makes the data available in
// global variables declared in Mobo.c
void adc_poll(void)
{
	uint8_t readPacket[8];
	uint16_t adc_in[4];
	uint8_t i;
	
	if (ad7991_addr)	// If AD7991 was detected during init, then:
	{
		TWI_ReadPacket(ad7991_addr,10,readPacket,0,readPacket,sizeof(readPacket));
	
		// The output of the 12bit ADCs is contained in four consecutive byte pairs
		// read from the AD7991.  In theory, any ADC could be the first one read.
		// Each of the AD7991 four builtin ADCs has an address identifier (0 to 3)
		// in the uppermost 4 bits of the first byte.  The lowermost 4 bits of the first
		// byte are bits 8-12 of the A/D output.
		//
		for (i=0; i<4;i++)
		{
			adc_in[(readPacket[i*2] >> 4) & 0x03] = (readPacket[i*2] & 0x0f) * 0x100 + readPacket[i*2+1];
		}
		ad8307_adV = adc_in[0];				// Measure voltage from AD8307
		ad8307_adI = adc_in[1];				// Measure current from AD8307
		mck12140pos = adc_in[2];			// Measure voltage from MCK12140 positive direction pin
		mck12140neg = adc_in[3];			// Measure voltage from MCK12140 negative direction pin
	}

	else		// If no I2C, then revert to builtin 10 bit A/D converters
	{
		ad8307_adV = adc_Read(0);			// Measure voltage from MCK12140 positive direction pin
		ad8307_adI = adc_Read(1);			// Measure voltage from MCK12140 negative direction pin
		mck12140pos = adc_Read(2);			// Measure voltage from AD8307
		mck12140neg = adc_Read(3);			// Measure current from AD8307
	}
}
#else				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Simple Power & SWR Code
//-----------------------------------------------------------------------------------------
// 							Poll the AD7991 2 x ADC chip
//									or alternately
//				use built-in 10 bit A/D converters if I2C device not present
//-----------------------------------------------------------------------------------------
//
// This function reads all four A/D inputs and makes the data available in
// global variables declared in Mobo.c
void adc_poll(void)
{
	uint8_t readPacket[4];
	uint16_t adc_in[4];
	
	if (ad7991_addr)	// If AD7991 was detected during init, then:
	{
		TWI_ReadPacket(ad7991_addr,10,readPacket,0,readPacket,sizeof(readPacket));
		
		// The output of the 12bit ADCs is contained in two consecutive byte pairs
		// read from the AD7991.  In theory, the second could be read before the first.
		// Each of the AD7991 four builtin ADCs has an address identifier (0 to 3)
		// in the uppermost 4 bits of the first byte.  The lowermost 4 bits of the first
		// byte are bits 8-12 of the A/D output.
		// In this routine we only read the two first ADCs, as set up in the I2C_Init()
		//
		adc_in[(readPacket[0] >> 4) & 0x03] = (readPacket[0] & 0x0f) * 0x100 + readPacket[1];
		adc_in[(readPacket[2] >> 4) & 0x03] = (readPacket[2] & 0x0f) * 0x100 + readPacket[3];
		ad8307_adF = adc_in[0];
		ad8307_adR = adc_in[1];
	}

	else		// If no I2C, then revert to builtin 10 bit A/D converters
	{
		ad8307_adF = adc_Read(0);			// Measure forward from AD8307
		ad8307_adR = adc_Read(1);			// Measure reverse from AD8307
	}
}
#endif				// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> End Power&Phase vs Power&SWR code selection