# MotionClock

Motion Clock is a device which utilises motion to control and set the device in its different modes.
There are four modes that can be used, which are Alarm Clock, Weather Forecast, Onboard Temperature and Humidity, and Low Power Mode.

Communication protocols it uses are UART, I2C and SPI. 
UART is used to communicate between the Seeeduino Xiao and the ATMEGA328P. 
I2C are used to communicate to its various sensors. 
SPI is used to program the ATMEGA328P through UsbASP


Note:

Missing u8g2_fonts.c file in MotionClock/AtmelStudioFiles/GccApplication1

Need to get it from the u8g2 library to fully function the code. Could not upload due to 25Mb capping on github.
