#include <avr/sleep.h>
#include "sh1106.h"
#include "bme280.h"
#include "sgp40.h"
#include "dcf77.h"
#include <Wire.h>


// button input pin for screen change. 
#define SWITCH_INPUT_PIN	6





//// RTC handling stuff.


#define RTC_I2C_ADDRESS 0x68


// Sets the time in the RTC registers, with the given arguments.
// This function should only be used, when its neccessary to set the correct time.
void set_rtc_time(unsigned int year, unsigned int month, unsigned int day_of_month, unsigned int week_day, unsigned int hours, unsigned int minutes, unsigned int seconds)
{
	Wire.beginTransmission(RTC_I2C_ADDRESS);
	Wire.write(0x0);
	Wire.write(((seconds / 10) << 4) + (seconds % 10));
	Wire.write(((minutes / 10) << 4) + (minutes % 10));
	
	// set hours, and activate 24 hr mode.
	byte hour_byte = ((hours / 10) << 4) + (hours % 10);
	Wire.write(hour_byte & 0b10111111);
	
	Wire.write(week_day);
	Wire.write(((day_of_month / 10) << 4) + (day_of_month % 10));
	Wire.write(((month / 10) << 4) + (month % 10));
	Wire.write(((year / 10) << 4) + (year % 10) + 0x80);
	
	Wire.endTransmission();
}


// read time data from RTC controller DS3231. 
void get_rtc_time_values(unsigned int* year, unsigned int* month, unsigned int* day_of_month, unsigned int* week_day, unsigned int* hours, unsigned int* minutes, unsigned int* seconds)
{
	// Start reading the RTC registers from address 0x0.
	Wire.beginTransmission(RTC_I2C_ADDRESS);
	Wire.write(0x0);
	Wire.endTransmission();


	// Get 3 bytes (seconds register, minutes register, hours register) 
	Wire.requestFrom(RTC_I2C_ADDRESS, 7);
	*seconds = bcd_to_dec(Wire.read() & 0x7F);
	*minutes = bcd_to_dec(Wire.read() & 0x7F);
	*hours = bcd_to_dec(Wire.read() & 0x7F);
	*week_day = bcd_to_dec(Wire.read() & 0x7);
	*day_of_month = bcd_to_dec(Wire.read() & 0x3F);
	*month = bcd_to_dec(Wire.read() & 0x3);
	*year = bcd_to_dec(Wire.read() & 0x7F);
}


// get dec format number, from bcd
byte bcd_to_dec(byte bcd_value)
{
	return (bcd_value & 0x0F) + (bcd_value >> 4) * 10;
}






//// Controller handling stuff.

// init stuff for microcontroller.
void init_mcu()
{
	// enable external interrupt rising edge at INT1 pin
	EICRA |= 0x0C;
	
	// acivate extern interrupt INT1
	EIMSK = 0x02;
	
	// reset both extern interrupt flags.
	EIFR  = 0x03;	
}



// external interrupt at INT1 pin, triggered by RTC 1Hz signal.
ISR (INT1_vect)
{
	// just wake up controller, for a display refresh.
}





//// Application stuff.

// Arduino init
void setup()
{
    // disable interrupts
    cli();

	// switch Input for display view switch. 
    pinMode(SWITCH_INPUT_PIN, INPUT);
    
    // activate the pull up for the switch pin.
    digitalWrite(SWITCH_INPUT_PIN, HIGH);

	// init microcontroller
    init_mcu();
    
    // init arduino i2c framework class.
    Wire.begin();
    
    // enable interrupts
    sei();
}





