"""
    Program to read and save data from Sensirion SPS30 sensor

    by
    Szymon Jakubiak
    Twitter: @SzymonJakubiak
    LinkedIn: https://pl.linkedin.com/in/szymon-jakubiak

    MIT License

    Copyright (c) 2018 Szymon Jakubiak
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    Units for measurements:
        PM1, PM2.5, PM4 and PM10 are in ug/m^3, numerical concentrations are in #/cm^3

    20240209 Matthias Prinke    Created from log_1_sec.py for passive monitoring of the
                                SPS30 UART TX output in a
                                Bresser Air Quality PM2.5/PM10 Sensor (PN 7009970).
                                The aim is to find the actual sensor data in the payload
                                of the 868 MHz FSK radio message.
    
"""
import sps30, time

# Specify serial port name for sensor
# i.e. "COM10" for Windows or "/dev/ttyUSB0" for Linux
device_port = "/dev/ttyUSB0"

# Specify output file name and comment about experiment
output_file = "SPS30_output.txt"
comment = "Experiment ID"

# Create a header in file with output data
date = time.localtime()
date = str(date[0]) + "/" + str(date[1]) + "/" + str(date[2])
header = "\n" + "* * * *\n\n" + date
if len(comment) > 0:
    header += "\n" + comment
header += "\n\n" + "* * * *\n\n"
header += "Date,Time,Mass,,,,Number\n"
header += "yyyy/m/d,h:m:s,PM1,PM2.5,PM4,PM10,0.3÷0.5,0.3÷1,0.3÷2.5,0.3÷4,0.3÷10,typical size\n"
header += ",,ug/m^3,ug/m^3,ug/m^3,ug/m^3,#/cm^3,#/cm^3,#/cm^3,#/cm^3,#/cm^3,um\n"
print(header)
file = open(output_file, "a")
file.write(header)
file.close()

sensor = sps30.SPS30(device_port)

try:
    avg = None
    while True:
        start = time.time()
        output = sensor.monitor_values()
        end = time.time()
        delta = end - start
        sensorData = ""

        # Averaging of sensor data sent in blocks
        # separated by a gap in the serial transmission
        if delta > 0.08:
            if avg:
                print("               AVG - ", end='')
                avgData = ""
                for i, val in enumerate(avg):
                    if i < 4:
                       avgData += "{0},".format(round(val))
                    else:
                        avgData += "{0:.2f},".format(val)
                print(avgData[:-1])
            avg = list(output)
        else:
            if avg:
                for i in range(len(output)):
                    avg[i] = (avg[i] + output[i]) / 2
        for val in output:
            sensorData += "{0:.2f},".format(val)

                
        date = time.localtime()
        act_date = str(date[0]) + "/" + str(date[1]) + "/" + str(date[2])
        act_time = str(date[3]) + ":" + str(date[4]) + ":" + str(date[5])

        output_data = act_date + "," + act_time + " - " + sensorData[:-1] # remove comma from the end
        
        file = open(output_file, "a")
        file.write(output_data + "\n")
        file.close()
        print(output_data)

        time.sleep(1)

except KeyboardInterrupt:
    sensor.close_port()
    print("Data logging stopped")
