import serial
from serial.tools import list_ports
import time
import firmware



if __name__ == "__main__":   
    ports = []
    for port in list_ports.comports():
        try:
            usartPort = serial.Serial(port=port.device, baudrate=115200, timeout=100)
            ports.append(port.description)
            print(port.device)
        except:
            print(port.description, " is busy")
            
            
    while usartPort.in_waiting < 3: #waiting for Firmware request
        time.sleep(0.001)
    
    if usartPort.read(3) != "FWR":
        print("Error during communication")
        
    bufferCounter = 0 #for sending multiple (30) bytes    
        
    for byte in firmware.firmware:
        usartPort.write(byte)
        bufferCounter += 1
        if bufferCounter >=30:
            bufferCounter = 0
            while usartPort.in_waiting == 0:    #waiting for transmitting data via i2c
                time.sleep(0.00001)
            if usartPort.read() != "C":
                print("Error during communication - not acknowlaged with correct char")
                
    for byte in firmware.defaultConfig:
        usartPort.write(byte)
        bufferCounter += 1
        if bufferCounter >=30:
            bufferCounter = 0
            while usartPort.in_waiting == 0:    #waiting for transmitting data via i2c
                time.sleep(0.00001)
            if usartPort.read() != "C":
                print("Error during communication - not acknowlaged with correct char")
            

            
    
