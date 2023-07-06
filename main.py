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
    print(data.get('2023-7-6'))

    baudrate = 9600
    port = "COM13"

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
            #ser.reset_input_buffer()
            #ser.reset_output_buffer()
            #import forecast information
            print('Transferring weather report')
            # let the atmega328p know its going to get the weekly weather
            ser.write("forecast\0".encode('ascii'))

            #Send a day's worth of data
            seven_days = 0
            current_date = []
            count = 0
            enable_transfer = 0
            for date in data:
                #ser.reset_input_buffer()
                #ser.reset_output_buffer()
                #print(date)
                current_date_data = time.localtime(time.time())
                #print(current_date_data)
                #print('above me is the date')
                # time.localtime returns tuple

                for something in current_date_data:
                    if count == 3:
                        break
                    #print(something)
                    current_date.append(str(something))
                    count += 1

                date_values = date.split('-')
                #print(date_values)
                #print(current_date)
                if current_date == date_values:
                    enable_transfer = 1

                if enable_transfer == 1:
                    if seven_days == 7:
                        continue
                    print("YEHAW")
                    for x in date_values:
                        send_date_values = str(x) + '\0'
                        ser.write(send_date_values.encode('ascii'))
                        ser.write("12\0".encode('ascii'))
                        print('sent ' + send_date_values)
                        time.sleep(0.1)

                    x = ser.readline().decode('ascii')
                    hmm = str(x).endswith('next\n') # sometimes error is placed in uart line, only last part counts
                    print('1st round: ' + str(x))
                    while hmm == False:
                        # block until it receives confirmation
                        time.sleep(1)
                        print('receive buffer: ' + str(ser.in_waiting))
                        x = ser.readline()
                        print('something: ' + str(x))
                        print('transmit buffer: ' + str(ser.out_waiting))
                        print('waiting')
                        print('ya')
                        continue

                    for stuff in data.get(date):
                        # prints variables types (temp, humid, and weather)
                        #print (stuff)

                        # prints values of variables
                        values = data.get(date).get(stuff)
                        sendout = str(values) + '\0'
                        #ser.write(sendout.encode('ascii'))
                        ser.write("32\0".encode('ascii'))
                        print('sent ' + sendout)
                        time.sleep(0.1)

                    x = ser.readline().decode('ascii')
                    hmm = str(x).endswith('next\n') # sometimes error is placed in uart line, only last part counts
                    print('2nd round: ' + str(x))
                    while hmm == False:
                        # block until it receives confirmation
                        time.sleep(1)
                        print('receive buffer: ' + str(ser.in_waiting))
                        x = ser.readline()
                        print('something: ' + str(x))
                        print('transmit buffer: ' + str(ser.out_waiting))
                        print('waiting')
                        print('ya')
                        continue

                    seven_days += 1

                    #print("received next confirmation")
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

        elif cmd == 'r':

            out = ser.readline()
            print('Receiving...' + str(out))
            print('receive buffer: ' + str(ser.in_waiting))
            print('transmit buffer: ' + str(ser.out_waiting))
        elif cmd == 'receive':
            while True:
                out = ser.read().decode('ascii')
                print('Receiving...' + str(out))

        elif cmd == 'test':
            """
            ser.write("test\0".encode('ascii'))
            ser.write("jello\0".encode('ascii'))
            ser.write("jell2o\0".encode('ascii'))
            ser.write("jell3o\0".encode('ascii'))
            ser.write("jel4lo\0".encode('ascii'))
            ser.write("je5llo\0".encode('ascii'))
            """
            #ser.reset_input_buffer()
            #ser.reset_output_buffer()
            #import forecast information
            print('Transferring weather report')
            # let the atmega328p know its going to get the weekly weather
            ser.write("f\0".encode('ascii'))
            print("send forecast")

            #Send a day's worth of data
            seven_days = 0
            current_date = []
            count = 0
            enable_transfer = 0
            for date in data:
                #ser.reset_input_buffer()
                #ser.reset_output_buffer()
                #print(date)
                current_date_data = time.localtime(time.time())
                #print(current_date_data)
                #print('above me is the date')
                # time.localtime returns tuple

                for something in current_date_data:
                    if count == 3:
                        break
                    #print(something)
                    current_date.append(str(something))
                    count += 1

                date_values = date.split('-')
                #print(date_values)
                #print(current_date)
                if current_date == date_values:
                    enable_transfer = 1

                if enable_transfer == 1:
                    if seven_days == 7:
                        continue
                    print("YEHAW")

                    for x in date_values:
                        send_date_values = str(x) + '\0'
                        ser.write(send_date_values.encode('ascii'))
                        #ser.write("12\0".encode('ascii'))
                        print('sent ' + send_date_values)
                        time.sleep(0.5)


                    for stuff in data.get(date):
                        # prints variables types (temp, humid, and weather)
                        #print (stuff)

                        # prints values of variables
                        extra = data.get(date).get(stuff)
                        sendout = str(extra) + '\0'
                        ser.write(sendout.encode('ascii'))
                        #ser.write("32\0".encode('ascii'))
                        print('sent ' + sendout)
                        time.sleep(0.5)

                    seven_days += 1

        elif cmd == 'erase':
            ser.reset_input_buffer()
            ser.reset_output_buffer()


        time.sleep(0.1)



# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    main()

# See PyCharm help at https://www.jetbrains.com/help/pycharm/
