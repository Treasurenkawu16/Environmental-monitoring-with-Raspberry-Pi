/*  adcreader.cpp
*   This program sets the initialisation of the MCP3424 ADC.
*   To run this:
*   Required package{
*   apt-get install libi2c-dev
*/

#include "adcreader.h"
#include "circularbuffer.h"


#include <QDebug>

#include <stdio.h>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>

using namespace std;
//constructor

//variables definations
int i2cbus;
const char *fileName = "/dev/i2c-1"; // change to /dev/i2c-0 if you are using a revision 0002 or 0003 model B
unsigned char writebuffer[10] = { 0 };
unsigned char readbuffer[10] = { 0 };
char signbit = 0;

  //sensor buffers, need to be accessable to window.cpp so put extern in front
Circularbuffer tempbuffer;	
Circularbuffer humbuffer;
Circularbuffer lightbuffer;



//functions implementations
void ADCreader::run()
{
  //function variables
  //double temp, temp_volt,hum, hum_volt,light, light_volt ;
  
	running = true;
	//ADCreader adcreader;

	if(!tempbuffer.Full()){tempbuffer.Insert((read_voltage(0x68,2, 12, 1,1) - 0.621)/0.01);}
	if(!humbuffer.Full()){humbuffer.Insert((0.826-(read_voltage(0x68,3, 12, 1,1)))/0.04);}
	if(!lightbuffer.Full()){lightbuffer.Insert((read_voltage(0x68,1, 12, 1,1)/1.98)*100);}	
	
	while (running) {

	  if(!tempbuffer.Full()){tempbuffer.Insert((read_voltage(0x68,2, 12, 1,1) - 0.621)/0.01);}
	  if(!humbuffer.Full()){humbuffer.Insert((0.826-(read_voltage(0x68,3, 12, 1,1)))/0.04);}
	  if(!lightbuffer.Full()){lightbuffer.Insert((read_voltage(0x68,1, 12, 1,1)/1.98)*100);}


	  
	  //sleep(1);
       	}
}

void ADCreader::quit()
{
	running = false;
	exit(0);
}

void ADCreader::read_byte_array(char address, char reg, char length) {
  //fopen() when you want to write portable code
  int fd = open(fileName, O_RDWR); //fd = file descriptor
  

	if ((i2cbus = open(fileName, O_RDWR)) < 0) {
	  cout << "file pointer: %d" << fd;
		cout << "Failed to open i2c port for read %s \n", strerror(errno);

		exit(1);
	}

	if (ioctl(i2cbus, I2C_SLAVE, address) < 0) {
		cout << "Failed to write to i2c port for read\n";
		exit(1);
	}

	writebuffer[0] = reg;

	if ((write(i2cbus, writebuffer, 1)) != 1) {
		cout << "Failed to write to i2c device for read\n";
		exit(1);
	}

	read(i2cbus, readbuffer, 4);

	close(i2cbus);
}

char ADCreader::update_byte(char byte, char bit, char value) {
	/*
	 internal method for setting the value of a single bit within a byte
	 */
	if (value == 0) {
		return (byte &= ~(1 << bit));

	} else {
		return (byte |= 1 << bit);
	}

}

char ADCreader::set_pga(char config, char gain) {
	/*
	 internal method for Programmable Gain Amplifier gain selection
	 */
	switch (gain) {
	case 1:
		config = update_byte(config, 0, 0);
		config = update_byte(config, 1, 0);
		break;
	case 2:
		config = update_byte(config, 0, 1);
		config = update_byte(config, 1, 0);
		break;
	case 4:
		config = update_byte(config, 0, 0);
		config = update_byte(config, 1, 1);
		break;
	case 8:
		config = update_byte(config, 0, 1);
		config = update_byte(config, 1, 1);
		break;
	default:
		break;
	}
	return (config);
}

char ADCreader::set_bit_rate(char config, char rate) {
	/*
	 internal method for bit rate selection
	 */
	switch (rate) {
	case 12:
		config = update_byte(config, 2, 0);
		config = update_byte(config, 3, 0);
		break;
	case 14:
		config = update_byte(config, 2, 1);
		config = update_byte(config, 3, 0);
		break;
	case 16:
		config = update_byte(config, 2, 0);
		config = update_byte(config, 3, 1);

		break;
	case 18:
		config = update_byte(config, 2, 1);
		config = update_byte(config, 3, 1);

		break;
	default:
		break;
	}
	return (config);
}

char ADCreader::set_conversion_mode(char config, char mode) {
	/*
	 internal method for setting the conversion mode
	 */
	if (mode == 1) {
		config = update_byte(config, 4, 1);
	} else {
		config = update_byte(config, 4, 0);
	}

	return (config);
}

