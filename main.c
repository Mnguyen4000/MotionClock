#include <avr/io.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include <u8g2.h>
#include <u8x8_avr.h>
#include <i2cmaster.h>
#include <stdlib.h>
#include <stdio.h>
#include <uart.h>
#include <avr/interrupt.h>
#include <string.h>

#define ACCEL_ADDR 0x53 // 0x3A for Write, 0x3B for Read
#define TEMPHUMID_ADDR 0x5C // 0xB8 is (0x5C << 1)
#define CLOCK_ADDR 0x6F // 0b1101111

#define I2CREAD 1
#define I2CWRITE 0

#define HALFDAY 1
#define FULLDAY 0
#define AM 0
#define PM 1
#define DEACTIVATED 0
#define ARMED 1
#define ACTIVE 2

#define CLOCK_OFF 0
#define CLOCK_ON 1

#define FORECAST 1
#define CURRENT_TIME 2
#define AM_OR_PM 3
#define CELCIUS 4

#define UART_TX_BUFFER_SIZE 32
#define UART_RX_BUFFER_SIZE 32


u8g2_t u8g2;

 struct Reading{
	uint8_t celsius;
	uint8_t temperature;
	uint8_t humidity;
	short xAxis;
	short yAxis;
	short zAxis;
	uint8_t seconds; // ONLY 24 HOUR
	uint8_t minutes; // ONLY 24 HOUR
	uint8_t hours; // ONLY 24 HOUR
	uint8_t hourtype;
	uint8_t AmPm;
	uint8_t alarmmin; // ONLY 24 HOUR
	uint8_t alarmhour; // ONLY 24 HOUR
	uint8_t alarmAmPm; // ONLY 24 HOUR
	uint8_t alarmMask; // ONLY 24 HOUR
	uint8_t alarmstate;
};


void test(struct Reading *reading);
void receive_data(struct Reading *reading);
uint16_t read_temp(void);
void read_accel(struct Reading *reading);
void debug_temphumid(void);
void debug_accel(void);
void debug_clock(void);
char detect_mode(struct Reading reading);
void setup_accel (void);
void read_temp_and_humidity(struct Reading *reading);
void set_clock (struct Reading *reading);
void read_clock(struct Reading *reading);
void set_alarm (struct Reading reading);
void mode_A(struct Reading *reading);
void mode_C(struct Reading reading);
void transition_state(struct Reading *reading);
void clear_alarm_flags(void);
static uint8_t count = 0, times = 0;
void display_alarm_time(struct Reading *reading);

/*
Notes:
The clock values are always in 24 hour format, the 12 hour format in the IC is not used
when in 12 hour format, it is identified through software.


*/
int main (void)
{
	struct Reading reads;
	u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_avr_hw_i2c, u8x8_avr_delay);
	u8g2_InitDisplay(&u8g2);
	u8g2_SetPowerSave(&u8g2, 0);
	
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_6x13_tf );
	u8g2_SetFontRefHeightText(&u8g2);
	u8g2_SetFontPosTop(&u8g2);
	u8g2_DrawStr(&u8g2, 0, 20, "Initial Screen XD");
	u8g2_SendBuffer(&u8g2);
	
	// Initialising Default
	reads.xAxis = 99;
	reads.yAxis = 99;
	reads.zAxis = 99;
	
	reads.celsius = 1;
	
	reads.hours = 0x19;
	reads.minutes = 0x59;
	reads.seconds = 0x49;
	reads.hourtype = FULLDAY;
	reads.AmPm = PM;
	reads.alarmhour = 0x12;
	reads.alarmmin = 0x01;
	reads.alarmAmPm = PM;
	reads.alarmstate = DEACTIVATED;
	set_clock(&reads);
	set_alarm(reads);
	
	setup_accel();
	DDRD = 0xFF;
	DDRB |= (1<<0);
	char current_mode;
	uart_init(UART_BAUD_SELECT(9600, F_CPU));
	sei();
	int counting = 0;
	char sample_buffer[100];
	while(1) {
		read_accel(&reads);
		read_clock(&reads);
		current_mode = detect_mode(reads);

		
		if (current_mode != 'N') {
			switch (current_mode) {
				case 'A':
					mode_A(&reads);
					break;
				case 'B':
					//Mode_B();
					break;
				case 'C':
					mode_C(reads);
					break;
				case 'D':
					//mode_D();
					break;
				default:
					debug_temphumid();
			}
		}
		
		if (uart_available()) {
			receive_data(&reads);
		}
		//test(&reads);

	
		_delay_ms(20);
	}
}
void test(struct Reading *reading) {
	char junk[30];
	char str1[20];			
	
	sprintf(str1, "Received message");
	sprintf(junk, "T:%i", reading->hourtype);
	// Display data on OLED
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_unifont_t_symbols);
	u8g2_SetFontRefHeightText(&u8g2);
	u8g2_SetFontPosTop(&u8g2);
	u8g2_DrawStr(&u8g2, 0, 20,str1);// Low Humid
	u8g2_DrawStr(&u8g2, 0, 30,junk);// Low Humid
	u8g2_SendBuffer(&u8g2);
}

