#ifndef SH1106_H_
#define SH1106_H_



#define SH1106_I2C_ADDRESS 	0x3C//0x3D


#define OLED_WIDTH 132
#define OLED_HEIGHT 64



class SH1106
{
	private:
		void write_command(unsigned char command);
		void write_data(unsigned char *data, unsigned data_length);
		void set_page(unsigned int page);
		void draw_point(unsigned int x, unsigned int y);
		void draw_letter(unsigned int x_pos, unsigned int y_pos, unsigned int char_index);
		
		unsigned char i2c_address;
		
	public:
		SH1106(unsigned char sh1106_i2c_address = SH1106_I2C_ADDRESS);
		void clear_oled_buffer();
		void flush_oled_buffer();
		void draw_string(unsigned int x_pos, unsigned int y_pos, unsigned char *string);
};




#endif
