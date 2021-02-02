#ifndef SGP40_H_
#define SGP40_H_

#include <Wire.h>
extern "C" {
#include "sensirion_voc_algorithm.h"
};

#define SGP40_I2C_ADDRESS 	0x59
#define SGP40_CRC8_POLYNOMIAL 	0x31


class SGP40Sensor
{
	private:
		unsigned char generate_crc(unsigned char *data, unsigned char data_length);
		bool do_raw_measurement(float temperature, float humidity, unsigned int* raw_value);
		
		
		// voc algorithm parameters needed for Sensirion VOC index algorithm.
		VocAlgorithmParams voc_algorithm_parameters;
			
	public:
		SGP40Sensor();
		bool get_voc_index(float temperature, float humidity, unsigned int* voc_index);

};




#endif
