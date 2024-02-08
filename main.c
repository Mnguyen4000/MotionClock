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

#define ACCEL_ADDR 0x53
#define TEMPHUMID_ADDR 0x5C
#define CLOCK_ADDR 0x6F
#define SSD1306 0x78

#define I2CREAD 1
#define I2CWRITE 0

#define HALFDAY 1
#define FULLDAY 0
#define AM 0
#define PM 1
#define DEACTIVATED 0
#define ARMED 1
#define ACTIVE 2
#define DISCONNECTED 0
#define CONNECTEDCOOLDOWN 1
#define CONNECTED 2
#define DEFAULTAXIS 99
#define INIT 0
#define OFF 0
#define ON 1

#define MON 0
#define TUE 1
#define WED 2
#define THU	3
#define FRI 4
#define SAT 5
#define SUN 6

u8g2_t u8g2;

struct Reading{
	uint8_t celsius; 	// 0 -> fahrenheit, 1-> celcius
	uint8_t temperature;
	uint8_t humidity;
	short xAxis;
	short yAxis;
	short zAxis;
	uint8_t seconds; 	// ONLY 24 HOUR in HEX
	uint8_t minutes; 	// ONLY 24 HOUR in HEX
	uint8_t hours; 		// ONLY 24 HOUR in HEX
	uint8_t day; 		// 0x00 to 0x30/0x31
	uint8_t month; 		// 0x00 to 0x12
	uint8_t year; 		// Hexadecimal of the last 2 digits of the year 0x00 to 0x99
	uint8_t weekday; 	// MON -> 0,..., SUN -> 6
	uint8_t hourtype;
	uint8_t AmPm;
	uint8_t alarmmin; 	// ONLY 24 HOUR in HEX
	uint8_t alarmhour; 	// ONLY 24 HOUR in HEX
	uint8_t alarmAmPm; 	// ONLY 24 HOUR in HEX
	uint8_t alarmstate;
	uint8_t lowpower;
	uint8_t connected; 	// 0 -> not connected, 1 -> pending another signal, 2 -> new signal of connection
};

struct Forecast{ 		// numbers in decimal format
	uint8_t year; 		// Last 2 digits: 2023 -> 23
	uint8_t month; 		// Decimal format: 0 - 255
	uint8_t day; 		// Decimal format: 0 - 255
	float temperature;
	float humidity;
	char status[8];
};

void sync_forecast(struct Forecast *forecast);
void receive_data(struct Reading *reading, struct Forecast *forecast);
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
static uint8_t staticCount = 0, alarm_count = 0;
void display_alarm_time(struct Reading *reading);
void update_clock(struct Reading *reading);
void mode_B(struct Reading *reading, struct Forecast *forecast);
void mode_D(struct Reading *reading);
void send_info(struct Reading *reading, struct Forecast *forecast);
char int_to_char(int number);
void change_alarm(struct Reading *reading);
void alarm_active(void);


/*
Notes:
The clock values are always in 24 hour format, the 12 hour format in the IC is not used
when in 12 hour format, it is identified through software.
*/


/*
 *	Initialises the readings and forecast and starts the main loop.
 */
int main (void)
{
	// Struct stores IC values
	struct Reading reads;
	struct Forecast fore[7];

	char current_mode;

	// Initialise SSD1306 Screen
	u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_avr_hw_i2c, u8x8_avr_delay);
	u8g2_SetI2CAddress(&u8g2, SSD1306);
	u8g2_InitDisplay(&u8g2);
	u8g2_SetPowerSave(&u8g2, OFF);
	
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_6x13_tr);
	u8g2_SetFontMode(&u8g2, 1);
	u8g2_SetDrawColor(&u8g2, 2);
	u8g2_SetFontRefHeightText(&u8g2);
	u8g2_SetFontPosTop(&u8g2);
	u8g2_DrawStr(&u8g2, 0, 20, "Initial Screen");
	u8g2_SendBuffer(&u8g2);
	
	// Initialising Default Values
	// Accelerometer
	reads.xAxis = DEFAULTAXIS;
	reads.yAxis = DEFAULTAXIS;
	reads.zAxis = DEFAULTAXIS;
	
	// Onboard Temp and Humid Sensor
	reads.celsius = INIT;
	reads.temperature = INIT;
	reads.humidity = INIT;
	
	// Clock IC
	reads.hours = INIT;
	reads.minutes = INIT;
	reads.seconds = INIT;
	reads.weekday = MON;
	reads.hourtype = FULLDAY;
	reads.alarmhour = INIT;
	reads.alarmmin = INIT;
	reads.alarmstate = DEACTIVATED;
	reads.connected = DISCONNECTED; 	// 2 -> Initial Connection, 1-> Cooldown Connection, 0 -> No Connection
	reads.lowpower = OFF; 	// 0 -> Low-power mode OFF, 1-> low-power mode ON
	
	set_clock(&reads);
	set_alarm(reads);
	setup_accel();
	
	// Setting GPIO Pins
	DDRD = 0xFF;
	DDRB |= (1<<0);
	
	// Initialise UART Protocol
	uart_init(UART_BAUD_SELECT(9600, F_CPU));
	sei();

	// Initialising values for weather forecast
	for(uint8_t a = 0; a < 7; a++){
		fore[a].day = INIT;
		fore[a].month = INIT;
		fore[a].year = INIT;
		fore[a].temperature = INIT;
		fore[a].humidity = INIT;
		strncpy(fore[a].status, "error", 6); // Initial Status is "Error"
	}

	uint8_t sendCount = 0, timeCount = 0, connectedCount = 0;

	// Start loop
	while(1) {
		// Turn off screen if in low power mode
		if (reads.lowpower == ON) {
			u8g2_SetPowerSave(&u8g2, 0);
			i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);// set device address and write mode
			i2c_write(0x2C);	// Function code 0x03
			i2c_write((1<<3));	// Sets speed for lowpower mode
			i2c_stop();
			reads.lowpower = OFF;
		}
		
		if (reads.lowpower == OFF) {
			// Continue reading clock values
			read_clock(&reads);
		}
		
		// Get readings from the accelerometer
		read_accel(&reads);
		current_mode = detect_mode(reads);

		// Orientation of the Clock
		if (current_mode != 'N') {
			switch (current_mode) {
				case 'A':
					mode_A(&reads);
					break;
				case 'B':
					mode_B(&reads, fore);
					break;
				case 'C':
					mode_C(reads);
					break;
				case 'D':
					reads.lowpower = ON;
					mode_D(&reads);
					break;
			}
		} else {
			// IF cannot detect orientation, default to mode_A()
			mode_A(&reads);
		}
		
		// Receive messages from Seeduino Xiao and Process them
		if (uart_available()) {
			receive_data(&reads, fore);
		}
		// Read Temp and humid in time intervals
		if (timeCount <= 40) {
			timeCount++;
		} else {
			read_temp_and_humidity(&reads);
			timeCount = 0;
		}
		
		// Send information to the SeeduinoXiao in intervals
		if (reads.connected) {
			if (sendCount == 5) {
				send_info(&reads, fore);
				PORTD ^= (1<<7);
				sendCount = 0;
				} else {
				sendCount++;
			}
		}

		// Starts timed buffer to allow it to ask if its still connected
		if (reads.connected == CONNECTED) {
			connectedCount = 20;
			reads.connected = CONNECTEDCOOLDOWN;
		} else if (reads.connected == CONNECTEDCOOLDOWN){
			connectedCount--;
		}

		if (connectedCount <= 0) {
			reads.connected = DISCONNECTED;
		}
		_delay_ms(50);
	}
}

