// 2020 - electronstogo


#include "dcf77.h"
#include "Arduino.h"






DCF77::DCF77()
{
	// configurate the DCF77 interrupt and enable pins.
	pinMode(DCF77_DATA_PIN, INPUT);
	pinMode(DCF77_ENABLE_PIN, OUTPUT);

	// activate the pull up for the data pin.
	// Is used because the DCF77 signal comes from a open collectot output.
	digitalWrite(DCF77_DATA_PIN, HIGH);

	// activate the DCF77 module.
	digitalWrite(DCF77_ENABLE_PIN, HIGH);
	delay(100);
	digitalWrite(DCF77_ENABLE_PIN, LOW);

}



// values for interrupt signal communication.
unsigned long timestamp_high_signal;
unsigned long timestamp_low_signal;
bool dcf_signal_triggered = false;


// external interrupt at INT0 pin, triggered by both level DCF signals.
ISR(INT0_vect)
{
	static unsigned char last_signal_level = 1;
	unsigned long current_timestamp = millis();


	// check signal level, and save the respective timestamp.
	if(digitalRead(DCF77_DATA_PIN) && !last_signal_level)
	{	 
		last_signal_level = 1; 
		timestamp_high_signal = current_timestamp;
		dcf_signal_triggered = true;
	}
	else if(!digitalRead(DCF77_DATA_PIN) && last_signal_level)
	{
		last_signal_level = 0;
		timestamp_low_signal = current_timestamp;
	}
}




// Function trys to receive the current time from DCF77.
// A watchdog timer cancels the function, if the process takes to long.
bool DCF77::syncronize_time()
{
	// enable external interrupt for both edges at INT0 pin.
	EICRA |= 0x01;	
	// acivate extern interrupt INT1
	EIMSK |= 0x01;



	// array to keep the read dcf data signals.
	unsigned char dcf_data[8];
	memset(dcf_data, 0, 8);

	unsigned int dcf_data_index = 1234;
	unsigned int watchdog_counter = 0;

	// this timestamp is used to calculate the waiting time, until the minute is finished
	unsigned long signal_timestamp;

	while(1)
	{

		// loop until the next bit from DCF77 was received. 
		while(!dcf_signal_triggered)
		{	
			delay(1);
		}

		signal_timestamp = millis();

		// analyze the signal timings.
		this->handle_dcf_signal(timestamp_high_signal - timestamp_low_signal, &dcf_data_index, dcf_data);

		watchdog_counter += 1;
		dcf_signal_triggered = false;



		// cancel the process after 5 min, in case of bad signal.
		if(watchdog_counter > 300)
		{
			// disable extern Interrupt for DCF77.
			EIMSK &= ~0x01;	
			return false;
		}


		if(dcf_data_index == 58)
		{
			calculate_time_from_dcf77_data(dcf_data, signal_timestamp);

			// disable extern Interrupt for DCF77.
			EIMSK &= ~0x01;

			// validate the received time data.
			bool result = this->minutes < 60;
			result = (this->hours < 24) && result;
			result = (this->week_day <= 7) && result;
			result = (this->day_of_month <= 31) && result;
			result = (this->month <= 12) && result;
			result = (this->year < 100) && result; 			

			return result;
		}
	}
}




// Function analyzes the signal timings, and updates the DCF77 time data.
void DCF77::handle_dcf_signal(int signal_low_time, unsigned int* dcf_data_index, unsigned char* dcf_data)
{
	if(signal_low_time > 1500)
	{
		*dcf_data_index = 0;
		return;
	}

	// catch invalid array indices.
	if(*dcf_data_index >= 60)
	{
		return;
	}
	


	// set data from timing interpretation.
	if(signal_low_time >= 750 && signal_low_time < 850)
	{
		dcf_data[*dcf_data_index / 8] |= (0x1 << (*dcf_data_index % 8));
	}
	else if(signal_low_time >= 850 && signal_low_time < 950)
	{
		// data is reseted with 0 values anyway.
	}
	else
	{
		// catch error timings here.
		*dcf_data_index = 1234;
		return;
	}
		

	*dcf_data_index += 1;
}



// Calculate the time components from DCF77 data.
void DCF77::calculate_time_from_dcf77_data(unsigned char* dcf_data, unsigned long last_signal_timestamp)
{
	const unsigned int factors[8] = {1, 2, 4, 8, 10, 20, 40};
	

	// wait until the :00 seconds moment.
	signed int waiting_time = 2000 - (millis() - last_signal_timestamp) - 250;
	
	if(waiting_time > 0)
	{
		delay(waiting_time);
	}	
	
	this->minutes = 0;
	this->hours = 0;
	this->day_of_month = 0;
	this->week_day = 0;
	this->month = 0;
	this->year = 0;
	
	for(int i = 0; i < 7; i++)
	{
		this->minutes += factors[i] * get_bit_value(dcf_data, 21 + i);
		this->year += factors[i] * get_bit_value(dcf_data, 50 + i);
		
		if(i >= 6)
		{
			continue;
		}
		
		this->hours += factors[i] * get_bit_value(dcf_data, 29 + i);
		this->day_of_month += factors[i] * get_bit_value(dcf_data, 36 + i);
		
		if(i >= 5)
		{
			continue;
		}
		
		this->month += factors[i] * get_bit_value(dcf_data, 45 + i);
		
		if(i >= 3)
		{
			continue;
		}
		
		this->week_day += factors[i] * get_bit_value(dcf_data, 42 + i);
	}
}




// get bit value from data by index.
unsigned char DCF77::get_bit_value(unsigned char *data, unsigned int index)
{
	unsigned char bit_value = data[index / 8] & (0x1 << ((index) % 8)) ? 1 : 0;

	return bit_value;
}
