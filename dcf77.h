// 2020 - electronstogo

#ifndef DCF77_H_
#define DCF77_H_

#define DCF77_DATA_PIN		2
#define DCF77_ENABLE_PIN	4



class DCF77
{
	private:
		void calculate_time_from_dcf77_data(unsigned char* dcf_data, unsigned long last_signal_timestamp);
		void handle_dcf_signal(int signal_low_time, unsigned int* dcf_data_index, unsigned char* dcf_data);
		unsigned char get_bit_value(unsigned char *data, unsigned int index);
			
	public:
		DCF77();
		bool syncronize_time();
		unsigned char minutes;
		unsigned char hours;
		unsigned char week_day;
		unsigned char day_of_month;
		unsigned char month;
		unsigned char year;
};




#endif