// Arduino while loop
void loop()
{  
	// Initiate the class variables.
	// Attention: This doesnt work during the Arduino setup() function,
	// because the I2C communication in the native Wire (TWI)library class will stuck.

	// SH1106 controller class variable to control the OLED display.
	SH1106 sh1106(0x3C);
	char* char_pointer;
	String string_buffer;

	// BME280 sensor class variable to read BME280 measurements.
	BME280Sensor bme280_sensor;

	// SGP40 sensor class variable to read SGP40 measurements.
	SGP40Sensor sgp40_sensor;

	// DCF77 class to syncronize the time.
	DCF77 dcf77;

	// value that activates a time syncronizing process with DCF77.
	bool sync_mode_active = true;


	unsigned char current_page_index = 1;
	bool view_switch_enabled = false;

	// Following communication sets the control register of the RTC, for the 1Hz interrupt.
	//Wire.beginTransmission(RTC_I2C_ADDRESS);
	//Wire.write(0x0E);
	//Wire.write(0x40);
	//Wire.endTransmission();
	
	while(1)
	{     	

		if(sync_mode_active)
		{
			sh1106.clear_oled_buffer();
			sh1106.draw_string(20, 50, "TIME");
			sh1106.draw_string(20, 30, "SYNC...");
			sh1106.flush_oled_buffer();

			// do a time syncronization process.
			if(dcf77.syncronize_time())
			{
				set_rtc_time(dcf77.year, dcf77.month, dcf77.day_of_month, dcf77.week_day, dcf77.hours, dcf77.minutes, 0);
			}

			// disable syncronization mode.
			sync_mode_active = false;
		}



		// Show sensor/time data on the OLED display. 

		// Clear the OLED display.
		sh1106.clear_oled_buffer(); 


		// do a BME280 sensor measurement.
		int32_t temperature;
		uint32_t pressure, humidity;
		bme280_sensor.do_humidity_temperature_pressure_measurement(&temperature, &pressure, &humidity);

		// get the VOC index from SGP40.
		unsigned int voc_index;
		if(!sgp40_sensor.get_voc_index((float)temperature / 1000.0, (float)humidity / 100.0, &voc_index))
		{
			voc_index = 1234;
		}


		// check if the user pushes the button
		if(!digitalRead(SWITCH_INPUT_PIN) && view_switch_enabled)
		{
			current_page_index = !current_page_index;
			view_switch_enabled = false; 
		}
		else
		{
			view_switch_enabled = true;
		}


		if(current_page_index == 0)
		{
			//concatenate humidity.
			string_buffer = String((float)humidity / 1000.0, 1);
			string_buffer += "% R.H.";
			string_buffer += '\0';
			char_pointer = string_buffer.c_str();
			sh1106.draw_string(0, 60, char_pointer);

			// concatenate temperature.
			string_buffer = String(temperature / 100.0, 1);
			string_buffer += "$C";
			string_buffer += '\0';
			char_pointer = string_buffer.c_str();
			sh1106.draw_string(0, 40, char_pointer);

			// pressure.
			string_buffer = String(pressure);
			string_buffer += " MBAR";
			string_buffer += '\0';
			char_pointer = string_buffer.c_str();
			sh1106.draw_string(0, 20, char_pointer);

		}
		else
		{
			string_buffer = "VOC: ";

			if(voc_index != 1234)
			{
				string_buffer += String(voc_index);
			}
			else
			{
				string_buffer += "ERROR";
			}

			string_buffer += '\0';
			char_pointer = string_buffer.c_str();
			sh1106.draw_string(0, 60, char_pointer);

			// read the current time from RTC.
			unsigned int hours, minutes, seconds, day_of_month, week_day, year, month;
			get_rtc_time_values(&year, &month, &day_of_month, &week_day, &hours, &minutes, &seconds);

			const char* week_day_names[7] = {"MO", "DI", "MI", "DO", "FR", "SA", "SO"};

			// Transform a 2 digit string for display.
			char time_string[10];

			sprintf(time_string, "%i%i", hours / 10, hours % 10);
			string_buffer = String(time_string);
			string_buffer += ":";
			sprintf(time_string, "%i%i", minutes / 10, minutes % 10);
			string_buffer += String(time_string);
			string_buffer += ":";
			sprintf(time_string, "%i%i", seconds / 10, seconds % 10);
			string_buffer += String(time_string);
			string_buffer += "  ";
			string_buffer += String(week_day_names[week_day - 1]);
			string_buffer += '\0';
			char_pointer = string_buffer.c_str();
			sh1106.draw_string(0, 40, char_pointer);


			sprintf(time_string, "%i%i", day_of_month / 10, day_of_month % 10);
			string_buffer = String(time_string);
			string_buffer += ".";
			sprintf(time_string, "%i%i", month / 10, month % 10);
			string_buffer += String(time_string);
			string_buffer += ".";
			string_buffer += String(2000 + year);//year);
			string_buffer += '\0';
			char_pointer = string_buffer.c_str();
			sh1106.draw_string(0, 20, char_pointer);
		}

		// Draw data on the OLED display now.
		sh1106.flush_oled_buffer();




		// enter sleep mode, no need for a active controller, until the next display refresh.
		// RTC 1Hz output will awake controller through a extern interrupt.
		SMCR |= (1 << 0);
		SMCR |= (1 << 2);
		sleep_cpu();
	}
}