/*
 *   Converts Hex Values to Decimal Values
 */
uint8_t hex_to_deci(uint8_t number) {
	uint8_t ten, one, deci;
	ten = (number >> 4) * 10;
	one = number & 0x0F;
	deci = ten + one;
	return deci;
}

/*
 * 	Sends the stored readings and forecast through to the SeeeduinoXiao
 *	The Seeeduino has decodes the strings whilst here encodes it.
 */
void send_info(struct Reading *reading, struct Forecast *forecast) {
	char stats[40];
	uint8_t current, decimalDay, t1, t2, t3, t4, t5, t6;
	
	// Accelerometer Readings and onboard Temp/Humid
	snprintf(stats, 40, "X:%hi+Y:%hi+Z:%hi+T:%i+H:%i\n", reading->xAxis, reading->yAxis, reading->zAxis, reading->temperature, reading->humidity);
	uart_puts(stats); // Sends to seeeduino
	
	// Current Time, Weekday and Hour Type
	snprintf(stats,40, "h:%i+m:%i+s:%i+w:%i+H:%i\n", hex_to_deci(reading->hours), hex_to_deci(reading->minutes), hex_to_deci(reading->seconds), reading->weekday, reading->hourtype);
	uart_puts(stats); // Sends to seeeduino
	
	// Current Alarm Hour, Min, State, Celsius Status
	snprintf(stats,40, "H:%i+M:%i+S:%i+C:%i\n", hex_to_deci(reading->alarmhour), hex_to_deci(reading->alarmmin), reading->alarmstate, reading->celsius);
	uart_puts(stats); // Sends to seeeduino

	// Scan to see if the day matches, gives the i index.
	for (uint8_t i = 0; i < 7; i++) {
		// If the day-date matches to current day, gather that information
		decimalDay = ((reading->day >> 4) * 10) + (reading->day & 0x0F);
		if (forecast[i].day == decimalDay) {
			current = i;
			break;
			} else {
			current = MON; // Default is MON
		}
	}
	
	// Convert temp readings to strings format to send to SeeeduinoXiao
	char sign;
	float tempReading = forecast[current].temperature;

	if (tempReading < 0) {
		sign = '-';
		tempReading = tempReading * -1;
		} else {
		sign = ' ';
	}

	t1 = tempReading;
	t2 = (tempReading - t1)*100;
	t3 = (tempReading - t1 - t2/100) * 1000;

	tempReading = forecast[current].humidity;
	t4 = tempReading;
	t5 = (tempReading - t4)*100;
	t6 = (tempReading - t4 - t5/100) * 1000;
	sprintf(stats, "T:%c%i.%i%i+H:%i.%i%i+s:%s\n", sign, t1, t2, t3, t4, t5, t6, forecast[current].status);
	uart_puts(stats); // Send to seeeduino

} 

/*
 *	Receives the data from the SeeeduinoXiao
 *	Decodes the data encoded by the SeeeduinoXiao
 */
void receive_data(struct Reading *reading, struct Forecast *forecast) {
	char buffer[16];
	char data;
	for (uint8_t i = 0; i < 16; i++) {
		data = uart_getc();
		if (data == '\0') {
			buffer[i] = '\0';
			break;
			} else {
			buffer[i] = data;
		}
	}
	
	// Decode and act on the data
	if (strcmp(buffer, "f") == 0 ) { // If the buffer and forecast match
		sync_forecast(forecast);
	} else if (strcmp(buffer, "12hour") == 0 ) {
		reading->hourtype = HALFDAY;
	} else if (strcmp(buffer, "24hour") == 0 ) {
		reading->hourtype = FULLDAY;
	} else if (strcmp(buffer, "clock") == 0) {
		// This gives the current clock information into the registers
		update_clock(reading);
		set_clock(reading);
	} else if (strcmp(buffer, "C") == 0) {
		reading->celsius = 1;
	} else if (strcmp(buffer, "F") == 0) {
		reading->celsius = 0;
	} else if (strcmp(buffer, "alarm") == 0) {
		change_alarm(reading);
		set_alarm(*reading);
	} else if (strcmp(buffer, "K") == 0) {
		reading->connected = CONNECTED;
	}
}

