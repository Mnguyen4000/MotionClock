import serial
import json
from time import strftime, localtime
import time
import urllib.request
from tkinter import *
from tkinter import ttk

url = "https://tp-weather.uqcloud.net/weather.json"

response = urllib.request.urlopen(url)
data_json = json.loads(response.read())
data = data_json.get('weather')



root = Tk()
root.title("Motion Clock GUI")
root.geometry("500x400")  # Sets window size
root.resizable(0, 0)  # Disables window readjusting



timetext = Label(root,font=("Arial", 12))
timetext.grid(row=0, column=4)


xValue = StringVar()
xValue.set(999)

yValue = StringVar()
yValue.set(999)

zValue = StringVar()
zValue.set(999)

measurement = StringVar()
measurement.set('error')  # Celcius or Fahrenheit Placeholder

tempValue = StringVar()
tempValue.set("error")

humidityValue = StringVar()
humidityValue.set("error")


# Device Time
hoursValue = StringVar()
hoursValue.set("error")
minutesValue = StringVar()
minutesValue.set("error")
secondsValue = StringVar()
secondsValue.set("error")

weekdayValue = StringVar()
weekdayValue.set("error")

hourTypeValue = StringVar()
hourTypeValue.set("error")


alarmhourValue = StringVar()
alarmhourValue.set("error")

alarmminuteValue = StringVar()
alarmminuteValue.set("error")

alarmState = StringVar()
alarmState.set("error")

celsius = StringVar()
celsius.set("error")

fctemperatureValue = StringVar()
fctemperatureValue.set("error")

fchumidityValue = StringVar()
fchumidityValue.set("error")
statusValue = StringVar()
statusValue.set("error")


hours = Entry(root)
hours.grid(row=2, column=4)

minutes = Entry(root)
minutes.grid(row=3, column=4)

xs = Label(root, text='X :')
xs.grid(row=1, column=0, sticky=W)
xReading = Label(root, textvariable=xValue)
xReading.grid(row=1, column=1, sticky=W)

ys = Label(root, text='Y :')
ys.grid(row=2, column=0, sticky=W)
yReading = Label(root, textvariable=yValue)
yReading.grid(row=2, column=1, sticky=W)

zs = Label(root, text='Z :')
zs.grid(row=3, column=0, sticky=W)
zReading = Label(root, textvariable=zValue)
zReading.grid(row=3, column=1, sticky=W)


tempReadings = StringVar()
tempReadingstext = Label(root, text="Temperature:")
tempReadingstext.grid(row=4, column=0, sticky=W)
temp = Label(root, textvariable=tempValue)
temp.grid(row=4, column=1, sticky=W)
tempmeasurement = Label(root, text="C")
tempmeasurement.grid(row=4, column=2, sticky=W)

humid = Label(root, text="Humidity")
humid.grid(row=5, column=0, sticky=W)
humid = Label(root, textvariable=humidityValue)
humid.grid(row=5, column=1, sticky=W)
humidmeasurement = Label(root, text="%")
humidmeasurement.grid(row=5, column=2, sticky=W)

onboardClockHrTxt = Label(root, text="Device Clock Hour::")
onboardClockHrTxt.grid(row=6, column=0, sticky=W)
onboardClockHr = Label(root, textvariable=hoursValue)
onboardClockHr.grid(row=6, column=1, sticky=W)


onboardClockMinTxt = Label(root, text="Device Clock Min::")
onboardClockMinTxt.grid(row=7, column=0, sticky=W)
onboardClockMin = Label(root, textvariable=minutesValue)
onboardClockMin.grid(row=7, column=1, sticky=W)


onboardClockSecTxt = Label(root, text="Device Clock Sec::")
onboardClockSecTxt.grid(row=8, column=0, sticky=W)
onboardClockSec = Label(root, textvariable=secondsValue)
onboardClockSec.grid(row=8, column=1, sticky=W)



weekdayText = Label(root, text="Weekday: ")
weekdayText.grid(row=9, column=0, sticky=W)
weekday = Label(root, textvariable=weekdayValue)
weekday.grid(row=9, column=1, sticky=W, columnspan=2)