void receive_data(struct Reading *reading) {
	char buffer[16];
	char data;
	uint8_t result;
	for (int i = 0; i < 16; i++) {
		data = uart_getc();
		if (data == '\0') {
			buffer[i] = '\0';
			break;
		} else {
			buffer[i] = data;
		}
	}
	if (strcmp(buffer, "forecast") == 0 ) { // If the buffer and forecast match
		result = FORECAST;
		// Do the forecast function.
	} else if (strcmp(buffer, "12hour") == 0 ) {
		reading->hourtype = HALFDAY; // works
	} else if (strcmp(buffer, "24hour") == 0 ) {
		reading->hourtype = FULLDAY; // works
	} else if (strcmp(buffer, "clock") == 0) {
		// This gives the current clock information into the registers
	}
}

// COMPLETE
// Reads the temperature and humidity
// Returns the readings into the Reading Struct
void read_temp_and_humidity(struct Reading *reading) {
	unsigned char lowTemp, highTemp, lowHumid, highHumid;
	// Reading
	
	// To use the humidity sensor, you must awaken it, then it allows the acquisition of temp and humidity sensors
	// the entire process must happen within 3 seconds, due to it becoming inactive and must be awaken once more to start again
	// you can only acquire the information every 2 seconds!!!

	// Wake up the device
	i2c_start((TEMPHUMID_ADDR<<1) + I2C_WRITE); // set device address and write mode
	_delay_ms(1);
	i2c_stop();
	
	// Once awaken, send read commands
	if (i2c_start((TEMPHUMID_ADDR<<1)+I2C_WRITE)) { // set device address and write mode
		PORTD &= ~(1<<7); // Slave did not respond
		} else {
		PORTD |= (1<<7); // Slave found
		
		i2c_write(0x03);	// Function code 0x03
		i2c_write(0x00);	// Set Starting Register Address to 0x00
		i2c_write(0x04);	// Number of Registers to read
		i2c_stop();
		_delay_ms(2);
		i2c_rep_start((TEMPHUMID_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
		i2c_readAck();									// Byte one:	Function Code (0x03)
		i2c_readAck();									// Byte Two:	Data Length (0x04)
		highHumid = i2c_readAck();						// Byte Three:	High Humidity reading
		lowHumid = i2c_readAck();						// Byte Four:	Low Humidity reading
		highTemp = i2c_readAck();						// Byte Five:	High Temperature reading
		lowTemp = i2c_readAck();						// Byte Six:	Low Temperature reading
		i2c_readAck();									// Byte Seven: CRC Most Significant Byte
		i2c_readNak();									// Byte Eight: CRC Least Significant Byte
			
		_delay_ms(1);
		i2c_stop();
			
		//Calculates actual temperature and humidity from readings
		int temperatureCalc = (highTemp * 256) + ((lowTemp >> 4)* 16) + (lowTemp & 0x0F);
		int temperature = temperatureCalc/10;
		int humidityCalc = (highHumid * 256) + ((lowHumid >> 4)* 16) + (lowHumid & 0x0F);
		int humidity = humidityCalc/10;
			
		// Update readings into struct
		reading->humidity = humidity;
		reading->temperature = temperature;
	}     
}

// COMPLETE
// Displays the statistics for the temperature and humidity module on the OLED Display
void debug_temphumid(void) {
	unsigned char lowTemp, highTemp, lowHumid, highHumid;
	// Reading
	
	// To use the humidity sensor, you must awaken it, then it allows the acquisition of temp and humidity sensors
	// the entire process must happen within 3 seconds, due to it becoming inactive and must be awaken once more to start again
	// you can only acquire the information every 2 seconds!!!

	// Wake up the device
	i2c_start((TEMPHUMID_ADDR<<1) + I2C_WRITE);
	_delay_ms(1);
	i2c_stop();
	
	// Once awaken, send read commands
	if (i2c_start((TEMPHUMID_ADDR<<1)+I2C_WRITE)){
		PORTD &= ~(1<<7);
		} else {
		PORTD |= (1<<7);
	}     // set device address and write mode
	
	i2c_write(0x03);	// Function code 0x03
	i2c_write(0x00);	// Set Starting Register Address to 0x00
	i2c_write(0x04);	// Number of Registers to read
	i2c_stop();
	_delay_ms(2);
	i2c_rep_start((TEMPHUMID_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode		
	i2c_readAck();									// Byte one:	Function Code (0x03)
	i2c_readAck();									// Byte Two:	Data Length (0x04)
	highHumid = i2c_readAck();						// Byte Three:	High Humidity reading
	lowHumid = i2c_readAck();						// Byte Four:	Low Humidity reading
	highTemp = i2c_readAck();						// Byte Five:	High Temperature reading
	lowTemp = i2c_readAck();						// Byte Six:	Low Temperature reading
	i2c_readAck();									// Byte Seven: CRC Most Significant Byte 
	i2c_readNak();									// Byte Eight: CRC Least Significant Byte
	
	_delay_ms(1);
	i2c_stop();
	
	
	//Calculates actual temperature and humidity from readings
	int temperatureCalc = (highTemp * 256) + ((lowTemp >> 4)* 16) + (lowTemp & 0x0F);
	int temperature = temperatureCalc/10;
	int humidityCalc = (highHumid * 256) + ((lowHumid >> 4)* 16) + (lowHumid & 0x0F);
	int humidity = humidityCalc/10;
			
	// Create buffers to place integer to strings
	char str1[30];
	char str2[30];
	char str3[30];
	char str4[30];
	char str5[30];

	sprintf(str1, "lowHumid:\t %i", lowHumid);
	sprintf(str2, "highHumid:\t%i", highHumid);
	sprintf(str3, "lowTemp:\t %i", lowTemp);
	sprintf(str4, "highTemp:\t %i", highTemp);
	sprintf(str5, "T: %i, H: %i", temperature, humidity);
			
	// Display data on OLED
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_smart_patrol_nbp_tr);
	u8g2_SetFontRefHeightText(&u8g2);
	u8g2_SetFontPosTop(&u8g2);
	u8g2_DrawStr(&u8g2, 0, 10,str1); // Low Humid
	u8g2_DrawStr(&u8g2, 0, 20,str2); // High Humid
	u8g2_DrawStr(&u8g2, 0, 30,str3); // Low Temperature
	u8g2_DrawStr(&u8g2, 0, 40,str4); // High Temperature
	u8g2_DrawStr(&u8g2, 0, 50,str5); // Temperature and Humidity
	u8g2_SendBuffer(&u8g2);
}

// INCOMPLETE
// Initialise the accelerometer
void setup_accel (void) {
	// Disable measurement register
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);
	i2c_write(0x2D);	// Function code 0x03
	i2c_write(0x00);	// Disable Measurement
	i2c_stop();
		
	//bypass fifo
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x38);	// Function code 0x03
	i2c_write(0x00);	// Function code 0x03
	i2c_stop();
		
		
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);
	i2c_write(0x1D);	// THRESHOLD FOR TAP
	i2c_write(0x8F);	// THRESHOLD IS HALF
	i2c_stop();
	
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);
	i2c_write(0x2A);	// Tap Detection Axes
	i2c_write(0x07);	// Enable all Axes detection
	i2c_stop();
		
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);
	i2c_write(0x21);	// Duration Register
	i2c_write(0x10);	// Duration is half
	i2c_stop();
		
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);
	i2c_write(0x22);	// Latency Register
	i2c_write(0x10);	// Latency is half
	i2c_stop();
		
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);
	i2c_write(0x23);	// THRESHOLD FOR TAP
	i2c_write(0xFF);	// Window is half
	i2c_stop();
		
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);
	i2c_write(0x2E);	// Set Interrupt Mapping
	i2c_write((1<<6) | (1<<5));	// Single Tap and Double Tap Enabled
	i2c_stop();
		
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);
	i2c_write(0x2F);	// Set Interrupt Mapping
	i2c_write(1<<5);	// Pins -> int1: Single Tap and int2: Double Tap Enabled
	i2c_stop();
	
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);
	i2c_write(0x2D);	// Function code 0x03
	i2c_write((1<<3));	// Enable Measurement
	i2c_stop();
}