/*
 *	Changes the set alarm based on the values from the struct Reading.
 */
void change_alarm(struct Reading *reading) {
	uint8_t vTen, vOne, value;
	for (uint8_t j = 0; j < 2; j++) { // Alarm Data (Hours, Minutes)
		char buffer[16]; // Buffer holds received message
		char data;
		// Message Received
		for (uint8_t k = 0; k < 16; k++) {
			data = uart_getc();
			if (data == '\0') {
				buffer[k] = '\0';
				break;
				} else {
				buffer[k] = data;
			}
		}
		// Change the set alarm hours to the received messages alarm
		value = atoi(buffer);
		vTen = value/10;
		vOne = value - vTen*10;
		if (j == 0) {
			reading->alarmhour = (vTen<<4) | vOne;
		} else {
			reading->alarmmin = (vTen<<4) | vOne;
		}	
	}
}

/*
 * 	Receives a week's worth of forecast from the seeeduinoXiao
 * 	and stores it into the Forecast struct
 */
void sync_forecast(struct Forecast *forecast) {	
	for (uint8_t i = 0; i < 7; i++) { // 7 Days
		for (uint8_t j = 0; j < 6; j++) { // The days data
			char buffer[16];
			char data;
			// Message Received
			for (uint8_t k = 0; k < 16; k++) {
				data = uart_getc();
				if (data == '\0') {
					buffer[k] = '\0';
					break;
					} else {
					buffer[k] = data;
				}	
			}
			
			// Store information into forecast struct
			switch (j) {
				case 0: // Year
					forecast[i].year = atoi(buffer) - 2000;			
					break;

				case 1: // Month
					forecast[i].month = atoi(buffer);		
					break;

				case 2: // Day
					forecast[i].day = atoi(buffer);
					break;
				
				case 3: // Temperature (float)
					forecast[i].temperature = atof(buffer);
					break;

				case 4: // Humidity (float)
					forecast[i].humidity = atof(buffer);
					break;

				case 5:
					strncpy(forecast[i].status, buffer, 10);
					PORTD |= (1<<7);
					break;
			}
			_delay_ms(575);
		}
	}
}

/*
 * 	Receives the current time information from the seeeduinoXiao
 * 	and stores it into the Reading struct.
 */
void update_clock(struct Reading *reading) {
	uint8_t count = 0, tens, ones;
	uint16_t newvalue;
	// Stores 7 parts of data information
	while (count < 7) {
		char buffer[16];
		char data;
		// Receive the data from the seeeduinoXiao
		for (uint8_t i = 0; i < 16; i++) {
			data = uart_getc();
			if (data == '\0') {
				buffer[i] = '\0';
				break;
				} else {
				buffer[i] = data;
			}
		}
		if (count == 0) {
			// At count 0, its the year
			newvalue = atoi (buffer) - 2000;
		} else {
			newvalue = atoi (buffer);
		}
		tens = newvalue / 10;
		ones = newvalue - (tens * 10);
		
		switch (count) {
			case 0: // Year
				reading->year = (tens << 4) | ones;
				break;
			case 1: // Month
				reading->month = (tens << 4) | ones;
				break;						
			case 2: // Day
				reading->day = (tens << 4) | ones;
				break;
			case 3: // Hour
				reading->hours = (tens << 4) | ones;
				break;
			case 4: // Minutes
				reading->minutes = (tens << 4) | ones;
				break;	
			case 5: // Seconds
				reading->seconds = (tens << 4) | ones;
				break;	
			case 6: // Weekday
				reading->weekday = ones;
				break;
		}
		count++;
	}
}

/*
 *	Reads the temperature and humidity
 *	Returns the readings into the Reading Struct
 */
void read_temp_and_humidity(struct Reading *reading) {
	uint8_t lowTemp, highTemp, lowHumid, highHumid;
	// Reading
	// To use the humidity sensor, you must awaken it, then it allows the acquisition of temp and humidity sensors
	// the entire process must happen within 3 seconds, due to it becoming inactive and must be awaken once more to start again
	// you can only acquire the information every 2 seconds!

	// Wake up the device
	i2c_start((TEMPHUMID_ADDR<<1) + I2C_WRITE); // set device address and write mode
	_delay_ms(1);
	i2c_stop();
	
	// Once awaken, send read commands
	if (i2c_start((TEMPHUMID_ADDR<<1)+I2C_WRITE)) { // set device address and write mode
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
		i2c_readAck();									// Byte Seven: 	CRC Most Significant Byte
		i2c_readNak();									// Byte Eight: 	CRC Least Significant Byte
			
		_delay_ms(1);
		i2c_stop();
			
		//Calculates actual temperature and humidity from readings
		short temperatureCalc = (highTemp * 256) + ((lowTemp >> 4)* 16) + (lowTemp & 0x0F);
		uint8_t temperature = temperatureCalc/10;
		short humidityCalc = (highHumid * 256) + ((lowHumid >> 4)* 16) + (lowHumid & 0x0F);
		uint8_t humidity = humidityCalc/10;
		
		// Update readings into struct
		if (humidity <=100) {
			reading->humidity = humidity;
		}
		if (temperature <= 100) {
			reading->temperature = temperature;
		}
	}     
}

/*
 * Initialises the accelerometer
 */
