import serial
import json
import time
import urllib.request


def main():
    url = "https://tp-weather.uqcloud.net/weather.json"

    response = urllib.request.urlopen(url)
    data_json = json.loads(response.read())
    data = data_json.get('weather')
    print(data)
    print(data.get('2023-6-30'))

    baudrate = 9600
    port = "COM13"

    ser = serial.Serial(port, baudrate, timeout=2)
    if ser.isOpen():
        print(ser.name + ' is open.....')
    else:
        ser.open()
        print("It just opened")


    while True:

        cmd = input("Enter command or 'exit': ")
        if cmd == 'exit':
            ser.close()
            exit()
        elif cmd == 'clock':
            ser.write("clock\0".encode('ascii'))
            print('sending clock')
            tested = time.localtime(time.time())
            print(tested)
            #time.localtime returns tuple
            count = 0
            for x in tested:
                count += 1
                if count >= 8:
                    break
                else:
                    print(x)
                    output = str(x)+'\0'
                    ser.write(output.encode('ascii'))

        elif cmd == 'fc':
            #import forecast information
            print('Transferring weather report')
            # let the atmega328p know its going to get the weekly weather
            ser.write("weather\0".encode('ascii'))

            #Send a day's worth of data
            for date in data:
                print(date)
                ser.write(date.encode('ascii'))
                for stuff in data.get(date):
                    # prints variables types (temp, humid, and weather)
                    #print (stuff)

                    # prints values of variables
                    values = data.get(date).get(stuff)
                    print(values)
                    sendout = str(values) + '\0'
                    ser.write(sendout.encode('ascii'))

                while (ser.readline() != 'next\n'):
                    #block until it receives confirmation
                    continue

                    #if received confirmation, it will keep going until all data is gone

                    #Receive confirmation of it being received and fully processed

                    #Send next day's worth of data

                    #receive

                    #repeat

                    #final receive tells thats they received all 7 days

        elif cmd == 'am':
            ser.write("am\0".encode('ascii'))

        elif cmd == 'pm':
            ser.write("pm\0".encode('ascii'))

        elif cmd == 'degrees':
            ser.write("degrees\0".encode('ascii'))

        elif cmd == '12hour':
            ser.write("12hour\0".encode('ascii'))

        elif cmd == '24hour':
            ser.write("24hour\0".encode('ascii'))

        elif cmd == 'setalarm':
            ser.write("setalarm\0".encode('ascii'))

        elif cmd == 'receive':
            out = ser.readline()
            print('Receiving...' + str(out))
        elif cmd == 'test':
            print ('nothing')




        time.sleep(1)



# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    main()

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