// COMPLETE
// Reads the X, Y, Z Axis onto the Reading Struct
void read_accel(struct Reading *reading) {
	short xAxis, yAxis, zAxis;
	char xAxisMSB, xAxisLSB, yAxisMSB, yAxisLSB, zAxisMSB, zAxisLSB;
	
	// Enable Measurement bit
	if (i2c_start((ACCEL_ADDR<<1)+I2C_WRITE)){// set device address and write mode
		PORTB &= ~(1<<0);
		} else {
		PORTB |= (1<<0);
	}
		
	i2c_write(0x2D);	// Function code 0x03
	i2c_write(0b00001000);	// Function code 0x03
	i2c_stop();
	
	// Write Data Format
	if (i2c_start((ACCEL_ADDR<<1)+I2C_WRITE)){// set device address and write mode
		PORTB &= ~(1<<0);
	} else {
		PORTB |= (1<<0);	
	}     
	
	i2c_write(0x31);	// Function code 0x03
	i2c_write(0b00001011); // Enable 16g
	i2c_stop();


	// X Axis LSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x32);	// Function code 0x03
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	xAxisLSB = i2c_readNak();									// Byte one:
	i2c_stop();
	// X Axis MSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x33);	// Function code 0x03
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	xAxisMSB = i2c_readNak();									// Byte one:	
	i2c_stop();
	
	// Y Axis LSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x34);	// Function code 0x03
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	//i2c_readAck();
	yAxisLSB = i2c_readNak();									// Byte one:	
	i2c_stop();
	// Y Axis MSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x35);	// Function code 0x03
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	//i2c_readAck();
	yAxisMSB = i2c_readNak();									// Byte one:
	i2c_stop();
	
	// Z Axis LSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE); // set device address and write mode
	i2c_write(0x36);	// Function code 0x03
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	//i2c_readAck();
	zAxisLSB = i2c_readNak();									// Byte one:
	i2c_stop();	
	// Z Axis LSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE); // set device address and write mode
	i2c_write(0x37);	// Function code 0x03
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	//i2c_readAck();
	zAxisMSB = i2c_readNak();									// Byte one:
	i2c_stop();
	
	xAxis = (xAxisMSB << 8) | xAxisLSB;
	yAxis = (yAxisMSB << 8) | yAxisLSB;
	zAxis = (zAxisMSB << 8) | zAxisLSB;
	
	reading->xAxis = xAxis;
	reading->yAxis = yAxis;
	reading->zAxis = zAxis;
}