void setup_accel (void) {
	// Disable measurement register
	i2c_start((ACCEL_ADDR << 1) + I2C_WRITE);	// Set device address and write mode
	i2c_write(0x2D);	// Function code 0x03
	i2c_write(0x00);	// Disable Measurement
	i2c_stop();
		
	i2c_start((ACCEL_ADDR << 1) + I2C_WRITE);
	i2c_write(0x38);	
	i2c_write(0x00);	// Bypass FIFO
	i2c_stop();
		
	i2c_start((ACCEL_ADDR << 1) + I2C_WRITE);
	i2c_write(0x1D);	// THRESHOLD FOR TAP
	i2c_write(0x8F);	// THRESHOLD IS HALF
	i2c_stop();
	
	i2c_start((ACCEL_ADDR << 1) + I2C_WRITE);
	i2c_write(0x2A);	// Tap Detection Axes
	i2c_write(0x07);	// Enable all Axes detection
	i2c_stop();
		
	i2c_start((ACCEL_ADDR << 1) + I2C_WRITE);
	i2c_write(0x21);	// Duration Register
	i2c_write(0x10);	// Duration is half
	i2c_stop();
		
	i2c_start((ACCEL_ADDR << 1) + I2C_WRITE);
	i2c_write(0x22);	// Latency Register
	i2c_write(0x10);	// Latency is half
	i2c_stop();
		
	i2c_start((ACCEL_ADDR << 1) + I2C_WRITE);
	i2c_write(0x23);	// THRESHOLD FOR TAP
	i2c_write(0xFF);	// Window is half
	i2c_stop();
		
	i2c_start((ACCEL_ADDR << 1) + I2C_WRITE);
	i2c_write(0x2E);				// Set Interrupt Mapping
	i2c_write((1 << 6) | (1 << 5));	// Single Tap and Double Tap Enabled
	i2c_stop();
		
	i2c_start((ACCEL_ADDR << 1) + I2C_WRITE);
	i2c_write(0x2F);	// Set Interrupt Mapping
	i2c_write(1 << 5);	// Pins -> int1: Single Tap and int2: Double Tap Enabled
	i2c_stop();
	
	i2c_start((ACCEL_ADDR << 1) + I2C_WRITE);
	i2c_write(0x2C);		
	i2c_write((1 << 3));	// Sets speed for lowpower mode
	i2c_stop();
	
	i2c_start((ACCEL_ADDR << 1) + I2C_WRITE);
	i2c_write(0x2D);		
	i2c_write((1 << 3));	// Enable Measurement
	i2c_stop();
}

/*
 *	Reads the X, Y, Z Axis onto the Reading Struct from the Accelerometer
 */
void read_accel(struct Reading *reading) {
	short xAxis, yAxis, zAxis;
	char xAxisMSB, xAxisLSB, yAxisMSB, yAxisLSB, zAxisMSB, zAxisLSB;
	
	// Enable Measurement bit
	(i2c_start((ACCEL_ADDR << 1) + I2C_WRITE)); // Set device address and write mode		
	i2c_write(0x2D);		// Function code 0x03
	i2c_write(0b00001000);	// Function code 0x03
	i2c_stop();
	
	// Write Data Format
	(i2c_start((ACCEL_ADDR << 1) + I2C_WRITE));	
	i2c_write(0x31);		// Function code 0x03
	i2c_write(0b00001011); 	// Enable 16g
	i2c_stop();

	// X Axis LSB
	i2c_start((ACCEL_ADDR << 1 ) + I2C_WRITE);		// Set device address and write mode
	i2c_write(0x32);	// X Axis LSB
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR << 1) + I2C_READ);    // Set device address and Read Mode
	xAxisLSB = i2c_readNak();
	i2c_stop();
	// X Axis MSB
	i2c_start((ACCEL_ADDR << 1) + I2C_WRITE);		// Set device address and write mode
	i2c_write(0x33);	// X Axis MSB
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR << 1) + I2C_READ);    // Set device Address and Read Mode
	xAxisMSB = i2c_readNak();
	i2c_stop();
	
	// Y Axis LSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x34);	// Y Axis LSB
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device Address and Read Mode
	yAxisLSB = i2c_readNak();	
	i2c_stop();
	// Y Axis MSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x35);	// Y Axis MSB
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device Address and Read Mode
	yAxisMSB = i2c_readNak();
	i2c_stop();
	
	// Z Axis LSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE); // set device address and write mode
	i2c_write(0x36);	// Z Axis LSB
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device Address and Read Mode
	zAxisLSB = i2c_readNak();								
	i2c_stop();	
	// Z Axis MSB
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE); // set device address and write mode
	i2c_write(0x37);	// Z Axis MSB
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device Address and Read Mode
	zAxisMSB = i2c_readNak();
	i2c_stop();
	
	// Format and store the data
	xAxis = (xAxisMSB << 8) | xAxisLSB;
	yAxis = (yAxisMSB << 8) | yAxisLSB;
	zAxis = (zAxisMSB << 8) | zAxisLSB;
	
	reading->xAxis = xAxis;
	reading->yAxis = yAxis;
	reading->zAxis = zAxis;
}

/*
 *	Sets the clock times onto the MCP790N
 *	Argument hourMode: 1 -> 12 hour, 0 -> 24 Hour
 *	startClock: 0 -> Clock is OFF, 1 -> Clock is ON
 */
