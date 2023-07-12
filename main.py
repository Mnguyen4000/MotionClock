import serial
import json
import time
import urllib.request
from tkinter import *
from tkinter import ttk


def main():

    url = "https://tp-weather.uqcloud.net/weather.json"

    response = urllib.request.urlopen(url)
    data_json = json.loads(response.read())
    data = data_json.get('weather')
    print(data)
    print(data.get('2023-7-6'))

    baudrate = 9600
    port = "COM13"
    """
    root = Tk()
    root.title("Motion Clock GUI")
    root.geometry("500x300") #Sets window size
    root.resizable(0,0) #Disables window readjusting


    testbutton = Button(root, text='TEST')
    testbutton.grid(row=0, column=0)

    
    t = time.time()
    clockimport = time.localtime(time.time())
    clockReading = str(clockimport[3]) + ':' + str(clockimport[4]) + ':' + str(clockimport[5])
    
    dayReading = "Day of the Week: Monday"
    day = Label(root, text=dayReading)
    day.grid(row=1, column=0, columnspan=4)

    clockReading = "20:32:13"
    time = Label(root, text=clockReading)
    time.grid(row=2, column=0, columnspan=4)


    alarmtext = Label(root, text="Set Alarm:")
    alarmtext.grid(row=3, column = 0)

    hourstext = Label(root, text="Hours: ")
    hourstext.grid(row=4, column = 0)
    hours = Entry(root)
    hours.grid(row=4, column = 1)

    minutestext = Label(root, text="Minutes: ")
    minutestext.grid(row=4, column=2)
    minutes = Entry(root, width=20)
    minutes.grid(row=4, column = 4)

    anotherbutton = Button(root, text='Levels', command=test)
    anotherbutton.grid(row=20, column=0)

    AMButton = Button(root, text='button')
    AMButton.grid()


    stats = Label(root, text='Statistics')
    stats.grid(row=0, column=5)

    accelReadings = 'X: ' + str(1) + ' Y: ' + str(2) + ' Z: ' + str(3)
    accel = Label(root, text=accelReadings)
    accel.grid(row=1, column=5)

    measurement = 'C' #Celcius or Fahrenheit Placeholder
    tempReadings = 'Temperature: ' + str(32.324) + measurement
    temp = Label(root, text=tempReadings)
    temp.grid(row=2, column=5, sticky=W)

    humidityvalue = 23.345
    humidReadings = 'Humidity: ' + str(humidityvalue) + '%'
    humid = Label(root, text = humidReadings)
    humid.grid(row=3, column=5, sticky=W)

    forecastvalue = "Storm"
    forecastReading = "Forecast: " + str(forecastvalue)
    forecast = Label(root, text=forecastReading)
    forecast.grid(row=4, column=5, sticky=W)


    #root.mainloop()
    """
    ser = serial.Serial(port, baudrate, timeout=2)
    if ser.isOpen():
        print(ser.name + ' is open.....')
    else:
        ser.open()
        print("It just opened")





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
            ser.write("C\0".encode('ascii'))
        elif cmd == 'F':
            ser.write("F\0".encode('ascii'))

        elif cmd == 'test':
            send_forecast(ser, data)

        elif cmd == 'erase':
            erase(ser)


        time.sleep(0.1)

def test():
    print("test")
def send_clock(ser):
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


def send_forecast(ser, data):
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


def erase(ser):
    ser.reset_input_buffer()
    ser.reset_output_buffer()

# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    main()

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
