//*********************************************************************************
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
//** Platform........: ATMEL ATmega or AT90 series of Microcontrollers
//**
//**                   Two very simple Rotary Encoder routines.
//**					 The interrupt driven routine uses only one interrupt, 
//**					 half resolution, however if required it would be simple 
//**					 to expand this to use interrupts on both pins.
//**					 The poll routine is simple and sweet, but depends on
//**					 very fast real time polling  
//**
//**
//**                   2013-09-20 Loftur Jonasson, TF3LJ / VE2LJX
//**
//*********************************************************************************


#include "PM.h"

#if ENCODER_INT_STYLE			// Interrupt driven Rotary Encoder

#include <avr/io.h>
#include <avr/interrupt.h>

int16_t	encOutput;								// Output From Encoder

//
// Init Encoder and Interrupt for use
//
void encoder_Init(void)
{
	//
	// enable interrupts & set inputs with pullup
	//
	//ENC_A_DDR &= ~(ENC_A_PIN | ENC_B_PIN);
	//ENC_A_PORT |= (ENC_A_PIN | ENC_B_PIN);
	// Better to do it this way to get more freedom of assigning the Phase B
	// pin to any port (costs a few more bytes:
	ENC_A_DDR &= ~ENC_A_PIN;	// Enable pin for input
	ENC_A_PORT |= ENC_A_PIN;	// Activate internal pullup resistor
	ENC_B_DDR &= ~ENC_B_PIN;
	ENC_B_PORT |= ENC_B_PIN;
	ENC_PUSHB_DDR |= ENC_PUSHB_PIN;
	ENC_PUSHB_PORT |= ENC_PUSHB_PIN;

	// Configure interrupts for any edge
	ENC_A_ICR |= (ENC_A_ISCX0 & ~ENC_A_ISCX1);

	// Enable interrupt vector
	ENC_A_IREG |= ENC_A_INT;

	sei();
}

//
// Shaft Encoder interrupt handler
// PCINT could be used as a substitute for INT.  Would need a revision of
// the Interrupt init above.
//
ISR(ENC_A_SIGNAL)
{
	static int8_t	increment = 0;				// This interim variable used to add up changes
	
	// encoder has generated a pulse
	// check the relative phase of the input channels
	// and update position accordingly
	if(((ENC_A_PORTIN & ENC_A_PIN) == 0) ^ ((ENC_B_PORTIN & ENC_B_PIN) == 0))
	{
			#if	ENCODER_DIR_REVERSE
			increment--;						// increment
			#else
			increment++;						// increment
			#endif
	}
	else
	{
			#if	ENCODER_DIR_REVERSE
			increment++;						// decrement
			#else
			increment--;						// decrement
			#endif
	}

	encOutput += increment/R.encoderRes;		// Adjustable Encoder output resolution

	if (encOutput != 0)							// We have an output
	{
		increment = 0;
		Status |= ENC_CHANGE;					// Encoder state was changed
	}
	
}
#endif//ENCODER_INT_STYLE						// Interrupt driven Rotary Encoder

//***********************************************************************************

#if ENCODER_SCAN_STYLE							// Rotary Encoder which scans the  the GPIO inputs

int16_t	encOutput;								// Output From Encoder
uint8_t old_pha = 0, old_phb = 0;				// Variables conaining the previous encoder states
//
// Init Encoder for use
//
void encoder_Init(void)
{
	//
	// Set inputs with pullup
	//
	ENC_A_DDR &= ~ENC_A_PIN;					// Enable pin for input
	ENC_A_PORT |= ENC_A_PIN;					// Activate internal pullup resistor
    ENC_B_DDR &= ~ENC_B_PIN;
	ENC_B_PORT |= ENC_B_PIN;

	_delay_ms(25);								// Wait for pin states to stabilize
 	if (ENC_A_PORTIN & ENC_A_PIN) old_pha = 1;	// Normalise startup phase values, based
	if (ENC_B_PORTIN & ENC_B_PIN) old_phb = 1;	// on initial state of the rotary encoder

	encoder_Scan();								// Scan once and Reset data from Encoder
	Status &=  ~ENC_CHANGE;
	encOutput = 0;
}


//
// Scan the Rotary Encoder
//
void encoder_Scan(void)
{
	uint8_t pha = 0, phb= 0;					// Variables containing the current encoder states

	static int8_t	increment;					// This variable used to add up changes

	if (ENC_A_PORTIN & ENC_A_PIN) pha++;		// Read Phase A
	if (ENC_B_PORTIN & ENC_B_PIN) phb++;		// Read Phase B

	if ((pha != old_pha) && (phb != old_phb))	// Both states have changed, invalid
	{
		old_pha = pha;							// Prepare for next iteration
		old_phb = phb;							// and do nothing further
	}
	
	else if (pha != old_pha)					// State of Phase A has changed
	{
		old_pha = pha;							// Store for next iteration

		if(old_pha != old_phb)					// Decide direction and
			#if	ENCODER_DIR_REVERSE
			increment--;						// increment
			#else
			increment++;						// increment
			#endif
		else
			#if	ENCODER_DIR_REVERSE
			increment++;						// or decrement
			#else
			increment--;						// or decrement
			#endif
	}
	
	else if (phb != old_phb)					// State of Phase B has changed
	{
		old_phb = phb;							// Store for next iteration

		if(old_pha != old_phb)					// Decide direction and
			#if	ENCODER_DIR_REVERSE
			increment ++;						// decrement
			#else
			increment --;						// decrement
			#endif
		else
			#if	ENCODER_DIR_REVERSE
			increment --;						// or increment
			#else
			increment ++;						// or increment
			#endif
	}

	encOutput += increment/R.encoderRes;		// Adjustable Encoder output resolution

	if (encOutput != 0)							// We have an output
	{
		increment = 0;
		Status |= ENC_CHANGE;					// Encoder state was changed
	}
}
#endif//ENCODER_SCAN_STYLE						// Rotary Encoder which scans the inputs