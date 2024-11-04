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
            
            
    while usartPort.in_waiting < 1: #waiting for communication
        time.sleep(0.001)
    
    received = usartPort.read_all().decode()
    print("Decoded:\n")
    print(received)
    while "FWR" not in received:
        if usartPort.in_waiting > 0:
            recByte = usartPort.read(1).decode()
            print(recByte, end="")
            received += recByte
    
    
        
    bufferCounter = 0 #for sending multiple (30) bytes    
    
    nBytes = 0
        
    for byte in firmware.firmware:
        usartPort.write(bytes([byte]))
        time.sleep(0.00001)
        nBytes +=1
        
        bufferCounter += 1
        if bufferCounter >=31:
            bufferCounter = 0
            while usartPort.in_waiting == 0:    #waiting for transmitting data via i2c
                time.sleep(0.00001)
            recByte = usartPort.read().decode()            
            received = recByte            
            while "C" not in received:
                print(recByte, end="")
                if usartPort.in_waiting > 0:
                    recByte = usartPort.read(1).decode(errors="replace")                    
                    received += recByte
        else:
            while usartPort.in_waiting > 0:
                toDecode = usartPort.read()
                try:
                    
                    print(toDecode.decode(), end="")
                except:
                    print(toDecode)            
    print(nBytes)
                
    for byte in firmware.defaultConfig:
        usartPort.write(byte)
        bufferCounter += 1
        if bufferCounter >=31:
            bufferCounter = 0
            while usartPort.in_waiting == 0:    #waiting for transmitting data via i2c
                time.sleep(0.00001)
            received = usartPort.read()    
            print(received.decode(), end="")
            while "C" not in received.decode(): # not acknowlaged with correct char
                if usartPort.in_waiting > 0:
                    recByte = usartPort.read()
                    print(recByte.decode(), end="")
                    received += recByte
           
    while 1:
        if usartPort.in_waiting > 0:
            print(usartPort.read().decode())
            

            
    