// COMPLETE
// Displays the X, Y, Z Axis on th OLED
void debug_accel(void) {
	short xAxis, yAxis, zAxis;
	char xAxisMSB, xAxisLSB, yAxisMSB, yAxisLSB, zAxisMSB, zAxisLSB;
	
	//bypass fifo
	if (i2c_start((ACCEL_ADDR<<1)+I2C_WRITE)){// set device address and write mode
		PORTB &= ~(1<<0);
		} else {
		PORTB |= (1<<0);
	}
	
	i2c_write(0x38);	// Function code 0x03
	i2c_write(0x00);	// Function code 0x03
	i2c_stop();
	
	
	if (i2c_start((ACCEL_ADDR<<1)+I2C_WRITE)){// set device address and write mode
		PORTB &= ~(1<<0);
		} else {
		PORTB |= (1<<0);
	}
	
	i2c_write(0x2D);	// Function code 0x03
	i2c_write(0b00001000);	// Function code 0x03
	i2c_stop();
	
	if (i2c_start((ACCEL_ADDR<<1)+I2C_WRITE)){// set device address and write mode
		PORTB &= ~(1<<0);
		} else {
		PORTB |= (1<<0);
	}
	
	i2c_write(0x31);	// Function code 0x03
	i2c_write(0b00001010);
	i2c_stop();


	// X Axis LSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x32);	// Function code 0x03
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	xAxisLSB = i2c_readNak();									// Byte one:
	i2c_stop();
	// X Axis MSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x33);	// Function code 0x03
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	xAxisMSB = i2c_readNak();									// Byte one:
	i2c_stop();
	
	// Y Axis LSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x34);	// Function code 0x03
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	//i2c_readAck();
	yAxisLSB = i2c_readNak();									// Byte one:
	i2c_stop();
	// Y Axis MSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x35);	// Function code 0x03
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	//i2c_readAck();
	yAxisMSB = i2c_readNak();									// Byte one:
	i2c_stop();
	
	// Z Axis LSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE); // set device address and write mode
	i2c_write(0x36);	// Function code 0x03
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	//i2c_readAck();
	zAxisLSB = i2c_readNak();									// Byte one:
	i2c_stop();
	// Z Axis LSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE); // set device address and write mode
	i2c_write(0x37);	// Function code 0x03
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	//i2c_readAck();
	zAxisMSB = i2c_readNak();									// Byte one:
	i2c_stop();
	
	xAxis = (xAxisMSB << 8) | xAxisLSB;
	yAxis = (yAxisMSB << 8) | yAxisLSB;
	zAxis = (zAxisMSB << 8) | zAxisLSB;
	
	char str1[20];
	char str2[20];
	char str3[20];
	sprintf(str1, "x: %i", xAxis);
	sprintf(str2, "y: %i", yAxis);
	sprintf(str3, "z: %i", zAxis);
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_smart_patrol_nbp_tr);
	u8g2_SetFontRefHeightText(&u8g2);
	u8g2_SetFontPosTop(&u8g2);
	u8g2_DrawStr(&u8g2, 0, 10, str1);
	u8g2_DrawStr(&u8g2, 0, 20, str2);
	u8g2_DrawStr(&u8g2, 0, 30, str3);
	u8g2_SendBuffer(&u8g2);
}

