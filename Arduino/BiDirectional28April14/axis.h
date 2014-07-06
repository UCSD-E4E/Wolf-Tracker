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

void axisCreate (axis *axis, int servoPin, int nVoltage, int pot, int lim) //Initialize
{
  axis->isCentered = false;

  axis->neutralVoltage = nVoltage;
  axis->potentiometer = pot;
  axis->limit = lim;

  axis->center = analogRead(pot);
  axis->servo.attach(servoPin);
  axis->servo.write(nVoltage);
}

int axisError(axis *axis)
{
  axis->error = axis->center - analogRead(axis->potentiometer);
  return axis->error;
}

//axisCenter

boolean axisOverLimit(axis *axis, int dir)
{
  axis->error = axisError(axis);
  return ((axis->error < -axis->limit && dir > 0) || (axis->error > axis->limit && dir < 0));
  
}

void setCenter(axis *axis)
{
  axis->center = analogRead(axis->potentiometer);
}


