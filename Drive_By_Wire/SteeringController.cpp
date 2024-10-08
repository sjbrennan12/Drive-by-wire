#include <Arduino.h>
#include "PID_v1.h"
#include "DBW_Pins.h"
#include "Settings.h"
#include "SteeringController.h"

SteeringController::SteeringController()
  : steerPID(&steerAngleUS, &PIDSteeringOutput_us, &desiredTurn_us, proportional_steering, integral_steering, derivative_steering, DIRECT) {
  //Hacky fix for not burning servo circuit
  pinMode(STEER_ON_PIN, OUTPUT);
  #if DBWversion == 4 
    digitalWrite(STEER_ON_PIN, HIGH);
  #endif
  pinMode(25, OUTPUT);// h-bridge right turn
  pinMode(27,OUTPUT);// h-bridge left turn
  
  digitalWrite(25,LOW);
  digitalWrite(27,LOW);
  
  delay(10);
  waitCycles = 0;// used to extend delay in turning to prevent problems with the steering board


  steerPID.SetOutputLimits(MIN_TURN_MS, MAX_TURN_MS);
  steerPID.SetSampleTime(PID_CALCULATE_TIME);
  steerPID.SetMode(AUTOMATIC);

  //Steer_Servo.attach(STEER_PULSE_PIN);
  delay(1);
  //Steer_Servo.write(90);
  if (DEBUG) {
    Serial.println("Steering Setup Complete");
  }
}

SteeringController::~SteeringController() {
}

/** 
 *  
 *  @param desiredAngle 
 *  !UPDATE THIS OBSERVED INFO!

Current Calibration: (Last Update: May 14, 2023)
779 - 1000us (left)
722 - 1500us (middle)
639 - 1850us (right)

Update 4/18/2024
770 left
710 center 
610 right

Update 4/19/2024 Left Sensor is now working
630 left
315 center
91 right

Main function to call the steering
*/
int32_t SteeringController::update(int32_t desiredAngle) { // Summer 2024 Steering PID is disabled and replaced with basic control system that has a threshold 
  // desiredAngle = map(desiredAngle, MIN_TURN_Mdegrees, MAX_TURN_Mdegrees, MIN_TURN_MS, MAX_TURN_MS); // old settings
  int32_t mappedAngle = desiredAngle;

  //if (desiredAngle > 722) { 
    //mappedAngle = map(desiredAngle, 779, 722, MIN_TURN_MS, 1500); 
  //} else {
    //mappedAngle = map(desiredAngle, 722, 639, 1500, MAX_TURN_MS);
  //}

  //if (USE_PIDS) {
    //SteeringPID(mappedAngle);
  //} else {
    engageSteering(mappedAngle); 
  //}
  
  delay(1);

  //return map(currentSteeringUS, MIN_TURN_MS,MAX_TURN_MS,MIN_TURN_Kdegrees,MAX_TURN_Kdegrees); // old settings

  //return desiredAngle;
  steerAngleUS = computeAngleLeft(); // uncomment when testing with sensors
  return steerAngleUS;
}

//Private
void SteeringController::SteeringPID(int32_t input) {
  desiredTurn_us = input;
  //no need for steerPID.compute() ???
  if (PIDSteeringOutput_us != currentSteeringUS) {
    Steer_Servo.writeMicroseconds(PIDSteeringOutput_us);
    currentSteeringUS = PIDSteeringOutput_us;
  }
}

void SteeringController::engageSteering(int32_t input) {
  
  if (abs(currentAngle - input) < threshold) {
    digitalWrite(27, LOW);
    digitalWrite(25, LOW);
    steeringMode = 0; // no turn indicator 
    waitCycles ++; 
  } else if (input > currentAngle) {// left turn 
    if(digitalRead(25) == HIGH)// check if turning right and set steering to off before turning left
    {
    digitalWrite(27, LOW);
    digitalWrite(25, LOW);
    steeringMode = 0; // no turn indicator
    waitCycles = 0;
    }
    else if(waitCycles >= 5)
    {
    digitalWrite(27, HIGH);
    digitalWrite(25, LOW);
    steeringMode = 1; // left turn indicator
    
    }
    else
    {
      waitCycles ++;
    }
  } else {//right turn
  if(digitalRead(27) == HIGH)// check if turning left and set steering to off before turning right
    {
    digitalWrite(27, LOW);
    digitalWrite(25, LOW);
    steeringMode = 0; // no turn indicator
    waitCycles = 0;
    }
    else if(waitCycles >= 5)
    {
    digitalWrite(27, LOW);
    digitalWrite(25, HIGH);
    steeringMode = -1;// right turn indicator
    
    }
    else
    {
      waitCycles ++;
    }
  }

  if (DEBUG) {
  Serial.print("Steering Mode: ");
  Serial.println(steeringMode);
  }
  //  Steer_Servo.writeMicroseconds(input);
  delay(1);
  
}

// Calculates the angle from left sensor
int32_t SteeringController::computeAngleLeft() { // issues with sensor
  int32_t val = analogRead(R_SENSE_PIN);// change for final version  
 // val = map(val, Left_Read_at_MIN_TURN, Left_Read_at_MAX_TURN, MIN_TURN_MS, MAX_TURN_MS);
  if(DEBUG){
    Serial.print("Left sensor: ");
    Serial.println(val);
  } 

  return val;
}

// Calculates the angle from right sensor
int32_t SteeringController::computeAngleRight() {
  int32_t val = analogRead(R_SENSE_PIN);
  currentAngle = val;
  //val = map(val, Right_Read_at_MIN_TURN, Right_Read_at_MAX_TURN, MIN_TURN_MS, MAX_TURN_MS);
  if(DEBUG){
 Serial.print("Right sensor: ");
 Serial.println(val);
 } 
  return val;
}

int16_t SteeringController::getSteeringMode()
{
  return  steeringMode;

}