// COMPLETE
/*
	Sets the clock times onto the MCP790N
	Argument hourMode: 1 -> 12 hour, 0 -> 24 Hour
	startClock: 0 -> Clock is OFF, 1 -> Clock is ON
*/
void set_clock (struct Reading *reading) {
	char setSeconds, setMinutes, setHours;
	
	// Set the start bit for the clock + import seconds
	setSeconds = (1<<7) | reading->seconds; 

	setMinutes = reading->minutes;
	// BitMasked for 24 Hour Format
	setHours = 0x3F & reading->hours; 

	
	// Write the seconds
	if (i2c_start((CLOCK_ADDR<<1)+I2C_WRITE)){// set device address and write mode
		PORTB &= ~(1<<0);
		} else {
		PORTB |= (1<<0);
	}
		
	i2c_write(0x00);	// Function code: seconds
	i2c_write(setSeconds);	// Function code 
	i2c_stop();
	
	// Write the minutes
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x01);	// Function code 0x03
	i2c_write(setMinutes);	// Function code 0x03
	i2c_stop();	
	
	// Write the hours
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x02);	// Function code 0x03
	i2c_write(setHours);	// Function code 0x03
	i2c_stop();
	
}

// COMPLETE
// Reads the clock times onto the Reading Struct
// Converts Hexadecimal to integer values
void read_clock(struct Reading *reading) {
	// Read the seconds
	uint8_t readSeconds, readMinutes, readHours, scannedHours;
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x00);	// Function code 0x00
	i2c_stop();
	i2c_rep_start((CLOCK_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	readSeconds = ~(1<<7) & i2c_readNak();									// Byte one:
	i2c_stop();
	
	// Read the minutes
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x01);	// Function code 0x01
	i2c_stop();
	i2c_rep_start((CLOCK_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	readMinutes = i2c_readNak();									// Byte one:
	i2c_stop();
	
	// Read the hours
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x02);	// Function code 0x02
	i2c_stop();
	i2c_rep_start((CLOCK_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	scannedHours = i2c_readNak();									
	i2c_stop();
	
	// Mask the values
	readHours = 0x3F & scannedHours;
	
	reading->seconds = readSeconds;
	reading->minutes = readMinutes;
	reading->hours = readHours;
}

// Reads GPIO pins and transitions state
// Only able to transition in Mode A
void transition_state(struct Reading *reading) {
	// Armed -> Active
	if ((PINC & (1<<1)) == 0 && reading->alarmstate == ARMED) { // Clock Alarm
		reading->alarmstate = ACTIVE;
	}
	if (PINC & (1<<0)) { // If double tap detected.
		
		if (reading->alarmstate == DEACTIVATED) { // Deactivated -> Armed
			reading->alarmstate = ARMED;
		} else if (reading->alarmstate == ARMED && (PINC & (1<<1))) { // Armed -> Deactivated
			// if State: ARMED but Alarm is not active, switch back to deactivated
			reading->alarmstate = DEACTIVATED;
		} else if (reading->alarmstate == ACTIVE) { // Activated -> Deactivated
			reading->alarmstate = DEACTIVATED;
			clear_alarm_flags();
		}
	}
	// Clear interrupt
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x30);	// Function code 0x00
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	i2c_readNak();								// Clears the interrupt
	i2c_stop();
}

void debug_clock(void) {
	// Read the seconds
	uint8_t readSeconds, readMinutes, readHours, seconds, minutes, hours;
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x00);	// Function code 0x00
	i2c_stop();
	i2c_rep_start((CLOCK_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	readSeconds = ~(1<<7) & i2c_readNak();									// Byte one:
	i2c_stop();
	
	// Read the minutes
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x01);	// Function code 0x01
	i2c_stop();
	i2c_rep_start((CLOCK_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	readMinutes = i2c_readNak();									// Byte one:
	i2c_stop();
	
	// Read the hours
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x02);	// Function code 0x02
	i2c_stop();
	i2c_rep_start((CLOCK_ADDR<<1)+I2C_READ);    // Set device	Address and Read Mode
	readHours = i2c_readNak();
	i2c_stop();
	
	seconds = (readSeconds>>4) * 10 + (readSeconds & 0x0F);
	minutes = (readMinutes>>4) * 10 + (readMinutes & 0x0F);
	hours = (readHours>>4) * 10 + (readHours & 0x0F);
		
	char buff1[20];
	char buff2[20];
	char buff3[20];
	sprintf(buff1, "seconds: %i", seconds);
	sprintf(buff2, "minutes: %i", minutes);
	sprintf(buff3, "hours: %i", hours);
			
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_smart_patrol_nbp_tr);
	u8g2_SetFontRefHeightText(&u8g2);
	u8g2_SetFontPosTop(&u8g2);
	u8g2_DrawStr(&u8g2, 0, 10, buff1);
	u8g2_DrawStr(&u8g2, 0, 20, buff2);
	u8g2_DrawStr(&u8g2, 0, 30, buff3);
	u8g2_SendBuffer(&u8g2);
}

//COMPLETE
// Returns 'A', 'B', 'C', 'D', or 'N' Depending on what mode its in
char detect_mode(struct Reading reading) {
	char result;
	if (reading.xAxis < 50 && reading.xAxis > -50 && reading.yAxis < -200) {
		result = 'A';
	} else if (reading.yAxis < 50 && reading.yAxis > -50 && reading.xAxis > 200) {
		result = 'B';
	} else if (reading.xAxis < 50 && reading.xAxis > -50 && reading.yAxis > 200) {
		result = 'C';
	} else if (reading.yAxis < 50 && reading.yAxis > -50 && reading.xAxis < -200) {
		result = 'D';
	} else {
		result = 'N'; // Error Code N: Transition State, not any discrete part.
	}
	return result;
}


void clear_alarm_flags(void) {
	// Set Alarm Masks
	// Alarm 0, triggers if hours match
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x0D);	// Function code 0x03
	i2c_write(0x20);	// Alarm Mask for hours
	i2c_stop();
	// Alarm 1, triggers if minutes match
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x14);	// Function code 0x03
	i2c_write(0x10);	// Alarm Mask for minutes
	i2c_stop();
		
}

