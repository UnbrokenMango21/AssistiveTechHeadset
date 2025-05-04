#include "arduino_secrets.h"

const int NUM_SENSORS = 5;
const int trigPins[NUM_SENSORS]  = {2, 4, 13, 8, 12};    // Trigger pins for sensors
const int echoPins[NUM_SENSORS]  = {3, 7, A1, A2, A3};     // Echo pins for sensors
const int motorPins[NUM_SENSORS] = {5, 6, 9, 10, 11};      // PWM-capable pins for motors

// Effective range for haptic feedback (in cm)
const int minEffectiveDistance = 5;    // Below this, maximum vibration (PWM = 255)
const int maxEffectiveDistance = 100;  // Above this, no vibration (PWM = 0)
const float NO_DETECTION_DISTANCE = 1000.0;  // Fallback distance when no echo is received

// Timing and smoothing parameters
const long TIMEOUT_US = 30000;         // Timeout for pulseIn (30 ms)
int currentPWM[NUM_SENSORS] = {0, 0, 0, 0, 0};  // Holds current PWM values for smoothing
const int pwmStep = 5;                           // PWM step for smoothing transitions

// Reads distance (in cm) from sensor at the given index.
float readDistance(int sensorIndex) {
  long duration;
  float distance;
  
  // Trigger the sensor: send a 10 Âµs pulse
  digitalWrite(trigPins[sensorIndex], LOW);
  delayMicroseconds(2);
  digitalWrite(trigPins[sensorIndex], HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPins[sensorIndex], LOW);
  
  // Read echo duration (with timeout)
  duration = pulseIn(echoPins[sensorIndex], HIGH, TIMEOUT_US);
  
  // If no echo is received, set a high fallback distance
  if (duration == 0) {
    distance = NO_DETECTION_DISTANCE;
  } else {
    // Convert duration to distance in cm: (duration * 0.034) / 2
    distance = (duration * 0.034) / 2.0;
  }
  
  return distance;
}

// Maps the measured distance to a target PWM value.
int calculateTargetPWM(float distance) {
  int targetPWM;
  if (distance < minEffectiveDistance) {
    targetPWM = 255;  // Maximum vibration
  } else if (distance > maxEffectiveDistance) {
    targetPWM = 0;    // No vibration
  } else {
    targetPWM = map((int)distance, minEffectiveDistance, maxEffectiveDistance, 255, 0);
    targetPWM = constrain(targetPWM, 0, 255);
  }
  return targetPWM;
}

void setup() {
  Serial.begin(115200);
  
  // Initialize sensor and motor pins
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    pinMode(motorPins[i], OUTPUT);
  }
}

void loop() {
  // Process each sensor one by one
  for (int i = 0; i < NUM_SENSORS; i++) {
    float distance = readDistance(i);
    int targetPWM = calculateTargetPWM(distance);
    
    // Gradually adjust current PWM toward target PWM for smoother transitions
    if (currentPWM[i] < targetPWM) {
      currentPWM[i] = min(currentPWM[i] + pwmStep, targetPWM);
    } else if (currentPWM[i] > targetPWM) {
      currentPWM[i] = max(currentPWM[i] - pwmStep, targetPWM);
    }
    
    // Write the updated PWM value to the motor
    analogWrite(motorPins[i], currentPWM[i]);
    
    // Slow down sensor updates so changes in intensity are physically noticeable
    delay(100);
  }
}