hourtypeText = Label(root, text="Time Setting: ")
hourtypeText.grid(row=10, column=0, sticky=W)
hourtype = Label(root, textvariable=hourTypeValue)
hourtype.grid(row=10, column=1, sticky=W,columnspan=2)


alarmHourtext = Label(root, text="Alarm Hour:: ")
alarmHourtext.grid(row=11, column=0, sticky=W)
alarmHour = Label(root, textvariable=alarmhourValue)
alarmHour.grid(row=11, column=1, sticky=W)


alarmMinText = Label(root, text="Alarm Min::")
alarmMinText.grid(row=12, column=0, sticky=W)
alarmMin = Label(root, textvariable=alarmminuteValue)
alarmMin.grid(row=12, column=1, sticky=W)



alarmstatusText = Label(root, text="Alarm Status:")
alarmstatusText.grid(row=13, column=0, sticky=W)
alarmstatusReading = Label(root, textvariable=alarmState)
alarmstatusReading.grid(row=13, column=1, sticky=W, columnspan=2)


forecast1 = Label(root, text="Forecast Weather: ")
forecast1.grid(row=15, column=0, sticky=W)
forecast1 = Label(root, textvariable=statusValue)
forecast1.grid(row=15, column=1, sticky=W)

forecast2 = Label(root, text="Forecast Temperature:")
forecast2.grid(row=16, column=0, sticky=W)
forecast2 = Label(root, textvariable=fctemperatureValue)
forecast2.grid(row=16, column=1, sticky=W)
forecast2measurement = Label(root, text="C")
forecast2measurement.grid(row=16, column=2, sticky=W)

forecast3 = Label(root, text="Forecast Humidity:")
forecast3.grid(row=17, column=0, sticky=W)
forecast3 = Label(root, textvariable=fchumidityValue)
forecast3.grid(row=17, column=1, sticky=W)
forecast3measurement = Label(root, text="%")
forecast3measurement.grid(row=17, column=2, sticky=W)
baudrate = 9600
port = "COM13"

ser = serial.Serial(port, baudrate, timeout=2)
if ser.isOpen():
    print(ser.name + ' is open.....')
else:
    ser.open()
    print("It just opened")

def main():

    print(data)
    print(data.get('2023-7-6'))

    static_text()

    current_time()
    #t = time.time()
    #clockimport = time.localtime(time.time())
    #clockReading = str(clockimport[3]) + ':' + str(clockimport[4]) + ':' + str(clockimport[5])

    twelveButton = Button(root, text='12 Hour Format', command=twelvehour)
    twelveButton.grid(row=13, column=3)

    twentyfourButton = Button(root, text='24 Hour Format', command=twentyfourhour)
    twentyfourButton.grid(row=12, column=3)

    testbutton = Button(root, text='Test Button', command=test)
    testbutton.grid(row=20, column=0)

    AMButton = Button(root, text='button')
    AMButton.grid()

    syncfcbutton = Button(root, text=' Synchronise Forecast ', command=send_forecast)
    syncfcbutton.grid(row=9, column=3)

    syncCbutton = Button(root, text='   Synchronise Clock   ', command=send_clock)
    syncCbutton.grid(row=8, column=3)

    cButton = Button(root, text='    Set Celsius    ', command=send_C)
    cButton.grid(row=9, column=4)

    fButton = Button(root, text=' Set Fahrenheit ', command=send_F)
    fButton.grid(row=8, column=4)

    setAlarmButton = Button(root, text='Set Alarm', command=send_alarm)
    setAlarmButton.grid(row=5, column=3, columnspan=2)

    root.after(50, decode_stats)

    root.mainloop()

    while True:
        print('hello')
        cmd = input("Enter command or 'exit': ")
        #out = ser.readline()
        #print('Receiving...' + str(out))
        print('receive buffer: ' + str(ser.in_waiting))

        if cmd == 'exit':
            ser.close()
            exit()
        elif cmd == 'clock': # Done
            send_clock(ser)
        elif cmd == 'fc':
            #nothing
            print("hi")

        elif cmd == 'am': # Done
            ser.write("am\0".encode('ascii'))

        elif cmd == 'pm': # Done
            ser.write("pm\0".encode('ascii'))

        elif cmd == 'degrees': # Done
            ser.write("degrees\0".encode('ascii'))

        elif cmd == '12hour': # Done
            ser.write("12hour\0".encode('ascii'))

        elif cmd == '24hour': # Done
            ser.write("24hour\0".encode('ascii'))

        elif cmd == 'setalarm':
            ser.write("setalarm\0".encode('ascii'))

        elif cmd == 'r':

            out = ser.readline()
            print('Receiving...' + str(out))
            print('receive buffer: ' + str(ser.in_waiting))
            print('transmit buffer: ' + str(ser.out_waiting))

        elif cmd == 'receive':
            while True:
                out = ser.read().decode('ascii')
                print('Receiving...' + str(out))

        elif cmd == 're':
            while True:
                out = ser.readline()
                print('Receiving...' + str(out))
                print('receive buffer: ' + str(ser.in_waiting))
                print('transmit buffer: ' + str(ser.out_waiting))

        elif cmd == 'C':
            send_C(ser)
        elif cmd == 'F':
            send_F(ser)
        elif cmd == 'test':
            send_forecast(ser, data)
        elif cmd == 'alarm':
            send_alarm(ser, 1, 1)
        elif cmd == 'erase':
            erase(ser)


        time.sleep(0.1)