void set_alarm (struct Reading reading) {
	uint8_t alarmMinutes, alarmHours;
	
	// Set alarm minutes
	alarmMinutes = ~(1<<7) & reading.alarmmin;
	

	if (reading.hourtype) {
		if (reading.alarmhour > 0x12) {
			 reading.alarmhour -= 0x12;
		}
		// BitMasked to Include 12 hour bit
		alarmHours = (reading.hourtype<<6) |(reading.alarmAmPm << 5)| reading.alarmhour;
	} else {
		// Commented out part which compensates AM/PM connotation
		/*
		if (reading.alarmhour < 0x12 && reading.alarmAmPm == PM) {
			reading.alarmhour += 0x12;
		}
		*/
		alarmHours = 0x3F & reading.alarmhour; // BitMasked for 24 Hour Format
	}
	
	// Set Alarm Masks
	// Alarm 0, triggers if hours match
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x0D);	// Function code 0x03
	i2c_write(0x20);	// Alarm Mask for hours
	i2c_stop();
	// Alarm 1, triggers if minutes match
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x14);	// Function code 0x03
	i2c_write(0x10);	// Alarm Mask for minutes
	i2c_stop();
		
	


	// Write the hours
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x0C);	// Function code 0x03
	i2c_write(alarmHours);	
	i2c_stop();		

	// Write the minutes
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x12);	// Function code 0x03
	i2c_write(alarmMinutes);	
	i2c_stop();


	// Set Control Registers
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x07);	// Function code 0x03
	i2c_write((1<<5)|(1<<4));	// Enable single alarm interrupt (ALM0EN) and external crystal source
	i2c_stop();
	
}

