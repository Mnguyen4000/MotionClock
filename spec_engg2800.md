# RollClock

Team Project specification 2023, semester 1

Course: **ENGG2800**

---

Notes:

* For the purposes of this document, we consider “code” or “software” to refer to programs which runs on a PC and/or on an embedded system.
* This document uses the words "should", "must" and so on interchangeably to indicate a requirement of particular functionality. The use of "should" does not imply that functionality is optional.
* While we try our best to allow students to have the freedom to make their own design choices, it is not possible for the teaching staff to support every possible development system.
* This specification is a living document! Changes will occur and will be released throughout the semester. Some minor clarifications may appear on the discussion board only. You are expected to keep up to date with these.
* You are permitted to bundle outside orders with other teams in order to reduce the overall cost of shipping, as long as you take into account the UQ Academic Integrity and Student Conduct policy.
* You must only use PCBs which were ordered from ETSG (via [pinecone](https://pinecone2.uqcloud.net/)). Please see the course profile for details.
* There are a number of grade hurdle requirements outlined in the course profile - please make sure you read these *carefully*.
* You must follow all other requirements outlined in the [TP-STD standards](https://source.eait.uq.edu.au/gitlist/tp_std/tree/master/).
* All assessment must only use parts from the approved suppliers! See TP-STD-003.
* You must individually commit to git in four weeks of the semester. See TP-STD-004 and blackboard for further details.
* Did I mention that you should read the TP-STD documents? If you don't meet these requirements (as applicable), you will be limited to a maximum of 50% of the total marks available for the final demo.


---

## Introduction

RollClock is a device similar to a bedside alarm clock. However, rather than being controlled using physical buttons, it is instead controlled by rotating the device in space. The device is also battery powered and has a special low power mode to extend the life of the batteries.


## Device behaviour

Your device is required to use the following OLED display: ETSG 24-05-06 (the manufacturer documentation can be found [here](https://www.buydisplay.com/i2c-white-1-3-inch-oled-display-module-128x64-arduino-raspberry-pi)). This display is used to communicate the time, date and other information to the user. The display mode changes depending on the rotation of the device - the four possible rotations are shown in the image below:

![Display rotations](https://github.com/Mnguyen4000/MotionClock/edit/main/display.png)

The display must update at a rate of at least 1Hz and the mode must change within 1 second of the device being rotated. In all modes, the display must have the text and icons displayed as large as reasonably possible - more than 60% of the display must contain information.

The device must be able to communicate with the PC in all modes except the sleep mode. If the device is connected to the PC software, there should be an icon on the display indicating as such. If the device is not connected to the PC or the device is connected but the software is not yet running, the icon must not be displayed.

Your device must have a piezo buzzer or speaker that can be used to play an alarm tone. The alarm tone must be clearly audible in the lab environment. The tone is comprised of the following:

- Play a tone for 700ms
- Silent for 300ms
- (repeat until alarm is deactivated)


### Clock functionality (rotation mode A)

Your device must be able to display the current time in both 24 hour and 12 hour (AM/PM) formats as configured by the PC software. Your device should operate normally as a clock - that is, each second, the time should increase by one second. Your device must store time to a resolution of one second. The display format of the time is HH:MM:SS where HH is one or two digits for the hours, MM is two digits for the minutes, and SS is two digits for the seconds. The device must always display the ":" characters in the same position for all times, and it must never display a leading zero on the hours (for example, 9am must not be displayed as 09:00:00). If in 12 format, the display should also show AM or PM clearly on the display.

The clock source for timekeeping must have an accuracy of +/- 50ppm or better.

The user will be able to configure an alarm time via the PC software, and this alarm time must be retained even if the power is removed from the device. The alarm can be in one of three states:

* Deactivated 
* Armed (device is waiting for the alarm condition to be met)
* Active (device is actively playing the alarm tone)

The alarm state is changed from deactivated to armed by lifting and gently tapping the device on the table twice. If the device is doubled tapped again before the alarm condition is met, the state changes back to deactivated. The alarm must change from the armed to the active state as soon as the internal device time reaches the stored alarm time. In the active state, the device must play the alarm tone. While the alarm is in the active state, a double tap will change the state to deactivated. The state of the alarm must be maintained through power cycles of the device.

Your device must have a bell icon visible if the alarm is in the armed state. If the alarm is in the active state, this same icon should be present but also blinking. 

The display must always show the current alarm time in small text.

The alarm tone must be played regardless of the mode that the device is in. The only way to deactivate the active alarm is to rotate the device back into the clock mode and then double tap the device.


### Weather functionality (rotation mode B)

Your device must display the following information:

- The current day of the week (ie Tuesday, Wednesday, etc or "Mon", "Tue", etc)
- The current weather forecast for the day (temperature in Celsius or Farenheit, humidity in %, weather type)
- A large pictographic icon which corresponds to the weather type

The forecast data can be accessed here in JSON format: [https://tp-weather.uqcloud.net/weather.json](https://tp-weather.uqcloud.net/weather.json). Please note that this is not real forecast data - it is fake data which is regenerated every minute to allow for easier testing. The format of each item in the weather list is:

```
"Y-M-D": {  // Y is year, M is month, D is day
	"temperature": AAA, // AAA is a floating point number in the units Celsius
	"humidity": BBB, // BBB is a floating point number in the units %RH (or just %)
	"forecast": "CCC" // CCC is one of the following: "sunny", "cloudy", "rain", "storm", "snow"
}
```

The device must store the forecast data for the next 7 days, and must retain this data even if power is removed from the device. If the device is not synchronised to the PC software within the next 7 days, the display must show an error message or icon indicating that there is no forecast data available.

The device should automatically display the weather data for the next day as time reaches 00:00.

Your device must be configurable through the PC software to use the temperature units preferred by the user, either Celsius or Farenheit. This setting must be maintained even if power is removed from the device.


### Sensor functionality (rotation mode C)

When in the sensor mode, the device must show on the display:

- The current temperature as measured by an onboard sensor
- The current humidity as measured by an onboard sensor

The display must show units with the measurements, where the units for temperature are either Celsius or Farenheit with a preceeding [degree symbol](https://en.wikipedia.org/wiki/Degree_symbol). The units are always the same as the units used in the weather mode.

The measurements must be updated at a sample rate of at least 1Hz.


### Sleep (rotation mode D)

When the device is in the sleep mode, the display is turned off and other subsystems can be powered down as appropriate. The device time must still increment normally and the alarm tone must activate if the alarm conditions are met. To exit sleep mode, the device is rotated into one of the other modes. 

While in sleep mode, your device must draw less than 500uA (averaged over 5 seconds) from the battery at the nominal rated battery voltage while the USB port is not connected, the device is still and the alarm is in either the deactivated or armed state. The software functionality does not need to function in the sleep mode. The device must reduce the power draw below the limit within 1 second of entering the sleep mode. 

### Testing functionality

In order to test your device, you must have physical controls (switch/button) which change the device into one of three test modes:

- Test mode off (device behaves normally)
- Time speedup * 60 (1 second of real time = 1 minute in the device)
- Time speedup * 60 * 60 (1 second of real time = 1 hour in the device)

This control may be changed at any time while the device is on, however in the speedup modes, the low power sleep functionality is permitted to be disabled. The speed up modes do not apply to the display refresh rate. The speed up modes also do not need to adhere to the +/- 50ppm accuracy requirement.


## Power supply

Your device must be powered by 2 to 4 alkaline AA or AAA batteries (hereafter referred to as the "battery pack"). When connected to the PC, it is acceptable if the device is powered by the USB port.

When in sleep mode, your device must draw less than the current specified in the "Sleep" mode description. The connection to the PCB must have an inline jumper that can be used for measuring battery current as per the TP-STD documents.

Your device must be able to retain (and continue incrementing as appropriate) the current time while the device has the USB port disconnected *and* the battery pack is removed. This also applies to the alarm. It is expected that you will use a separate coin cell battery to implement this - the coin cell must not be connected to anything other than a clock IC - for example, it must not be connected to the main microcontroller. This coin cell must use a PCB mounted holder.

The device must have a physical power switch that can be used to turn the device on and off. It is expected that this switch will totally disconnect the battery, and as such can be one which is built into a battery pack.


## Communication

All communication with the PC must happen using the [polyglot-turtle-xiao](https://github.com/jeremyherbert/polyglot-turtle-xiao) firmware running on the seeeduino xiao (hereafter referred to as 'turtleboard'). This device must be the only means of communication with the PC. Note that also the device may be connected to a USB power supply that has no data (ie a USB charger, power bank, etc), in which case it must still operate normally.


## PC software and configuration

Your PC software must allow the display at a minimum rate of 1Hz the following parameters in a GUI interface:

* The current time and day of week on the device
* Whether the device is using the 12 or 24 hour time format
* The currently configured alarm time on the device and the alarm state
* The current readings for the accelerometer, temperature and humidity sensor
* The current forecast data stored in the device
* The current unit selection on the device

Your software must be able to display and synchronise the current time on the PC and the weather forecast data from the server to the device. The software must also be able to change the device between 12 and 24 hour time formats and change the temperature units between Celsius and Farenheit.

The software must not automatically synchronise or send any new settings to the device upon connection. There must be a GUI control (button or toggle, etc) which the user activates explicitly to trigger the synchronisation.

Your device must operate normally with your PC software closed or disconnected.


## Construction and physical dimensions

Your final product must be constructed using a custom designed PCB and mounted on the provided acrylic hardware (details will be available at a later date). The PCB must not extend beyond the acrylic enclosure. Your battery pack must be mounted on the PCB or on the enclosure - it is recommended that you mount it in a removable fashion (using screws, velcro, etc) so that it can be removed if necessary.

There will be a significant penalty for using a breadboard - the total marks for this task will be limited to 50% and hurdles listed in the course profile will apply.

To achieve the higher grades in this course, your device must not use any breakout boards in its construction as per the course profile. However, the following components are an exception to this rule:

* The turtleboard
* A breakout board containing an accelerometer


## Budget/Bill Of Materials (BOM)
Your final product must have a BOM total of $85 AUD or less (excluding GST). Your team will have a $150 AUD development budget available at ETSG; reimbursement up to this amount for parts purchased externally may be possible at the end of semester; further details and conditions will be made available on blackboard.
You do not need to include the cost of the USB cable in your BOM, but you must still provide them with your submission.

You do not need to include the cost of the acrylic or any mounting hardware provided with the acrylic in your BOM.


## Other components included in your locker

* A resistor kit containing various values
* A capacitor kit containing various values
* Seeeduino Xiao to be used with the polyglot-turtle firmware (ETSG part 24-01-01): [https://www.seeedstudio.com/Seeeduino-XIAO-Arduino-Microcontroller-SAMD21-Cortex-M0+-p-4426.html](https://www.seeedstudio.com/Seeeduino-XIAO-Arduino-Microcontroller-SAMD21-Cortex-M0+-p-4426.html)
* ATMEGA328P (ETSG part 10-03-03): [https://www.digikey.com.au/product-detail/en/microchip-technology/ATMEGA328P-PU/ATMEGA328P-PU-ND/1914589](https://www.digikey.com.au/product-detail/en/microchip-technology/ATMEGA328P-PU/ATMEGA328P-PU-ND/1914589)
* AVR ISP breakout (ETSG part 16-56-01): [https://www.adafruit.com/product/1465](https://www.adafruit.com/product/1465)
* Real Time Clock (RTC) example IC (ETSG part 07-35-02): [https://www.digikey.com.au/en/products/detail/microchip-technology/MCP7940M-I-P/3176796](https://www.digikey.com.au/en/products/detail/microchip-technology/MCP7940M-I-P/3176796)
* 32.768kHz crystal oscillator (ETSG part 05-68-02): [https://www.digikey.com.au/en/products/detail/citizen-finedevice-co-ltd/CFS-20632768DZBB/862578](https://www.digikey.com.au/en/products/detail/citizen-finedevice-co-ltd/CFS-20632768DZBB/862578)