void set_clock (struct Reading *reading) {
	char setSeconds, setMinutes, setHours, setDay, setMonth, setYear, setWeekday;
	
	// Set the start bit for the clock + import seconds
	setSeconds = (1<<7) | reading->seconds; 

	setMinutes = reading->minutes;
	// BitMasked for 24 Hour Format
	setHours = 0x3F & reading->hours; 
	
	setDay = reading->day;
	setMonth = 0x2F & reading->month;
	setYear = reading->year;
	setWeekday = (1 << 3) | reading->weekday;
	
	// Write the seconds
	i2c_start((CLOCK_ADDR << 1) + I2C_WRITE);// set device address and write mode		
	i2c_write(0x00);	// Function code: seconds
	i2c_write(setSeconds);	// Function code 
	i2c_stop();
	// Write the minutes
	i2c_start((CLOCK_ADDR << 1) + I2C_WRITE);
	i2c_write(0x01);
	i2c_write(setMinutes);	
	i2c_stop();	
	// Write the hours
	i2c_start((CLOCK_ADDR << 1) + I2C_WRITE);
	i2c_write(0x02);
	i2c_write(setHours);
	i2c_stop();
	// Write the weekday
	i2c_start((CLOCK_ADDR << 1) + I2C_WRITE);
	i2c_write(0x03);
	i2c_write(setWeekday);
	i2c_stop();
	// Write the day
	i2c_start((CLOCK_ADDR << 1) + I2C_WRITE);
	i2c_write(0x04);
	i2c_write(setDay);
	i2c_stop();
	// Write the month
	i2c_start((CLOCK_ADDR << 1) + I2C_WRITE);
	i2c_write(0x05);
	i2c_write(setMonth);
	i2c_stop();
	// Write the year
	i2c_start((CLOCK_ADDR << 1) + I2C_WRITE);
	i2c_write(0x06);	
	i2c_write(setYear);	
	i2c_stop();
}

/* 	
 *	Reads the clock times from the Clock IC onto the Reading Struct
 *	Converts Hexadecimal to integer values
 */
void read_clock(struct Reading *reading) {
	// Read the seconds
	uint8_t readSeconds, readMinutes, readHours, scannedHours;
	i2c_start((CLOCK_ADDR << 1) + I2C_WRITE);// set device address and write mode
	i2c_write(0x00);
	i2c_stop();
	i2c_rep_start((CLOCK_ADDR << 1) + I2C_READ);    // Set device address and Read Mode
	readSeconds = ~(1 << 7) & i2c_readNak();
	i2c_stop();
	
	// Read the minutes
	i2c_start((CLOCK_ADDR << 1) + I2C_WRITE);// set device address and write mode
	i2c_write(0x01);
	i2c_stop();
	i2c_rep_start((CLOCK_ADDR << 1) + I2C_READ);    // Set device address and Read Mode
	readMinutes = i2c_readNak();
	i2c_stop();
	
	// Read the hours
	i2c_start((CLOCK_ADDR << 1) + I2C_WRITE);// set device address and write mode
	i2c_write(0x02);
	i2c_stop();
	i2c_rep_start((CLOCK_ADDR << 1) + I2C_READ);    // Set device	Address and Read Mode
	scannedHours = i2c_readNak();									
	i2c_stop();
	
	// Mask the values
	readHours = 0x3F & scannedHours;
	
	reading->seconds = readSeconds;
	reading->minutes = readMinutes;
	reading->hours = readHours;
}

/*
 *	Transitions the states of the alarm between Armed, Active, and Deactivated
 * 	Alarm can only transition states when in Mode A
 * 	Function only triggers when it detects a double tap from the accelerometer
 */
void transition_state(struct Reading *reading) {
	// Armed -> Active
	if ((PINC & (1<<1)) == 0 && reading->alarmstate == ARMED) { 
		// Clock Alarm
		reading->alarmstate = ACTIVE;
	}
	if (PINC & (1<<0)) { // If double tap detected.
		// Deactivated -> Armed
		if (reading->alarmstate == DEACTIVATED) { 
			reading->alarmstate = ARMED;
		} else if (reading->alarmstate == ARMED && (PINC & (1<<1))) { 
			// Armed -> Deactivated
			// if State: ARMED but Alarm is not active, switch back to deactivated
			reading->alarmstate = DEACTIVATED;
		} else if (reading->alarmstate == ACTIVE) { 
			// Activated -> Deactivated
			reading->alarmstate = DEACTIVATED;
			clear_alarm_flags();
		}
	}
	// Clears double tap interrupt
	i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);    	// Set device address and write Mode
	i2c_write(0x30);
	i2c_stop();
	i2c_rep_start((ACCEL_ADDR<<1)+I2C_READ);    // Set device address and Read Mode
	i2c_readNak();								// Clears the interrupt
	i2c_stop();
}

/*
 * Detects the orientation of the device and clasifies it into
 * orientations 'A', 'B', 'C, 'D', or 'N', where N is an error.
 */
char detect_mode(struct Reading reading) {
	char result;
	if (reading.xAxis < 50 && reading.xAxis > -50 && reading.yAxis > 200) {
		result = 'A';
	} else if (reading.yAxis < 50 && reading.yAxis > -50 && reading.xAxis > 200) {
		result = 'B';
	} else if (reading.xAxis < 50 && reading.xAxis > -50 && reading.yAxis < -200) {
		result = 'C';
	} else if (reading.yAxis < 50 && reading.yAxis > -50 && reading.xAxis < -200) {
		result = 'D';
	} else {
		result = 'N'; // Error Code N: Transition State, not any discrete part.
	}
	return result;
}

/*
 * 	Resets the flags for the alarm in the Clock IC
 * 	Deactivates the alarm flag
 */
void clear_alarm_flags(void) {
	// Set Alarm Masks
	// Alarm 0, triggers if hours match
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x0D);
	i2c_write(0x20);	// Alarm Mask for hours
	i2c_stop();
	// Alarm 1, triggers if minutes match
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x14);
	i2c_write(0x10);	// Alarm Mask for minutes
	i2c_stop();		
}

/*
 *	Sets the alarm on the Clock IC to whats been stored in the Reading Struct	
 */