void alarm_active(void) {
	count += 1; // count goes up every 20ms
	if (count <= 14) { // 700ms
		PORTD |= (1<<6);
		} else if (count > 14 && count <= 20){
		PORTD &= ~(1<<6); // 300ms
	}
}

void display_alarm_time(struct Reading *reading) {
	uint8_t alarmHourCopy;
	uint8_t mTen, mOne, hTen, hOne;
	
	// If hourtype is half day but the format is full day (>12), it will fix it for it.
	if (reading->hourtype == FULLDAY) {
		alarmHourCopy = reading->alarmhour & 0x3F;
	} else if (reading->hourtype == HALFDAY && reading->alarmhour > 0x12) {
		alarmHourCopy = reading->alarmhour - 0x12;
		reading->alarmAmPm = PM;
	} else {
		alarmHourCopy = reading->alarmhour;
	}
	
	// Write the clock
	mTen = (reading->alarmmin>>4);
	mOne = (reading->alarmmin & 0x0F);
	hTen = (alarmHourCopy >> 4);
	hOne = alarmHourCopy & 0x0F;
	char buffer[20];
	// Handles leading 0 on hour
	if (hTen) {
		sprintf(buffer,"Alarm:%i%i:%i%i:00",  hTen, hOne, mTen, mOne);
		} else {
		sprintf(buffer,"Alarm: %i:%i%i:00", hOne, mTen, mOne);
	}
	u8g2_SetFont(&u8g2, u8g2_font_6x13_tf );
	u8g2_SetFontRefHeightText(&u8g2);
	u8g2_SetFontPosTop(&u8g2);
	if (reading->hourtype) {
		if (reading->alarmAmPm) { //0 -> AM, 1-> PM
			u8g2_DrawStr(&u8g2, 85, 0, "PM");
			} else {
			u8g2_DrawStr(&u8g2, 85, 0, "AM");
		}
	}
	u8g2_DrawStr(&u8g2, 0, 0, buffer);
}
void mode_A(struct Reading *reading) {
	int sTen, sOne, mTen, mOne, hTen, hOne;
	uint8_t seconds, minutes, hours;
	char buffer[50];
	
	// Reset counter if it gets too high
	if (count >= 20) {
		count = 0;
	}
	seconds = (reading->seconds>>4) * 10 + (reading->seconds & 0x0F);
	minutes = (reading->minutes>>4) * 10 + (reading->minutes & 0x0F);
	hours = (reading->hours>>4) * 10 + (reading->hours & 0x0F);
	// Write the clock
	sTen = seconds / 10;
	sOne = seconds- sTen*10;
	mTen = minutes / 10;
	mOne = minutes - mTen*10;
	hTen = hours / 10;
	hOne = hours - hTen*10;
	


	// Handles leading 0 on hour
	if (hTen) {
		sprintf(buffer,"%i%i:%i%i:%i%i",  hTen, hOne, mTen, mOne, sTen, sOne);
	} else {
		sprintf(buffer," %i:%i%i:%i%i", hOne, mTen, mOne, sTen, sOne);
	}
	
	transition_state(reading);
	
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_t0_22_tf   );
	u8g2_SetFontRefHeightText(&u8g2);
	u8g2_SetFontPosTop(&u8g2);
	if (reading->hourtype) {
		if (read_AM_PM()) {
			u8g2_DrawStr(&u8g2, 100, 30, "PM");
		} else {
			u8g2_DrawStr(&u8g2, 100, 30, "AM");
		}
	}
	if (reading->alarmstate == DEACTIVATED) {
		PORTD &= ~(1<<6);  // Turn off buzzer
	} else if (reading->alarmstate == ARMED) {
		PORTD &= ~(1<<6); // Turn off buzzer
		u8g2_SetFont(&u8g2, u8g2_font_unifont_t_symbols);
		u8g2_DrawGlyph(&u8g2, 100, 0, 0x23f0);
	} else if (reading->alarmstate == ACTIVE) {
		alarm_active();
		if (count <= 10) {
			u8g2_SetFont(&u8g2, u8g2_font_unifont_t_symbols);
			u8g2_DrawGlyph(&u8g2, 110, 0, 0x23f0);
		}
	}
	u8g2_SetFont(&u8g2, u8g2_font_t0_22_tf   );
	u8g2_SetFontRefHeightText(&u8g2);
	u8g2_SetFontPosTop(&u8g2);
	u8g2_DrawStr(&u8g2, 10, 30, buffer);
	
	display_alarm_time(reading);

	u8g2_SendBuffer(&u8g2);
}

