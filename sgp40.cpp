// 2020 - electronstogo

#include "sgp40.h"
#include <math.h>
#include <stdlib.h>
#include <util/delay.h>
#include "Arduino.h"




SGP40Sensor::SGP40Sensor()
{   
	// Get the VOC index algorithm parameters.
	VocAlgorithm_init(&this->voc_algorithm_parameters);
}





// returns the raw measurement from SGP40 sensor, after sending the current temperature and humidity.
bool SGP40Sensor::do_raw_measurement(float temperature, float humidity, unsigned int* raw_value)
{
	unsigned char i2c_command_data[8];
	
	i2c_command_data[0] = 0x26;
	i2c_command_data[1] = 0x0F;
	
	
	// Send the themperature & humidity to the sensor.
	
	// convert data into ticks (see datasheet 1.0 page 13).
	unsigned int tick_value = (unsigned int)((humidity * 65535) / 100 + 0.5);
	i2c_command_data[2] = tick_value >> 8;
	i2c_command_data[3] = tick_value && 0xFF;
	i2c_command_data[4] = this->generate_crc(i2c_command_data + 2, 2);
	
	tick_value = (uint16_t)(((temperature + 45) * 65535) / 175);
	i2c_command_data[5] = tick_value >> 8;
	i2c_command_data[6] = tick_value && 0xFF;
	i2c_command_data[7] = this->generate_crc(i2c_command_data + 5, 2);
	
	Wire.beginTransmission(SGP40_I2C_ADDRESS);
	Wire.write(i2c_command_data, 8);
	Wire.endTransmission();
	
	// wait until measurement is finished.
   	 _delay_ms(25);
	
	// Get the raw data from sensor.
	Wire.requestFrom(SGP40_I2C_ADDRESS, 3);
	unsigned char i2c_receive_data[2];
	
	i2c_receive_data[0] = Wire.read();
	i2c_receive_data[1] = Wire.read();
	unsigned char crc = Wire.read();
	

	// calculate the VOC raw value
	*raw_value = i2c_receive_data[0] << 8;
	*raw_value += i2c_receive_data[1];
	
	
	
	// CRC check of received raw data.
	if(this->generate_crc(i2c_receive_data, 2) != crc)
	{
		return false;
	}
	
	return true;
}



// get the VOC index from SGP40. The VOC index is an indicator of the measured air quality.
bool SGP40Sensor::get_voc_index(float temperature, float humidity, unsigned int* voc_index)
{
	long int voc_index_buffer;
	unsigned int raw_value;
	
	if(!do_raw_measurement(temperature, humidity, &raw_value))
	{
		return false;
	}
	
	// call the Sensirion SGP40 algorithm function to get the VOC index.
	VocAlgorithm_process(&voc_algorithm_parameters, raw_value, &voc_index_buffer);

	*voc_index = (unsigned int)voc_index_buffer;
	
	return true;
}





// function that calculates the CRC value from given data.
unsigned char SGP40Sensor::generate_crc(unsigned char *data, unsigned char data_length)
{
	unsigned char crc = 0xFF;

	for(unsigned char i = 0; i < data_length; i++)
	{
		//crc ^= (data >> (i * 8)) & 0xFF;
		crc ^= data[i];

		for(unsigned char b = 0; b < 8; b++)
		{
			if(crc & 0x80)
			{
				crc = (crc << 1) ^ SGP40_CRC8_POLYNOMIAL;
			}
			else
			{
				crc <<= 1;
		  	}
		}
	}
  
	return crc;
}
