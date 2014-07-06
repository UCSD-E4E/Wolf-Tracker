#include <Arduino.h>

typedef struct
{
  int PWM_Pin;
  int directionPin;
  int xVel;
  int yVel;
  int vel;
}
motor;

void motorCreate(motor *motor, int PWM_P, int DirPin) //Initialize
{
  pinMode(PWM_P, OUTPUT);
  pinMode(DirPin, OUTPUT);

  motor->PWM_Pin = PWM_P;
  motor->directionPin = DirPin;
  motor->xVel = 0;
  motor->yVel = 0;
  motor->vel = 0;
}

void driveMotors(motor *motor)
{
  motor->vel = motor->xVel + motor->yVel;

  if(motor->vel > 0)
  {
    digitalWrite(motor->directionPin, HIGH);
  }
  else
  {
    digitalWrite(motor->directionPin, LOW);
    motor->vel *= -1;
  }
  analogWrite(motor->PWM_Pin, motor->vel*18);
}

void stopMotors(motor *motor)
{
  analogWrite(motor->PWM_Pin, 0);
  /*
  motor->xVel = 0;
  motor->yVel = 0;
  driveMotors(motor);
  */
}
