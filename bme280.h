// 2020 - electronstogo

#ifndef BME280_H_
#define BME280_H_


#include <Wire.h>

#define BME280_I2C_ADDRESS 	0x76

#define CONCAT_BYTES(msb, lsb)             (((uint16_t)msb << 8) | (uint16_t)lsb)


class BME280Sensor
{
	private:
		// adjustment parameters from sensor memory.
		uint16_t adjust_t1;
		int16_t adjust_t2;	
		int16_t adjust_t3;
		
		uint16_t adjust_p1;
		int16_t adjust_p2;
		int16_t adjust_p3;
		int16_t adjust_p4;
		int16_t adjust_p5;
		int16_t adjust_p6;
		int16_t adjust_p7;
		int16_t adjust_p8;
		int16_t adjust_p9;
		
		uint8_t adjust_h1;
		int16_t adjust_h2;
		uint8_t adjust_h3;
		int16_t adjust_h4;
		int16_t adjust_h5;
		int8_t adjust_h6;

		int32_t t_fine;
		
		// intern sensor handling functions.
		void get_adjust_register_data();
		int32_t calculate_temperature_measurement(int32_t adc_temperature);
		uint32_t calculate_pressure_measurement(int32_t adc_pressure);
		uint32_t calculate_humidity_measurement(int32_t adc_humidity);
		
	public:
		BME280Sensor();
		void init();
		void do_humidity_temperature_pressure_measurement(int32_t* temperature, uint32_t* pressure, uint32_t* humidity);

};




#endif