def static_text():


    currenttimetext = Label(root, text='Current Time: ',font=("Arial", 12))
    currenttimetext.grid(row = 0, column=3, sticky=W)

    stats = Label(root, text='Statistics', font=("Arial", 12))
    stats.grid(row=0, column=0, columnspan = 2,  sticky=W)

    alarmtext = Label(root, text="Set Alarm:",font=("Arial", 11))
    alarmtext.grid(row=1, column = 3,  sticky=W)

    hourstext = Label(root, text="Hours: ")
    hourstext.grid(row=2, column = 3,  sticky=W)

    minutestext = Label(root, text="Minutes: ")
    minutestext.grid(row=3, column=3,  sticky=W)


def decode_stats():
    #first = ser.readline().decode('ascii')
    if str(ser.in_waiting) != '0':

        first = ser.readline().decode('ascii')
        #print(first)
        a = first.split('+')
        print(a)
        print(str(ser.in_waiting))

        print("foundit")
        if a[0][0] == 'X': # Accelerometer and Onboard Temp/Humidity
            for each_stat in a:
                component = each_stat.partition(':')
                if component[0] == 'X':
                    xValue.set(component[2].strip("\n"))
                elif component[0] == 'Y':
                    yValue.set(component[2].strip("\n"))
                elif component[0] == 'Z':
                    zValue.set(component[2].strip("\n"))
                elif component[0] == 'T':
                    tempValue.set(component[2].strip("\n"))
                elif component[0] == 'H':
                    humidityValue.set(component[2].strip("\n"))
        if a[0][0] == 'h': # Clock Section
            for each_stat in a:
                component = each_stat.partition(':')
                if component[0] == 'h':
                    hoursValue.set(component[2].strip("\n"))
                elif component[0] == 'm':
                    minutesValue.set(component[2].strip("\n"))
                elif component[0] == 's':
                    secondsValue.set(component[2].strip("\n"))
                elif component[0] == 'w':
                    if component[2] == '0':
                        weekdayValue.set("Monday")
                    elif component[2] == '1':
                        weekdayValue.set("Tuesday")
                    elif component[2] == '2':
                        weekdayValue.set("Wednesday")
                    elif component[2] == '3':
                        weekdayValue.set("Thursday")
                    elif component[2] == '4':
                        weekdayValue.set("Friday")
                    elif component[2] == '5':
                        weekdayValue.set("Saturday")
                    elif component[2] == '6':
                        weekdayValue.set("Sunday")

                elif component[0] == 'H':
                    if component[2] == '1\n':
                        hourTypeValue.set("12 Hour")
                    else:
                        hourTypeValue.set("24 Hour")

        if a[0][0] == 'H': # Alarm Section
            for each_stat in a:
                component = each_stat.partition(':')
                if component[0] == 'H': # Alarm Hour
                    alarmhourValue.set(component[2].strip("\n"))
                elif component[0] == 'M': # Alarm Minute
                    alarmminuteValue.set(component[2].strip("\n"))
                elif component[0] == 'S': # Alarm State
                    if component[2] == '0':
                        alarmState.set("DEACTIVATED")
                    elif component[2] == '1':
                        alarmState.set("ARMED")
                    elif component[2] == '2':
                        alarmState.set("ACTIVE")
                elif component[0] == 'C': # Celsius or Fahrenheit
                    celsius.set(component[2].strip("\n"))

        if a[0][0] == 'T': # Forecast Section
            for each_stat in a:
                print(each_stat)
                component = each_stat.partition(':')
                if component[0] == 'T':
                    fctemperatureValue.set(component[2].strip(" \n")) # Forecast Temperature
                elif component[0] == 'H':
                    fchumidityValue.set(component[2].strip("\n")) # Forecast Humidity
                elif component[0] == 's':
                    statusValue.set(component[2].strip("\n")) # Forecast Status

    root.after(50, decode_stats)