void set_alarm (struct Reading reading) {
	uint8_t alarmMinutes, alarmHours;
	
	// Set alarm minutes
	alarmMinutes = ~(1<<7) & reading.alarmmin;
	// BitMasked for 24 Hour Format
	alarmHours = 0x3F & reading.alarmhour; 
	
	// Set Alarm Masks
	// Alarm 0, triggers if hours match
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x0D);	
	i2c_write(0x20);	// Alarm Mask for hours
	i2c_stop();
	// Alarm 1, triggers if minutes match
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x14);	
	i2c_write(0x10);	// Alarm Mask for minutes
	i2c_stop();

	// Write the hours
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x0C);
	i2c_write(alarmHours);	
	i2c_stop();		

	// Write the minutes
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x12);
	i2c_write(alarmMinutes);	
	i2c_stop();

	// Set Control Registers
	i2c_start((CLOCK_ADDR<<1)+I2C_WRITE);// set device address and write mode
	i2c_write(0x07);	
	i2c_write((1<<5)|(1<<4));	// Enable single alarm interrupt (ALM0EN) and external crystal source
	i2c_stop();
}

/*
 * Toggles the GPIO Pin which is connected to the alarm buzzer
 * It cycles on for 700ms and off for 300ms.
 */
void alarm_active(void) {
	// Toggles PORTD6 GPIO
	alarm_count++; // count goes up every 20ms
	if (alarm_count <= 14) { // 700ms
		PORTD |= (1<<6);
	} else if (alarm_count > 14 && alarm_count < 20){
		PORTD &= ~(1<<6); // 300ms
	}
	if (alarm_count == 20) {
		alarm_count = 0;
	}
}

/*
 *	Displays the alarm time on the SSD1306 Screen
 */
void display_alarm_time(struct Reading *reading) {
	uint8_t alarmHourCopy;
	uint8_t mTen, mOne, hTen, hOne;
	uint8_t tempTen, tempOne, tempValue;
	
	// If hourtype is half day but the format is full day (>12), this will format it correctly.
	if (reading->hourtype == FULLDAY) {
		// FULLDAY hourtype means its ranges from 00:00 to 23:59
		if (reading->alarmhour == 0x24) {
			// If the alarm hour reaches 2400, return 0000
			alarmHourCopy = 0x00;
			reading->alarmAmPm = AM;
		} else {
			alarmHourCopy = reading->alarmhour & 0x3F;
		}
	} else if (reading->hourtype == HALFDAY && reading->alarmhour > 0x12) { 
		// HALFDAY hourtype means its range from 00:00 to 12:59 with AM and PM formatting
		// Occurs for any clock value in PM
		
		tempTen = reading->alarmhour >> 4;
		tempOne = reading->alarmhour & 0x0F;
		tempValue = tempTen * 10 + tempOne;
		tempValue -= 12;
		
		tempTen = tempValue/10;
		tempOne = tempValue - (tempTen*10);
		
		alarmHourCopy = tempTen << 4 | tempOne;
		
		reading->alarmAmPm = PM;
	} else if (reading->hourtype == HALFDAY && reading->alarmhour == 0x12){ 
		// exception: 12PM
		alarmHourCopy = reading->alarmhour;
		reading->alarmAmPm = PM;
	} else if (reading->hourtype == HALFDAY && reading->alarmhour == 0x00){ 
		// exception: 12AM
		alarmHourCopy = 0x12;
		reading->alarmAmPm = AM;
	} else if (reading->hourtype == HALFDAY && reading->alarmhour == 0x24){ 
		// exception: 12AM
		alarmHourCopy = 0x12;
		reading->alarmAmPm = AM;
	} else{
		// Occurs for any clock value in AM
		alarmHourCopy = reading->alarmhour;
		reading->alarmAmPm = AM;
	}
	
	// Write the clock
	mTen = (reading->alarmmin>>4);
	mOne = (reading->alarmmin & 0x0F);
	hTen = (alarmHourCopy >> 4);
	hOne = alarmHourCopy & 0x0F;
	char buffer[20], alarmString[20];
	
	// Handles leading 0 on hour
	if (hTen) {
		char buffer[20] = {'A', 'l', 'a','r','m',':',int_to_char(hTen), int_to_char(hOne),
							':', int_to_char(mTen), int_to_char(mOne), ':', '0', '0'};
		strncpy(alarmString, buffer, 20);
		} else {
		char buffer[20] = {'A', 'l', 'a','r','m',':',' ', int_to_char(hOne),
		':', int_to_char(mTen), int_to_char(mOne), ':', '0', '0'};
		strncpy(alarmString, buffer, 20);
	}
	
	// Display the alarm time on the SSD1306
	u8g2_SetFont(&u8g2, u8g2_font_6x13_tr);
	u8g2_SetFontMode(&u8g2, 1);
	u8g2_SetDrawColor(&u8g2, 2);
	if (reading->hourtype) {
		if (reading->alarmAmPm) { //0 -> AM, 1-> PM
			u8g2_DrawStr(&u8g2, 85, 0, "PM");
			} else {
			u8g2_DrawStr(&u8g2, 85, 0, "AM");
		}
	}
	u8g2_DrawStr(&u8g2, 0, 0, alarmString);
}

/*
 *	Displays the current time with options of AM/PM or 24 Hour format
 *	Also displays the alarm that was set, and if it is active or not.
 */
