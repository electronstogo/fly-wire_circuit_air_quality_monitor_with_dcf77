#include "bme280.h"






BME280Sensor::BME280Sensor()
{  
  // do a sensor reset, through sending 0xB6 to register address 0xE0
  char command_data[2] = {0xE0, 0xB6};
  Wire.beginTransmission(BME280_I2C_ADDRESS);
  Wire.write(command_data, 2);
  
  
  // wait for adjust parameters, are written to memory.
  while(true)
  {
    Wire.write(0xF3);
    Wire.endTransmission();
    Wire.requestFrom(BME280_I2C_ADDRESS, 1);
    if(!(Wire.read() & 0x01))break;
    for(int i = 0; i < 200; i++);
  }
  
  Wire.endTransmission();
  

  // get adjustment parameters from memory.
  this->get_adjust_register_data();
    
  
  // set sensor asleep, before configuring.
  Wire.beginTransmission(BME280_I2C_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0b00);
  
  // trigger forced mode of bmp sensor.
  // configure all measurements, with oversampling rate 1.  
  
  // write humidity configuration.
  Wire.write(0xF2);
  Wire.write(0b001);
  
  // write temperature / pressure configuration and meas mode.
  Wire.write(0xF4);
  Wire.write(0b00100101);
  Wire.endTransmission();
  
  // disable filters.
  Wire.beginTransmission(BME280_I2C_ADDRESS);
  Wire.write(0xF5);
  Wire.write(0x0);
  Wire.endTransmission();
}




// read adjustment parameters from sensor memory.
void BME280Sensor::get_adjust_register_data()
{
	// get thirst part adjust registers.
	uint8_t reg_data[26];
	
	Wire.beginTransmission(BME280_I2C_ADDRESS);
	Wire.write(0x88);
	Wire.endTransmission();
	

	Wire.requestFrom(BME280_I2C_ADDRESS, 26);
	for(int i = 0; i < 26; i++)reg_data[i] = Wire.read();

	
	this->adjust_t1 = CONCAT_BYTES(reg_data[1], reg_data[0]);
	this->adjust_t2 = (int16_t)CONCAT_BYTES(reg_data[3], reg_data[2]);
	this->adjust_t3 = (int16_t)CONCAT_BYTES(reg_data[5], reg_data[4]);
	
	this->adjust_p1 = CONCAT_BYTES(reg_data[7], reg_data[6]);
	this->adjust_p2 = (int16_t)CONCAT_BYTES(reg_data[9], reg_data[8]);
	this->adjust_p3 = (int16_t)CONCAT_BYTES(reg_data[11], reg_data[10]);
	this->adjust_p4 = (int16_t)CONCAT_BYTES(reg_data[13], reg_data[12]);
	this->adjust_p5 = (int16_t)CONCAT_BYTES(reg_data[15], reg_data[14]);
	this->adjust_p6 = (int16_t)CONCAT_BYTES(reg_data[17], reg_data[16]);
	this->adjust_p7 = (int16_t)CONCAT_BYTES(reg_data[19], reg_data[18]);
	this->adjust_p8 = (int16_t)CONCAT_BYTES(reg_data[21], reg_data[20]);
	this->adjust_p9 = (int16_t)CONCAT_BYTES(reg_data[23], reg_data[22]);
	
	this->adjust_h1 = reg_data[25];
	
	
	// get second part adjust registers.
	Wire.beginTransmission(BME280_I2C_ADDRESS);
	Wire.write(0xE1);
	Wire.endTransmission();
	
	
	Wire.requestFrom(BME280_I2C_ADDRESS, 7);
	for(int i = 0; i < 7; i++)reg_data[i] = Wire.read();
	
	this->adjust_h2 = (int16_t)CONCAT_BYTES(reg_data[1], reg_data[0]);
	this->adjust_h3 = reg_data[2];
	
	int16_t calculation_buffer = (int16_t)(((int8_t)reg_data[3]) << 4);
	this->adjust_h4 = calculation_buffer + (int16_t)(reg_data[4] & 0x0F);
	
	calculation_buffer = (int16_t)(((int8_t)reg_data[5]) << 4);
	this->adjust_h5 = calculation_buffer + (int16_t)(reg_data[4] >> 4); 
	
	this->adjust_h6 = (int8_t)reg_data[6];
}





