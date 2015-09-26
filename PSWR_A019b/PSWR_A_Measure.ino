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

//
//-----------------------------------------------------------------------------------------
//
//      Power and SWR Meter Functions
//
//      Measure Forward and Reverse Power from a Tandem Match Coupler
//      to determine Power and SWR 
//
//-----------------------------------------------------------------------------------------
//

#if defined(__MK20DX256__)
//-----------------------------------------------------------------------------------------
// If Teensy 3.1 ARM Cortex M4 microcontroller, then instanciate an ADC Object
// TODO - ARDUINO PREPROCESSOR BUG - the #include below has to be commented out by hand if not Teensy 3.1
#include <ADC.h>                      // Syncrhonous read of the two builtin ADC, if using Teensy 3.1
ADC *adc = new ADC();
#endif

int8_t  ad7991_addr = 0;              // Address of AD7991 I2C connected A/D, 0 if none detected
#define AD7991_0  0x28                // I2C Address of AD7991-0
#define AD7991_1  0x29                // I2C Address of AD7991-1

//
//-----------------------------------------------------------------------------------------
//                                Initialize builtin ADCs.
//
// If AVR Microcontroller then 10 bit ADC at 2.56V. If Teensy 3.1 then 2x12 bit ADCs at 3.3V
//-----------------------------------------------------------------------------------------
//
void adc_init(void)
{ 
  #if defined(__MK20DX256__)
  // If Teensy 3.1 MK20DX256 (ARM Cortex M4) based microcontroller, then set up the two separate ADCs
  // for synchronous read at 12 bit resolution and lowest possible measurement speed (minimal noise) 
  pinMode(Pfwd, INPUT);
  pinMode(Pref, INPUT);
  adc->setSamplingSpeed(ADC_VERY_LOW_SPEED);     // Sampling speed, ADC_VERY_LOW_SPEED, ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED or ADC_VERY_HIGH_SPEED
  adc->setSamplingSpeed(ADC_VERY_LOW_SPEED, ADC_1);
  adc->setConversionSpeed(ADC_VERY_LOW_SPEED);   // Conversion speed
  adc->setConversionSpeed(ADC_VERY_LOW_SPEED, ADC_1);
  adc->setResolution(12);                        // AD resolution, 12 bits
  adc->setResolution(12, ADC_1);
  adc->setAveraging(32);                         // Averaging of 32 samples at very low speed results in each measurement taking 580us
  adc->setAveraging(32, ADC_1);

  #else                                          // Atmel AVR based microcontroler, like AT90USB1286 or ATmega1280/2560 etc...
  analogReference(INTERNAL2V56);                 // Use internal 2.56V reference rather than 5V
  #endif                                         // for better bit resolution
}


#if WIRE_ENABLED
//
//-----------------------------------------------------------------------------------------
//                    Scan I2C bus for AD7991 - 4x 12bit AD
//
// Return which type detected (AD7991-0 or AD7991-1) or 0 if nothing found
//-----------------------------------------------------------------------------------------
//
uint8_t I2C_Init(void)
{
  uint8_t found;                      // Returns status of I2C bus scan

  // Scan for AD7991-0 or AD7991-1
  ad7991_addr = AD7991_1;             // We start testing for an AD7991-1
  found = 2;						//  Assume it will be found
  Wire.beginTransmission(ad7991_addr);
  if(Wire.endTransmission() != 0)     // If error
  {
    ad7991_addr = AD7991_0;           // AD7991-0 was not detected
    found = 1;                        // We may have an AD7991-0	
  }
  Wire.beginTransmission(ad7991_addr);
  if(Wire.endTransmission() != 0)     // If error
  {
    ad7991_addr = 0;                  // AD7991-1 was not detected
    found = 0;                        // No I2C capable device was detected
  }

  // If Power & SWR Code and AD7991 12 bit ADC, the program it to use a 2.6V reference connected
  // to ADC channel 4 and only read ADC channels 1 and 2 consecutively.
  //
  if (found)                          // If AD7991 ADC was found, then configure it...
  {
    //uint8_t writePacket = 0x38;     // Set ADCs 1 and 2 for consecutive reads and REF_SEL = external
    //TWI_WritePacket(ad7991_addr,10,&writePacket,0,&writePacket,1);
    Wire.beginTransmission(ad7991_addr);
    Wire.write(0x38);                 // Set ADCs 1 and 2 for consecutive reads and REF_SEL = external
    Wire.endTransmission();
  }
  return found;
}
#endif