void mode_A(struct Reading *reading) {
	uint8_t sTen, sOne, mTen, mOne, hTen, hOne;
	uint8_t seconds, minutes, hours;
	
	// Format from Reading Struct to decimal format.
	seconds = (reading->seconds >> 4) * 10 + (reading->seconds & 0x0F);
	minutes = (reading->minutes >> 4) * 10 + (reading->minutes & 0x0F);
	hours = (reading->hours>>4) * 10 + (reading->hours & 0x0F);
	// Gets the individial digit values
	sTen = seconds / 10;
	sOne = seconds- sTen * 10;
	mTen = minutes / 10;
	mOne = minutes - mTen * 10;
	
	// Formatting for HALFDAY or if the time is before 1PM.
	if (reading->hourtype == HALFDAY && hours > 12) { 
		// PM Values
		uint8_t hourdigits = hours - 12;
		hTen = hourdigits / 10;
		hOne = hourdigits - hTen*10;
		reading->AmPm = PM;
	} else if (reading->hourtype == HALFDAY && hours == 12) { 
		// 12PM
		hTen = 1;
		hOne = 2;
		reading->AmPm = PM;
	}else if (reading->hourtype == HALFDAY && hours == 0) { 
		// 12AM
		hTen = 1;
		hOne = 2;
		reading->AmPm = AM;
	} else { 
		// AM Values
		hTen = hours / 10;
		hOne = hours - hTen*10;
		reading->AmPm = AM;
	}
	
	// Handles leading 0 on hour
	char display[10];
	if (hTen) {
		char sample[10] = {int_to_char(hTen), int_to_char(hOne),':', int_to_char(mTen), int_to_char(mOne) ,':', int_to_char(sTen) , int_to_char(sOne)};
		strncpy(display, sample, 10);
	} else {
		// Removes leading 0
		char sample[10] = {' ', int_to_char(hOne),':', int_to_char(mTen), int_to_char(mOne) ,':', int_to_char(sTen) , int_to_char(sOne)};
		strncpy(display, sample, 10);
	}
	
	// Check and transition state if double tap flag is detected
	transition_state(reading);
	
	// Setup parameters for the SSD1306
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetDisplayRotation(&u8g2, U8G2_R0);
	u8g2_SetFont(&u8g2, u8g2_font_t0_14b_tr);
	u8g2_SetFontMode(&u8g2, 1);
	u8g2_SetDrawColor(&u8g2, 2);
	if (reading->hourtype) {
		if (reading->AmPm) {
			u8g2_DrawStr(&u8g2, 110, 55, "PM");
		} else {
			u8g2_DrawStr(&u8g2, 110, 55, "AM");
		}
	}
	// Display Glyph to show if the seeeduinoXiao is plugged in
	if (reading->connected) {
		u8g2_SetFont(&u8g2, u8g2_font_unifont_t_76);
		u8g2_DrawGlyph(&u8g2, 0, 50, 0x2620);
	}

	// Toggle the buzzer and display depending on the alarm state
	if (reading->alarmstate == DEACTIVATED) {
		PORTD &= ~(1<<6);  // Turn off buzzer
	} else if (reading->alarmstate == ARMED) {
		PORTD &= ~(1<<6); // Turn off buzzer
		u8g2_SetFont(&u8g2, u8g2_font_unifont_t_76);
		u8g2_DrawGlyph(&u8g2, 110, 0, 0x2611);
	} else if (reading->alarmstate == ACTIVE) {
		alarm_active();
		if (staticCount <= 5) {
			u8g2_SetFont(&u8g2, u8g2_font_unifont_t_76 );
			u8g2_DrawGlyph(&u8g2, 110, 0, 0x2611);
		} else if (staticCount >= 10) {
			// Reset counter if it gets too high
			staticCount = 0;
		}
		staticCount++;
	}
	// Display the Clock and Alarm on the SSD1306
	u8g2_SetFont(&u8g2, u8g2_font_profont29_tr );
	u8g2_SetFontMode(&u8g2, 1);
	u8g2_SetDrawColor(&u8g2, 2);
	u8g2_DrawStr(&u8g2, 0, 25, display); // Write Clock String
	display_alarm_time(reading); // Write Alarm String
	u8g2_SendBuffer(&u8g2);
}

/*
 *	Mode B: displays the current weather forecast that was stored in the device 
 */
