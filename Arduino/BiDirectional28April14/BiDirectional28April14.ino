#include <Servo.h>
#include "axis.h"
#include "motor.h"

/*
TO DO:
Integrate Voltage/Current sensor from other code
Integrate GPS Parsing into Server/Client code
Integrate IMU sensor from other code
*/



char jsBuffer[3]; //Raw Joystick Value - (component, direction, magnitude)
int js; //Joystick value

int tmpPot; //temporary potentiometer values
unsigned long time;

int currentPin = 2;
float current;

int voltPin = 3;
float volts;

boolean isSpeaking = false;
boolean timedOut = false;

axis yaw;
axis pitch;

motor leftDrive;
motor rightDrive;

/**
* Initializes axes, motors, serial connection and timing
*/
void setup()
{
  axisCreate(&yaw, 9, 82, 0, 200);           //Initialize yaw axis (rotation around vertical axis)
  axisCreate(&pitch, 10, 87, 1, 100);       //Initialize pitch axis (rotation from side to side)

  motorCreate(&leftDrive, 5, 12);          //Initialize left motor drive
  motorCreate(&rightDrive, 6, 13);         //Initialize right motor drive

  time = millis();                         //Initialize number of milliseconds since Arduino board began running program

  Serial.begin(9600);                      //Initialize serial connection to 9600 bits per second
}


/**
*Periodically sends data for axes, current and voltage over serial connection 
*Reinitializes axes and motors if heartbeat is not received
*Reinitializes axes if they go over software limits for gimbal movement
*/
void loop()
{
  //Arduino waits for main computer to stop
  if(((millis() - time) > 10) && !isSpeaking)
  {
    time = millis();
    isSpeaking = true; //Arduino has waited for its turn
  }

  //Timeout if a heartbeat has not been recieved
  if ((millis() - time) > 2000 && isSpeaking) //19 April 2014
  {
    timedOut = true; 
  }
  
  if(timedOut) //only broken by the heartbeat
  {
    //Serial.println("Out");

    pitch.servo.write(pitch.neutralVoltage);   //Set pitch to neutral voltage
    yaw.servo.write(yaw.neutralVoltage);       //Set yaw to neutral voltage


    stopMotors(&leftDrive);                    //set x velocity, y velocity and PWM Pin of left motor to 0
    stopMotors(&rightDrive);                  //set x velocity, y velocity and PWM Pin of left motor to 0

    //pitch.voltage = pitch.neutralVoltage;
    //yaw.voltage = yaw.neutralVoltage;

    //timedOut = false;
    //time = millis();
  }
  else
  {
    //Software Limits to gimbal movement
    if(axisOverLimit(&pitch, pitch.voltage-pitch.neutralVoltage))
    {
      pitch.servo.write(pitch.neutralVoltage); //reset pitch to neutral voltage
    }

    if(axisOverLimit(&yaw, yaw.voltage-yaw.neutralVoltage))
    {
      yaw.servo.write(yaw.neutralVoltage);   //reset yaw to neutral voltage
    }



    //Send sensor data
    if(isSpeaking)
    {
      //Potentiometers
      tmpPot = analogRead(0);              //read value from pin 0
      if (abs(yaw.lastPot - tmpPot) > 2)   //update and print new yaw value if there is significant differnce since last reading
      {
        yaw.lastPot = tmpPot;              
        Serial.print("Y");
        Serial.println(yaw.lastPot);
      }

      tmpPot = analogRead(1);              //read value from pin 1
      if(abs(pitch.lastPot - tmpPot) > 2)  //update and print new pitch value if there is significant differnce since last reading
      {
        pitch.lastPot = tmpPot;
        Serial.print("P");
        Serial.println(pitch.lastPot);
      }

      //Current & Voltage Sensor every .195 seconds
      if((millis()-time)%200 > 190)
      {
        current = analogRead(currentPin)/49.44;
        Serial.print("I");
        Serial.println(current);


        volts = analogRead(voltPin)*(5.0/1024.0)*6;
        Serial.print("V");
        Serial.println(volts);
      }

      //Accelerometer

    }
  }

  //Process sensor data

}