//-----------------------------------------------------------------------------------------
//                            Poll the AD7991 2 x ADC chip
//                                  or alternately
//              use built-in 10 bit A/D converters if I2C device not present
//-----------------------------------------------------------------------------------------
//
// This function reads the A/D inputs
void adc_poll(void)
{
  #if WIRE_ENABLED  
  uint16_t adc_in[4];
  uint8_t  read_B[4];
  uint8_t  i=0;
  #endif
  #if defined(__MK20DX256__)          // If Teensy 3.1 ARM Cortex M4 microcontroller
  ADC::Sync_result result;
  #endif
  
  #if WIRE_ENABLED  
  //-----------------------------------------------------------------------------
  // use I2C connected AD7991 12-bit AD converter, if it was detected during init
  if (ad7991_addr)
  {
    Wire.requestFrom(ad7991_addr, 4);
    while (Wire.available()) read_B[i++] = Wire.read();

    // The output of the 12bit ADCs is contained in two consecutive byte pairs
    // read from the AD7991.  In theory, the second could be read before the first.
    // Each of the AD7991 four builtin ADCs has an address identifier (0 to 3)
    // in the uppermost 4 bits of the first byte.  The lowermost 4 bits of the first
    // byte are bits 8-12 of the A/D output.
    // In this routine we only read the two first ADCs, as set up in the I2C_Init()
    //
    adc_in[(read_B[0] >> 4) & 0x03] = (read_B[0] & 0x0f) * 0x100 + read_B[1];
    adc_in[(read_B[2] >> 4) & 0x03] = (read_B[2] & 0x0f) * 0x100 + read_B[3];
    fwd = adc_in[0];           // Input from AD7991 is 12 bit resolution
    ref = adc_in[1];           // contained in bit positions 0 to 11.
  }
  else
  #endif
  //----------------------------------------------------------------------------
  // If no I2C, then use builtin A/D converters (12 bit resolution if Teensy 3.1)
  {
    //------------------------------------------
    #if defined(__MK20DX256__)        // If Teensy 3.1 ARM Cortex M4 microcontroller
    result = adc->analogSynchronizedRead(Pref, Pfwd);  // ref=ADC0, fwd=ADC1
    if( (result.result_adc0 !=ADC_ERROR_VALUE) && (result.result_adc1 !=ADC_ERROR_VALUE) )
    {
      fwd = result.result_adc1 *3.25/2.56;      // Compensate for using 3.3V rather than
      ref = result.result_adc0 *3.25/2.56;      //   2.56V as voltage reference.
    }
    else  // error
    {
      fwd = -1;                           // Should never happen
      ref = -1;
    }

    //------------------------------------------
    #else // Atmel AVR 8 bit microcontroller
    fwd = analogRead(Pfwd) << 2;   // Read FWD and Convert from 10 to 12 bits
    ref = analogRead(Pref) << 2;   // Read REF and Convert from 10 to 12 bits
    #endif
  }
}