void mode_B(struct Reading *reading, struct Forecast *forecast) {
	
	// Mode B is the same fahreinheight/Celsius as Mode C
	float tempReading;
	char temperature[10], tempMeasurement[10], dayWord[15] = "nothing", humidity[10];
	char sign, tempSign;
	uint16_t weatherIcon;
	uint8_t current, decimalDay;
	// Find current day in the forecast[] struct array
	for (uint8_t i = 0; i < 7; i++) {
		// If the day-date matches to current day, gather that information
		decimalDay = ((reading->day >> 4) * 10) + (reading->day & 0x0F);
		if (forecast[i].day == decimalDay) {
			current = i;
			break;
		} else {
			current = 0;
		}
	}

	if (reading->celsius) {
		tempReading = forecast[current].temperature;
		tempSign = 'C';
	} else {
		// Convert to fahreinheight
		tempReading = (forecast[current].temperature * 9/5) + 32; 
		tempSign = 'F';
	}

	if (tempReading < 0) {
		sign = '-';
		tempReading = tempReading * -1;
	} else {
		sign = ' ';
	}

	uint8_t t1 = tempReading;
	uint8_t t2 = ((float)tempReading - t1)*100;
	uint8_t t3 = (tempReading - t1 - (float)t2/100) * 1000;
	snprintf(temperature,10, "%c%i.%i%i", sign, t1, t2, t3);
	char buffer[5] = {' ', 0xB0, tempSign};
	strncpy(tempMeasurement, buffer, 10);
	
	uint8_t h1 = forecast[current].humidity;
	uint8_t h2 = ((float)forecast[current].humidity - h1)*100;
	uint8_t h3 = (forecast[current].humidity - h1 - (float)h2/100) * 1000;
	snprintf(humidity,10, " %i.%i%i", h1, h2, h3);

	// Display different weekdays depending on the week
	switch (reading->weekday) {
		case MON:
			strncpy(dayWord, "Monday", 15);
			break;
		
		case TUE:
			strncpy(dayWord, "Tuesday", 15);
			break;

		case WED:
			strncpy(dayWord, "Wednesday", 15);
			break;
		
		case THU:
			strncpy(dayWord, "Thursday", 15);
			break;
		
		case FRI:
			strncpy(dayWord, "Friday", 15);
			break;
		
		case SAT:
			strncpy(dayWord, "Saturday", 15);
			break;
		
		case SUN:
			strncpy(dayWord, "Sunday", 15);
			break;		
		default:
			strncpy(dayWord, "error", 15);
			break;
	}
	
	// Display different symbols depending on the weather
	if (strcmp(forecast[current].status, "sunny") == 0) {
		weatherIcon = 0x2600; // A sun
	} else if (strcmp(forecast[current].status, "cloudy") == 0) {
		weatherIcon = 0x2601; // A cloud
	} else if (strcmp(forecast[current].status, "rain") == 0) {
		weatherIcon = 0x2602; // A umbrella
	} else if (strcmp(forecast[current].status, "storm") == 0) {
		weatherIcon = 0x2614; // A umbrella with rain
	} else if (strcmp(forecast[current].status, "snow") == 0) {
		weatherIcon = 0x2603; // A snowman
	} else {
		weatherIcon = 0x2613; // A big X
	}

	// Display data on OLED
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetDisplayRotation(&u8g2, U8G2_R3);
	if (reading->connected) {
		u8g2_SetFont(&u8g2, u8g2_font_unifont_t_76);
		u8g2_DrawGlyph(&u8g2, 50, 48, 0x2620);
	}
	// Set font and sizing
	u8g2_SetFont(&u8g2, u8g2_font_t0_14b_tr);
	u8g2_SetFontMode(&u8g2, 1);
	u8g2_SetDrawColor(&u8g2, 2);

	u8g2_DrawStr(&u8g2, 0, 10, dayWord);	// Day of the week

	// Set font and sizing
	u8g2_SetFont(&u8g2, u8g2_font_6x13_tf);
	u8g2_SetFontMode(&u8g2, 1);
	u8g2_SetDrawColor(&u8g2, 2);
	
	u8g2_DrawStr(&u8g2, 0, 50, "Temp:");				// Temperature Reading
	u8g2_DrawStr(&u8g2, 0, 65, temperature);				// Temperature Reading
	u8g2_DrawStr(&u8g2, 40, 65, tempMeasurement);			// Temperature Measurement
	u8g2_DrawStr(&u8g2, 0, 85, "Humidity:");				// Temperature Reading
	u8g2_DrawStr(&u8g2, 0, 100, humidity );					// Humidity Reading
	u8g2_DrawStr(&u8g2, 40, 100, "  %");					// Humidity Measurement
	u8g2_DrawStr(&u8g2, 21, 28, forecast[current].status);	// Humidity Measurement

	// Set font
	u8g2_SetFont(&u8g2, u8g2_font_unifont_t_76);

	u8g2_DrawGlyph(&u8g2, 0, 30, weatherIcon);

	u8g2_SendBuffer(&u8g2);
}

/*
 *	Mode C: Displays the onboard temperature and humidity readings
 */
void mode_C(struct Reading reading) {
	// Create buffers to place integer to strings
	char tempStr[10];
	char humStr[10];
	char unit1[5];
	int displayTemp;

	if (reading.celsius) {
		displayTemp = reading.temperature;
		char degSign[5] = {'C', '\0'};
		strncpy(unit1, degSign, 5);
	} else {
		displayTemp = (reading.temperature * 9/5) + 32;
		char degSign[2] = {'F', '\0'};
		strncpy(unit1, degSign, 5);
	}
	snprintf(tempStr,10, "%i", displayTemp);
	snprintf(humStr,10, "%i",reading.humidity);
	char degree[] = {0xB0, '\0'};
	// Display data on OLED
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetDisplayRotation(&u8g2, U8G2_R2);
	u8g2_SetFontMode(&u8g2, 1);
	u8g2_SetDrawColor(&u8g2, 2);
	if (reading.connected) {
		u8g2_SetFont(&u8g2, u8g2_font_unifont_t_76);
		u8g2_DrawGlyph(&u8g2, 110, 0, 0x2620);
	}

	u8g2_SetFont(&u8g2, u8g2_font_6x13_tf);
	u8g2_DrawStr(&u8g2, 105, 15,degree); // Temperature
	
	u8g2_SetFont(&u8g2, u8g2_font_t0_14b_tr);

	u8g2_DrawStr(&u8g2, 0, 00,"Temperature: "); // Temperature
	u8g2_DrawStr(&u8g2, 0, 15,tempStr); // Temperature
	u8g2_DrawStr(&u8g2, 110, 15,unit1); // Temperature
	
	u8g2_DrawStr(&u8g2, 0, 30,"Humidity: "); // Temperature
	u8g2_DrawStr(&u8g2, 0,45 ,humStr); // Humidity
	u8g2_DrawStr(&u8g2, 110, 45, "%"); // Humidity
	u8g2_SendBuffer(&u8g2);
}

/*
 * Mode D: Low power mode which turns off the screen to conserve power.
 */
void mode_D(struct Reading *reading) {
	// Low power mode
	// Set possible devices to low power mode
	// Stuff to disable: Temp/humid ic, oled

	if (reading->lowpower == 1) {
		//Set all low power settings
		
		// Set OLED to low power
		u8g2_SetPowerSave(&u8g2, ON);
		
		// accelerometer must still work to register change back
		i2c_start((ACCEL_ADDR<<1)+I2C_WRITE);// set device address and write mode
		i2c_write(0x2C);
		i2c_write((1<<4)|(1<<3));	// Enable single alarm interrupt (ALM0EN) and external crystal source
		i2c_stop();
		
		reading->lowpower = 2; // Indicates that setting low power is complete
	}

}

/*
 * Returns the ASCII of the characters '0' to '9' from 0 to 9 only.
 */
char int_to_char(int number) {
	char result = 'N';
	for (uint8_t i = 0; i < 10; i++) {
		if (number == i) {
			result = 0x30+i;
			break;
		}
	}
	return result;
}