#include <Arduino.h>

typedef struct
{
  Servo servo;
  boolean isCentered;
  int neutralVoltage;
  int voltage;
  int center;
  int error;
  int correction;
  int potentiometer;
  int limit;
  int lastPot;
}
axis;

/*
*Initializes axis members with values passed into function
*Members intialized: neutralVoltage, potentiometer, limit, *center, and servo
*/

void axisCreate (axis *axis, int servoPin, int nVoltage, int pot, int lim) 
{
  axis->isCentered = false;	    

  axis->neutralVoltage = nVoltage;
  axis->potentiometer = pot;
  axis->limit = lim;

  axis->center = analogRead(pot); //sets to potentiometer value
  axis->servo.attach(servoPin); //attaches servo to a pin
  axis->servo.write(nVoltage); //set axis to given voltage
}

/*
*Calculates axis error as the difference between the center and *the value of the potentiometer
*/

int axisError(axis *axis)
{
  axis->error = axis->center - analogRead(axis->potentiometer);
  return axis->error;
}

/*
*Determines whether gimbal movement is above software limits
*parameters: axis and direction (voltage of the axis)
*this function returns true: 
*IF 
*axis error < -limit AND voltage > 0
*OR
*axis error > -limit AND voltage < 0
*/

boolean axisOverLimit(axis *axis, int dir)
{
  axis->error = axisError(axis); //calculates axis error
  return ((axis->error < -axis->limit && dir > 0) || (axis->error > axis->limit && dir < 0));
  
}

/*
*Sets value of center to value read from potentiometer
*/
void setCenter(axis *axis)
{
  axis->center = analogRead(axis->potentiometer);
}