//
//-----------------------------------------------------------------------------------------
//                Calculate SWR if we have sufficient input power
//-----------------------------------------------------------------------------------------
//
void calculate_SWR(double v_fwd, double v_rev)
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
      if (flag.swr_alarm == TRUE)                // This is the second time in a row, raise the SWR ALARM
      {
        digitalWrite(R_Led,flag.swr_alarm);      // Turn on the Alarm LED
      }
      flag.swr_alarm = TRUE;                     // Set SWR flag - may be a one shot fluke
    }
    else if (flag.swr_alarm == TRUE)             // Clear SWR Flag if High SWR condition does not exist
    {
      flag.swr_alarm = FALSE;
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


//-----------------------------------------------------------------------------
// AD8307 specific Power measurement functions
#if AD8307_INSTALLED
//
//-----------------------------------------------------------------------------------------
//                Convert Voltage Reading into Power
//-----------------------------------------------------------------------------------------
//
void pswr_determine_dBm(void)
{
  double  delta_db;

  int16_t delta_F, delta_R;
  double  delta_Fdb, delta_Rdb;
  double  temp;

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
  ad8307_FdBm = (fwd - R.cal_AD[0].Fwd) * delta_Fdb + R.cal_AD[0].db10m/10.0;
  ad8307_RdBm = (ref - R.cal_AD[0].Rev) * delta_Rdb + R.cal_AD[0].db10m/10.0;

  // Test for direction of power - Always designate the higher power as "forward"
  // while setting the "Reverse" flag on reverse condition.
  if (ad8307_FdBm > ad8307_RdBm)        // Forward direction
  {
    Reverse = FALSE;
  }
  else                                  // Reverse direction
  {
    temp = ad8307_RdBm;
    ad8307_RdBm = ad8307_FdBm;
    ad8307_FdBm = temp;
    Reverse = TRUE;
  }
}


//
//-----------------------------------------------------------------------------------------
//                Calculate all kinds of power
//-----------------------------------------------------------------------------------------
//
void pswr_calc_Power(void)
{
  double f_inst;                                // Calculated Forward voltage
  double r_inst;                                // Calculated Reverse voltage

  // For measurement of peak and average power
  static int16_t db_buff[PEP_BUFFER];           // dB voltage information in a one second window
  static int16_t db_buff_short[BUF_SHORT];      // dB voltage information in a 100 ms window
  static double	p_avg_buf[AVG_BUF];             // all instantaneous power measurements in 1s
  static double	p_plus;                         // all power measurements within a 1s window added together
  static uint16_t a=0;                          // PEP ring buffer counter
  static uint8_t b=0;                           // 100ms ring buffer counter
  static uint8_t c=0;                           // 1s average ring buffer counter
  int16_t max=-32767, pk=-32767;                // Keep track of Max (1s) and Peak (100ms) dB voltage

  // Instantaneous forward voltage and power, milliwatts and dBm
  f_inst = pow(10,ad8307_FdBm/20.0);		// (We use voltage later on, for SWR calc)
  fwd_power_mw = SQR(f_inst);			// P_mw = (V*V) (current and resistance have already been factored in
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
  p_avg_buf[c] = power_mw;                      // Add the newest value onto ring buffer
  p_plus = p_plus + power_mw;                   // Add latest value to the total sum of all measurements in 1s
  if (c < AVG_BUF-1)                            // and subtract the oldest value in the ring buffer from the total sum
    p_plus = p_plus - p_avg_buf[c+1];
  else
    p_plus = p_plus - p_avg_buf[0];
  c++;                                          // Rotate window by advancing ring buffer counter
  if (c == AVG_BUF) c = 0;
  power_mw_avg = p_plus / (AVG_BUF);            // And finally, find the average
  power_db_avg = 10 * log10(power_mw_avg);


  //// Modulation index in a 1s window
  //double v1, v2;
  //v1 = sqrt(ABS(power_mw_pep));
  //v2 = sqrt(ABS(power_mw_avg));
  //modulation_index = (v1-v2) / v2;

  calculate_SWR(f_inst, r_inst);
}


//-----------------------------------------------------------------------------
// Diode detector specific Power measurement functions
#else
//
//-----------------------------------------------------------------------------------------
// 			Calculate all kinds of power
//-----------------------------------------------------------------------------------------
//
void calculate_pep_and_pk(int32_t p)
{
  // For measurement of peak (100ms pep) and pep (1s pep) power
  static int32_t buff[PEP_BUFFER];              // voltage information in a one second window
  static int32_t buff_short[BUF_SHORT];         // voltage information in a 100 ms window
  static int32_t buff_avg[AVG_BUF];             // all instantaneous power measurements in 1s
  static int64_t p_plus;                        // all power measurements within a 1s window added together

  static uint16_t a=0;                          // PEP ring buffer counter
  static uint8_t b=0;                           // 100ms ring buffer counter
  static uint8_t c=0;                           // 1s average ring buffer counter
  int32_t mx=0, pk=0;                           // Keep track of Max (1s) and Peak (100ms) voltage

  // Find peaks and averages
  // Store power level (mw) in two ring buffers
  buff[a] = buff_short[b] = p;
  // Advance PEP [1] and Peak [100ms] ringbuffer counters
  a++, b++;
  if (a >= R.PEP_period) a = 0;
  if (b >= BUF_SHORT) b = 0;

  // Retrieve Max Value within a [1, 2.5 or 5] second sliding window
  for (uint16_t x = 0; x < R.PEP_period; x++)
  {
    if (mx < buff[x]) mx = buff[x];
  }

  // Find Peak value within a [100ms] sliding window
  for (uint8_t x = 0; x < BUF_SHORT; x++)
  {
    if (pk < buff_short[x]) pk = buff_short[x];
  }

  // Average power (1 second), milliwatts and dBm
  buff_avg[c] = power_mw;                       // Add the newest value onto ring buffer
  p_plus = p_plus + power_mw;                   // Add latest value to the total sum of all measurements in 1s
  if (c < AVG_BUF-1)                            // and subtract the oldest value in the ring buffer from the total sum
    p_plus = p_plus - buff_avg[c+1];
  else
    p_plus = p_plus - buff_avg[0];
  c++;                                          // Rotate window by advancing ring buffer counter
  if (c == AVG_BUF) c = 0;

  power_mw_pep = mx;                            // PEP  (largest value measured in a sliding window of 1, 2.5 or 5 sec)
  power_db_pep = 10 * log10(power_db_pep);
  power_mw_pk = pk;                             // Peak (typically 100 milliseconds)
  power_db_pk = 10 * log10(power_db_pk);
  power_mw_avg = p_plus / (AVG_BUF);            // And finally, find the average
  power_db_avg = 10 * log10(power_mw_avg);
}

//
//---------------------------------------------------------------------------------
// Measure Forward and Reflected power and process
// Polled once every 5ms normally.  However, when stepper is in use,
// then polled once for each step progressed
//---------------------------------------------------------------------------------
//

void measure_power_and_swr(void)
{
  double v_fwd;                                 // Calculated Forward voltage
  double v_ref;                                 // Calculated Reflected voltage
  uint16_t temp;

  // Test for direction of power - Always designate the higher power as "forward"
  // while setting the "Reverse" flag on reverse condition.
  if (fwd > ref)                                // Forward direction
  {
    Reverse = FALSE;
  }
  else                                          // Reverse direction
  {
    temp = ref;
    ref = fwd;
    fwd = temp;
    Reverse = TRUE;
  }

  // Instantaneous forward voltage and power, milliwatts
  //
  // Establish actual measured voltage at diode
  v_fwd = fwd * 3.3/4096.0 * (VALUE_R2ND + VALUE_R1ST)/VALUE_R2ND;
  // Convert to VRMS in Bridge
  if (v_fwd >= D_VDROP) v_fwd = 1/1.4142135 * (v_fwd - D_VDROP) + D_VDROP;
  // Take Bridge Coupling into account
  v_fwd = v_fwd * BRIDGE_COUPLING * R.meter_cal/100.0;
  // Convert into milliwatts
  fwd_power_mw = 1000 * v_fwd*v_fwd/50.0;
  	
  // Instantaneous reflected voltage and power
  //
  // Establish actual measured voltage at diode
  v_ref = ref * 3.3/4096.0 * (VALUE_R2ND + VALUE_R1ST)/VALUE_R2ND;
  // Convert to VRMS in Bridge
  if (v_ref >= D_VDROP) v_ref = 1/1.4142135 * (v_ref - D_VDROP) + D_VDROP;
  // Take Bridge Coupling into account
  v_ref = v_ref * BRIDGE_COUPLING * R.meter_cal/100.0;
  // Convert into milliwatts
  ref_power_mw = 1000 * v_ref*v_ref/50.0;
	
  // Instantaneous Real Power Output
  power_mw = fwd_power_mw - ref_power_mw;
  if (power_mw <  0) power_mw = power_mw * -1;

  calculate_pep_and_pk(power_mw);    // rate, hence this would make limited sense
  
  calculate_SWR(v_fwd, v_ref);         // Calculate measured_swr based on forward and reflected voltages
}
#endif
