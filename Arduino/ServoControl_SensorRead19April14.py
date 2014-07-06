"""
Current Code
"""

import serial, pygame, time

#ser = serial.Serial('/dev/cu.usbmodem411', 9600) #Arduino Mega
#ser = serial.Serial('/dev/cu.usbserial-A900GZPW', 9600) #OSEPP Mega
ser = serial.Serial('/dev/cu.usbserial-A6008e6L', 9600) #Arduino Deumilanove
#ser = serial.Serial('/dev/tty.usbmodem411', 9600) #Arduino Uno
#ser = serial.Serial('/dev/cu.usbmodem2411', 9600) #Galileo

pygame.init()

joystick = pygame.joystick.Joystick(0)
joystick.init()
threshold = .2

outputs = ["x", "y", "X", "Y"]
outputBuffer = ["x00", "y00", "X00", "Y00"]
currentState = ["x00", "y00", "X00", "Y00"]

print joystick.get_name()
print joystick.get_id()
print joystick.get_axis(0)

lastHeartbeat = time.time()

currentData = open("currentData.txt", "w")
I_time = open("currentTime.txt", "w")

voltData = open("voltData.txt", "w")
V_time = open("voltTime.txt", "w")

V_lastTime = time.time()
I_lastTime = time.time()

def controllerData(index, axis, outputs):
    
    outputChar = outputs[index] #Serial communcation needs a string, not an int. Use arbitrary char for neutral
    if axis[index] > threshold:    
        outputChar += '+' + str(int((axis[index] - threshold)*10))

    elif axis[index] < -threshold:
        outputChar += '-' + str(abs(int((axis[index] + threshold)*10)))
    else:
        outputChar += "00"
   
    return outputChar
    
while True:
    try:
        pygame.event.pump()
        axis = [joystick.get_axis(0), joystick.get_axis(1), joystick.get_axis(2),joystick.get_axis(3)]
        #left joystick [0,1] & right joystick [2,3]
        button = [joystick.get_button(1), joystick.get_button(2)]
        #joystick button, 2 on normal ps3 controller

        #Axis movement
        for x in range(4):
            currentState[x] = controllerData(x, axis, outputs)
            if(outputBuffer[x] != currentState[x]):
                outputBuffer[x] = currentState[x]
                print outputBuffer[x]
                ser.write(outputBuffer[x])

        #Center Signaled
        if button[1]:
            ser.write("C00")
            print "Centering Turret"

        #Redefine Center
        if button[0]:
            ser.write("R00")
            print "Redefine Center"
       
        #Read Serial Data    
        if(ser.inWaiting()):            
            sensorIn = ser.readline()
            print sensorIn
            
            if sensorIn[0] == 'I':
                currentData.write(sensorIn)
                I_lastTime += time.time()
                I_time.write(str(time.time() - I_lastTime)+'\n')
                
                
            if sensorIn[0] == 'V':
                voltData.write(sensorIn)
                V_lastTime += time.time()
                V_time.write(str(time.time() - V_lastTime)+'\n')
                
            

            

        #Heartbeat
        if((time.time() - lastHeartbeat) > .5):
            lastHeartbeat = time.time()
            print "HBT"
            ser.write("HBT")
        

    except KeyboardInterrupt: #use control-C to end serial communcation. Closing python takes too long
        currentData.write("End")
        I_time.write("End")
        voltData.write("End")
        V_time.write("End")

        currentData.close()
        I_time.close()
        voltData.close()
        V_time.close()
        print("\nExiting")
        raise