def test():
    print('Receiving...')

def twelvehour():
    ser.write("12hour\0".encode('ascii'))


def twentyfourhour():
    ser.write("24hour\0".encode('ascii'))


def send_C():
    ser.write("C\0".encode('ascii'))


def send_F():
    ser.write("F\0".encode('ascii'))


def synchronise():
    print('output = ' + hours.get() + ' and ' + minutes.get())


def send_clock():
    ser.write("clock\0".encode('ascii'))
    print('sending clock')
    tested = time.localtime(time.time())
    print(tested)
    # time.localtime returns tuple
    count = 0
    for x in tested:
        count += 1
        if count >= 8:
            break
        else:
            print(x)
            output = str(x) + '\0'
            ser.write(output.encode('ascii'))


def send_alarm():
    ser.write("alarm\0".encode('ascii'))
    set_hour = hours.get()
    set_min = minutes.get()
    hour_str = str(set_hour) + '\0'
    min_str = str(set_min) + '\0'
    print("sending: "+hour_str)
    print("sending: " + min_str)
    ser.write(hour_str.encode('ascii'))
    ser.write(min_str.encode('ascii'))

def send_forecast():
    print('Transferring weather report')
    # let the atmega328p know its going to get the weekly weather
    ser.write("f\0".encode('ascii'))
    print("send forecast")

    # Send a day's worth of data
    seven_days = 0
    current_date = []
    count = 0
    enable_transfer = 0
    for date in data:
        current_date_data = time.localtime(time.time())
        for something in current_date_data:
            if count == 3:
                break
            # print(something)
            current_date.append(str(something))
            count += 1

        date_values = date.split('-')
        # print(date_values)
        # print(current_date)
        if current_date == date_values:
            enable_transfer = 1

        if enable_transfer == 1:
            if seven_days == 7:
                continue
            print("YEHAW")

            for x in date_values:
                send_date_values = str(x) + '\0'
                ser.write(send_date_values.encode('ascii'))
                # ser.write("12\0".encode('ascii'))
                print('sent ' + send_date_values)
                time.sleep(0.5)

            for stuff in data.get(date):
                # prints variables types (temp, humid, and weather)
                # print (stuff)

                # prints values of variables
                extra = data.get(date).get(stuff)
                sendout = str(extra) + '\0'
                ser.write(sendout.encode('ascii'))
                # ser.write("32\0".encode('ascii'))
                print('sent ' + sendout)
                time.sleep(0.5)

            seven_days += 1


def erase():
    ser.reset_input_buffer()
    ser.reset_output_buffer()


def current_time():
    string = strftime('%H:%M:%S')
    timetext.config(text=string)
    timetext.after(1000, current_time)

# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    main()

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