void mode_B(void) {
	// Mode B is the same fahreinheight/Celsius as Mode C
	
}

void mode_C(struct Reading reading) {
	// Create buffers to place integer to strings
	char str1[30];
	char str2[30];
	char unit1[10];
	char unit2[10];

	if (reading.celsius) {
		sprintf(str1, "Temperature:  %i", reading.temperature);
		sprintf(unit1, " %c%c", 0xB0, 'C');
	} else {
		int fahrenheit = (reading.temperature * 9/5) + 35;
		sprintf(str1, "Temperature:  %i", fahrenheit);
		sprintf(unit1, " %c%c", 0xB0, 'F');
	}
	sprintf(str2, "Humidity:     %i",reading.humidity);
	
	sprintf(unit2, "  %c", '%');

		
	// Display data on OLED
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetDisplayRotation(&u8g2, U8G2_R2);
	u8g2_SetFont(&u8g2, u8g2_font_6x13_tf );
	u8g2_SetFontRefHeightText(&u8g2);
	u8g2_SetFontPosTop(&u8g2);
	u8g2_DrawStr(&u8g2, 0, 20,str1); // Temperature
	
	u8g2_DrawStr(&u8g2, 100, 20,unit1); // Temperature
	u8g2_DrawStr(&u8g2, 0, 40,str2); // Humidity
	
	u8g2_DrawStr(&u8g2, 100, 40,unit2); // Humidity

	u8g2_SendBuffer(&u8g2);
	

}

void mode_D(void) {
	// Low power mode
	// Display doesn't display, so have it be blank
	// Set possible devices to low power mode
	// aceelerometer must still work to register change back
	// Stuff to disable: Temp/humid ic, oled
}