/**
* Sends data from serial connection to control gimbal movements (X-axis, Y-axis and centering)
* Sends data from serial connection to control robot movements (velocity and turning)
*/
void serialEvent() { //When Serial data is recieved

  while (Serial.available() > 0) //find # of bits available to read 
  {
    Serial.readBytes(jsBuffer, 3); //put into Joystick Buffer
  }


  //Heartbeat from main computer 19 April 2014
  if(jsBuffer[0] == 'H')
  {
    timedOut = false; //remove timed out state
    time = millis(); //set timestamp, ONLY with a heartbeat
  }




  //Decode magnitude
  js = jsBuffer[2]-'0';

  if(jsBuffer[1] == '-')
  {
    js *= -1; //Apply direction
  }



  //Gimbal X-Axis Movement
  if(jsBuffer[0] == 'X')
  {
    tmpPot = analogRead(0); //getError(&Yaw)
    yaw.error = (yaw.center - tmpPot);
    if((yaw.error >  -yaw.limit || js < 0) && (yaw.error < yaw.limit || js > 0))//!axisOverLimit(&Yaw)
    { //Apply Joystick value
      yaw.voltage = yaw.neutralVoltage+js;
      yaw.servo.write(yaw.voltage);
    } 
    else //stay neutral
    {
      yaw.servo.write(yaw.neutralVoltage);
    }
  }

  //Gimbal Y-Axis Movement
  else if(jsBuffer[0] == 'Y')
  {
    tmpPot = analogRead(1);  //getError(&Pitch)
    pitch.error = (pitch.center - tmpPot);
    if((pitch.error >  -pitch.limit || js < 0) && (pitch.error < pitch.limit || js > 0))//!axisOverLimit(&Pitch)
    {//Apply Joystick value
      pitch.voltage = pitch.neutralVoltage+js;
      pitch.servo.write(pitch.voltage);
    }
    else //stay neutral
    {
      pitch.servo.write(pitch.neutralVoltage);
    }
  }

  //Redefine Center for Gimbal 19 April 2014
  else if(jsBuffer[0] == 'R')
  {
    setCenter(&yaw);
    setCenter(&pitch); 
  }

  //User indicates centering gimbal
  //Implements steps for centering gimbal if C is read as component value from joystick 
  else if (jsBuffer[0] == 'C')
  {
    yaw.lastPot = analogRead(0); //read value from pin 0
    pitch.lastPot = analogRead(1); //read value from pin 1
    while (!pitch.isCentered || !yaw.isCentered)
    {
      //How Correction works:
      //1. Get potentiometer error
      //2. Check if error equals zero
      //3. Add/Subtract to the neutral voltage, sets axis correction value
      // - Depends on if servo needs a voltage greater than or less than its neutralVoltage    
      //4. In the next iteration of the loop, write the correction value to servo
      if (!pitch.isCentered)
      {
        pitch.servo.write(pitch.correction);  //set pitch to value of its correction member (is pitch.correction ever initialized?)

        tmpPot = analogRead(1);              //read value from pin 1

        pitch.error = (pitch.center - tmpPot);  //set error as the difference between the center and the value read from pin 1
        pitch.correction = pitch.neutralVoltage + pitch.error;  //set correction as sum of error and neutral voltage
        pitch.lastPot = tmpPot;             //record most recent value from pin 1
      }
      if(pitch.lastPot == pitch.center)//if pitch is centered satisfy first condition of loop and reset to neutral voltage
      {
        pitch.isCentered = true; 
        pitch.servo.write(pitch.neutralVoltage);
      }

      if (!yaw.isCentered)
      {
        yaw.servo.write(yaw.correction); //set yaw to value of its correction member (is yaw.correction ever initialized?)

        tmpPot = analogRead(0);          //read value from pin 0

        yaw.error = (yaw.center - tmpPot); //set error as the difference between the center and the value read from pin 0
        yaw.correction = yaw.neutralVoltage + yaw.error; //set correction as sum of error and neutral voltage
        yaw.lastPot = tmpPot; //record most recent value from pin 1
      }
      if(yaw.lastPot == yaw.center) //if yaw is centered satisfy second condition of loop and reset to neutral voltage
      {
        yaw.isCentered = true;
        yaw.servo.write(yaw.neutralVoltage);
      }
    }
    //reset pitch and yaw to uncentered for next time loop will be implemented
    yaw.isCentered = false;
    pitch.isCentered = false;
  }

  //Robot forward velocity
  else if(jsBuffer[0] == 'y')
  {
    leftDrive.yVel = -js;
    rightDrive.yVel = js;
  }

  //Robot turn
  else if (jsBuffer[0] == 'x')
  {
    leftDrive.xVel = js;
    rightDrive.xVel = js;
  }

  //Drives motors at speed specified by joystick
  driveMotors(&leftDrive);
  driveMotors(&rightDrive);

  isSpeaking = false;
}
