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

/**
*Initializes velocity members of motor to 0 and sets PWM pin and *direction pins as the pins passed into tthe function
*/

void motorCreate(motor *motor, int PWM_P, int DirPin) 
{
  pinMode(PWM_P, OUTPUT); //sets PWM Pin as output pin
  pinMode(DirPin, OUTPUT); //sets direction pin as output pin

  motor->PWM_Pin = PWM_P;
  motor->directionPin = DirPin;
  motor->xVel = 0;
  motor->yVel = 0;
  motor->vel = 0;
}

/**
*Drives motor by generating steady square wave on PWM pin at 
*duty cycle value(proportion of on time to regular time interval)  
*/

void driveMotors(motor *motor)
{

//sets motor velocity as sum offorward velocity and turning

  motor->vel = motor->xVel + motor->yVel; 

  if(motor->vel > 0)
  {

//turns the directional pin on

    digitalWrite(motor->directionPin, HIGH); 
  }
  else
  {

//turns the directional pin off

    digitalWrite(motor->directionPin, LOW);

//makes negative motor velocity positive

    motor->vel *= -1;
  }

//writes duty cycle to PWM pin by multiplying velocity by 18

  analogWrite(motor->PWM_Pin, motor->vel*18);
}

/*
*Stops motor by setting duty cycle to always off
*/

void stopMotors(motor *motor)
{
  analogWrite(motor->PWM_Pin, 0); //set duty cycle to always off
  /*
  motor->xVel = 0;
  motor->yVel = 0;
  driveMotors(motor);
  */
}