char ADCreader::set_channel(char config, char channel) {
	/*
	 internal method for setting the channel
	 */
	switch (channel) {
	case 1:
		config = update_byte(config, 5, 0);
		config = update_byte(config, 6, 0);
		break;
	case 2:
		config = update_byte(config, 5, 1);
		config = update_byte(config, 6, 0);
		break;
	case 3:
		config = update_byte(config, 5, 0);
		config = update_byte(config, 6, 1);
		break;
	case 4:
		config = update_byte(config, 5, 1);
		config = update_byte(config, 6, 1);
		break;
	}

	return (config);
}

/// <summary>
/// Reads the raw value from the selected ADC channel
/// </summary>
/// <param name="address">I2C Address e.g.  0x68</param>
/// <param name="channel">1 to 4</param>
/// <param name="bitrate">12, 14, 16 or 18</param>
/// <param name="pga">1, 2, 4 or 8</param>
/// <param name="conversionmode">0 = one shot conversion, 1 = continuous conversion</param>
/// <returns>raw long value from ADC buffer</returns>
int ADCreader::read_raw(char address, char channel, int bitrate, int pga,
		char conversionmode) {
	// variables for storing the raw bytes from the ADC
	char h = 0;
	char l = 0;
	char m = 0;
	char s = 0;
	char config = 0x9C;
	long t = 0;
	signbit = 0;

	// set the config based on the provided parameters
	config = set_channel(config, channel);
	config = set_conversion_mode(config, conversionmode);
	config = set_bit_rate(config, bitrate);
	config = set_pga(config, pga);

	// keep reading the ADC data until the conversion result is ready
	int timeout = 1000; // number of reads before a timeout occurs
	int x = 0;

	do {
		if (bitrate == 18) {
			read_byte_array(address, config, 3);
			h = readbuffer[0];
			m = readbuffer[1];
			l = readbuffer[2];
			s = readbuffer[3];
		} else {
			read_byte_array(address, config, 2);
			h = readbuffer[0];
			m = readbuffer[1];
			s = readbuffer[2];
		}

		// check bit 7 of s to see if the conversion result is ready
		if (!(s & (1 << 7))) {
			break;
		}

		if (x > timeout) {
			// timeout occurred
			return (0);
		}

		x++;
	} while (1);

	// extract the returned bytes and combine in the correct order
	switch (bitrate) {
	case 18:
		t = ((h & 3) << 16) | (m << 8) | l;
		if ((t >> 17) & 1) {
			signbit = 1;
			t &= ~(1 << 17);
		}
		break;
	case 16:
		t = (h << 8) | m;
		if ((t >> 15) & 1) {
			signbit = 1;
			t &= ~(1 << 15);
		}
		break;
	case 14:
		t = ((h & 63) << 8) | m;
		if ((t >> 13) & 1) {
			signbit = 1;
			t &= ~(1 << 13);
		}
		break;
	case 12:
		t = ((h & 15) << 8) | m;
		if ((t >> 11) & 1) {
			signbit = 1;
			t &= ~(1 << 11);
		}
		break;
	default:
		break;
	}

	return (t);
}

/// <summary>
/// Returns the voltage from the selected ADC channel
/// </summary>
/// <param name="address">I2C Address e.g.  0x68</param>
/// <param name="channel">1 to 4</param>
/// <param name="bitrate">12, 14, 16 or 18</param>
/// <param name="pga">1, 2, 4 or 8</param>
/// <param name="conversionmode">0 = one shot conversion, 1 = continuous conversion</param>
/// <returns>double voltage value from ADC</returns>
double ADCreader::read_voltage(char address, char channel, int bitrate, int pga,
		char conversionmode) {
	int raw = read_raw(address, channel, bitrate, pga, conversionmode); // get the raw value

	// calculate the gain based on the pga value
	double gain = (double) pga / 2;
	double offset = 2.048 / (double) pga;

	// set the lsb value based on the bitrate
	double lsb = 0;

	switch (bitrate) {
	case 12:
		lsb = 0.0005;
		break;
	case 14:
		lsb = 0.000125;
		break;
	case 16:
		lsb = 0.00003125;
		break;
	case 18:
		lsb = 0.0000078125;
		break;
	default:
		return (9999);
		break;
	}

	if (signbit == 1) // if the signbit is 1 convert it back to positive and subtract 2.048.
			{
		double voltage = (double) raw * (lsb / gain) - offset; // calculate the voltage and return it
		return (voltage);
	} else {
		double voltage = (double) raw * (lsb / gain); // calculate the voltage and return it
		return (voltage);
	}
}
