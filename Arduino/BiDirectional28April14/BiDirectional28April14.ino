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

void setup()
{
  axisCreate(&yaw, 9, 82, 0, 200);
  axisCreate(&pitch, 10, 87, 1, 100);

  motorCreate(&leftDrive, 5, 12);
  motorCreate(&rightDrive, 6, 13);

  time = millis();

  Serial.begin(9600);
}

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

    pitch.servo.write(pitch.neutralVoltage);
    yaw.servo.write(yaw.neutralVoltage);


    stopMotors(&leftDrive);
    stopMotors(&rightDrive);

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
      pitch.servo.write(pitch.neutralVoltage); 
    }

    if(axisOverLimit(&yaw, yaw.voltage-yaw.neutralVoltage))
    {
      yaw.servo.write(yaw.neutralVoltage);
    }



    //Send sensor data
    if(isSpeaking)
    {
      //Potentiometers
      tmpPot = analogRead(0);
      if (abs(yaw.lastPot - tmpPot) > 2)
      {
        yaw.lastPot = tmpPot;
        Serial.print("Y");
        Serial.println(yaw.lastPot);
      }

      tmpPot = analogRead(1);
      if(abs(pitch.lastPot - tmpPot) > 2)
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

void serialEvent() { //When Serial data is recieved

  while (Serial.available() > 0)
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
    tmpPot = analogRead(1);
    pitch.error = (pitch.center - tmpPot);
    if((pitch.error >  -pitch.limit || js < 0) && (pitch.error < pitch.limit || js > 0))
    {
      pitch.voltage = pitch.neutralVoltage+js;
      pitch.servo.write(pitch.voltage);
    }
    else
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
  else if (jsBuffer[0] == 'C')
  {
    yaw.lastPot = analogRead(0);
    pitch.lastPot = analogRead(1);
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
        pitch.servo.write(pitch.correction);

        tmpPot = analogRead(1);

        pitch.error = (pitch.center - tmpPot);
        pitch.correction = pitch.neutralVoltage + pitch.error;
        pitch.lastPot = tmpPot;
      }
      if(pitch.lastPot == pitch.center)
      {
        pitch.isCentered = true; 
        pitch.servo.write(pitch.neutralVoltage);
      }

      if (!yaw.isCentered)
      {
        yaw.servo.write(yaw.correction);

        tmpPot = analogRead(0);

        yaw.error = (yaw.center - tmpPot);
        yaw.correction = yaw.neutralVoltage + yaw.error;
        yaw.lastPot = tmpPot;
      }
      if(yaw.lastPot == yaw.center)
      {
        yaw.isCentered = true;
        yaw.servo.write(yaw.neutralVoltage);
      }
    }
    yaw.isCentered = false;
    pitch.isCentered = false;
  }

  //Robot forward velocity
  else if(jsBuffer[0] == 'y')
  {
    leftDrive.yVel = js;
    rightDrive.yVel = -js;
  }

  //Robot turn
  else if (jsBuffer[0] == 'x')
  {
    leftDrive.xVel = js;
    rightDrive.xVel = js;
  }

  driveMotors(&leftDrive);
  driveMotors(&rightDrive);

  isSpeaking = false;
}