void BME280Sensor::do_humidity_temperature_pressure_measurement(int32_t* temperature, uint32_t* pressure, uint32_t* humidity)
{	
	// Read the humidity/temperature/pressure registers up from address 0xF7.
	// trigger a forced mode measurement.
	
	Wire.beginTransmission(BME280_I2C_ADDRESS);
	Wire.write(0xF4);
	Wire.write(0b00100101);
	
	
	// wait until the measurement finish flag is set.
	while(true)
	{
		Wire.write(0xF3);
		Wire.endTransmission();
		Wire.requestFrom(BME280_I2C_ADDRESS, 1);
		if(Wire.read() & 0x08)break;
		for(int i = 0; i < 200; i++);
	}
	
	Wire.endTransmission();
	
	uint8_t reg_data[8];
	uint32_t calculation_buffer;
	uint32_t adc_value;
	
	Wire.beginTransmission(BME280_I2C_ADDRESS);
	Wire.write(0xF7);
	Wire.endTransmission();
	
	// do a burst read, for all measurements.
	Wire.requestFrom(BME280_I2C_ADDRESS, 8);
	for(int i = 0; i < 8; i++)reg_data[i] = Wire.read();
	
	
	// calculate the temperature adc value.
	calculation_buffer = reg_data[3];
	calculation_buffer <<= 12;
	adc_value += calculation_buffer;
	calculation_buffer = reg_data[4];
	calculation_buffer <<= 4;
	adc_value += calculation_buffer;
	adc_value += reg_data[5];
	
	
	// calculate temperature measurement, from adc value.
	*temperature = this->calculate_temperature_measurement((int32_t)adc_value);
	
	
	adc_value = 0;
	
	// calculate the pressure adc value.
	calculation_buffer = reg_data[0];
	calculation_buffer <<= 12;
	adc_value += calculation_buffer;
	calculation_buffer = reg_data[1];
	calculation_buffer <<= 4;
	adc_value += calculation_buffer;
	adc_value += reg_data[2];
	
	// calculate pressure measurement, from adc value.
	*pressure = this->calculate_pressure_measurement((int32_t)adc_value);
	
	
	
	adc_value = 0;
	
	// calculate the humidity adc value.
	calculation_buffer = reg_data[6];
	calculation_buffer <<= 8;
	adc_value += calculation_buffer;
	adc_value += reg_data[7];
	
	*humidity = this->calculate_humidity_measurement((int32_t)adc_value);
	
	
	
}



int32_t BME280Sensor::calculate_temperature_measurement(int32_t adc_temp)
{	
	int32_t t1 = (int32_t)this->adjust_t1;
	int32_t t2 = (int32_t)this->adjust_t2;
	int32_t t3 = (int32_t)this->adjust_t3;
	
	
  	int32_t var1 = ((((adc_temp >> 3) - (t1 << 1))) * t2) >> 11;
  	int32_t var2 = (((((adc_temp >> 4) - t1) * ((adc_temp >> 4) - t1)) >> 12) * t3) >> 14;
          
    this->t_fine = var1 + var2;
    
    int32_t temperature =  (this->t_fine * 5 + 128) >> 8;
    
    // check temperature sensor limits, and cut if neccessary.
	if(temperature < -4000) return -4000;
	else if(temperature > 8500) return 8500;
    
    return temperature;    
}





uint32_t BME280Sensor::calculate_pressure_measurement(int32_t adc_pressure)
{
	int64_t p1 = (int64_t)this->adjust_p1;
	int64_t p2 = (int64_t)this->adjust_p2;
	int64_t p3 = (int64_t)this->adjust_p3;
	int64_t p4 = (int64_t)this->adjust_p4;
	int64_t p5 = (int64_t)this->adjust_p5;
	int64_t p6 = (int64_t)this->adjust_p6;
	int64_t p7 = (int64_t)this->adjust_p7;
	int64_t p8 = (int64_t)this->adjust_p8;
	int64_t p9 = (int64_t)this->adjust_p9;

	
	
	int64_t var1, var2, pressure;
	
	var1 = ((int64_t)t_fine) - 128000;
	var2 = var1 * var1 * p6;
	var2 = var2 + ((var1 * p5) << 17);
	var2 = var2 + (p4 << 35);
	var1 = ((var1 * var1 * p3) >> 8) + ((var1 * p2) << 12);
	var1 =(((((int64_t)1) << 47) + var1)) * p1 >> 33;

	// avoid division by zero
	if (var1 == 0){ return 300; }
	
	
	pressure = 1048576 - adc_pressure;
	pressure = (((pressure << 31) - var2) * 3125) / var1;
	var1 = (p9 * (pressure >> 13) * (pressure >> 13)) >> 25;
	var2 = (p8 * pressure) >> 19;

	pressure = ((pressure + var1 + var2) >> 8) + (p7 << 4);
	pressure = (((pressure >> 1) * 100) >> 7);
	
	
	if (pressure < 3000000)
	{
		return 300;
	}
	else if (pressure > 11000000)
	{
		return 1100;
	}
	
	return (uint32_t)pressure / 10000;
}





uint32_t BME280Sensor::calculate_humidity_measurement(int32_t adc_humidity)
{
	int32_t h1 = (int32_t)this->adjust_h1;
	int32_t h2 = (int32_t)this->adjust_h2;
	int32_t h3 = (int32_t)this->adjust_h3;
	int32_t h4 = (int32_t)this->adjust_h4;
	int32_t h5 = (int32_t)this->adjust_h5;
	int32_t h6 = (int32_t)this->adjust_h6;
	
	
	int32_t v_x1_u32r = (this->t_fine - ((int32_t)76800));

	v_x1_u32r = (((((adc_humidity << 14) - (h4 << 20) - (h5 * v_x1_u32r) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * h6) >> 10) * (((v_x1_u32r * h3) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * h2 + 8192) >> 14)));

	v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * h1) >> 4));

	v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
	v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;

	uint32_t humidity = (uint32_t)(v_x1_u32r >> 12);

	return humidity;
}
