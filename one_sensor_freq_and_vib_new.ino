#include "arduino_secrets.h"

const int NUM_SENSORS = 5;
const int trigPins[NUM_SENSORS]  = {2, 4, 13, 8, 12};    // Trigger pins for sensors
const int echoPins[NUM_SENSORS]  = {3, 7, A1, A2, A3};     // Echo pins for sensors
const int motorPins[NUM_SENSORS] = {5, 6, 9, 10, 11};      // PWM-capable pins for motors

// Effective range for haptic feedback (in cm)
const int minEffectiveDistance = 5;    // Below this, maximum vibration (255)
const int maxEffectiveDistance = 100;  // Beyond this, no vibration (0)
const float NO_DETECTION_DISTANCE = 1000.0;  // Fallback distance if no echo is received

// Timing and smoothing parameters
const long TIMEOUT_US = 30000;         // Timeout for pulseIn (30 ms)
int currentPWM[NUM_SENSORS] = {0, 0, 0, 0, 0};  // Holds current PWM values for smoothing
const int pwmStep = 5;                           // PWM step for smoothing transitions

// Arrays to store sensor measurements and target PWM values
float distances[NUM_SENSORS];   // Measured distances for each sensor
int targetPWM[NUM_SENSORS];     // Base PWM (before applying size factor)

// Read distance (in cm) from sensor at index 'sensorIndex'
float readDistance(int sensorIndex) {
  long duration;
  float distance;
  
  // Trigger sensor: send a 10 Âµs pulse
  digitalWrite(trigPins[sensorIndex], LOW);
  delayMicroseconds(2);
  digitalWrite(trigPins[sensorIndex], HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPins[sensorIndex], LOW);
  
  // Read echo duration (with timeout)
  duration = pulseIn(echoPins[sensorIndex], HIGH, TIMEOUT_US);
  
  // If no echo is received, assume no object detected
  if (duration == 0) {
    distance = NO_DETECTION_DISTANCE;
  } else {
    // Convert echo duration to distance in cm: (duration * 0.034)/2
    distance = (duration * 0.034) / 2.0;
  }
  
  return distance;
}

// Map measured distance to a PWM value (0-255) for a single sensor
int calculateTargetPWM(float distance) {
  int pwm;
  if (distance < minEffectiveDistance) {
    pwm = 255;  // Maximum vibration for very close objects
  } else if (distance > maxEffectiveDistance) {
    pwm = 0;    // No vibration when object is too far
  } else {
    pwm = map((int)distance, minEffectiveDistance, maxEffectiveDistance, 255, 0);
    pwm = constrain(pwm, 0, 255);
  }
  return pwm;
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    pinMode(motorPins[i], OUTPUT);
  }
}

void loop() {
  // Step 1: Read each sensor and calculate the base target PWM
  int countDetected = 0;
  for (int i = 0; i < NUM_SENSORS; i++) {
    distances[i] = readDistance(i);
    targetPWM[i] = calculateTargetPWM(distances[i]);
    // Count sensors that detect an object within the effective range
    if (distances[i] < maxEffectiveDistance) {
      countDetected++;
    }
  }
  
  // Step 2: Calculate a size factor based on the number of sensors that detect an object.
  // If more than one sensor detects an object, we assume the object is larger.
  // For each additional sensor detecting the object, increase the vibration intensity by 20%.
  float sizeFactor = 1.0;
  if (countDetected > 1) {
    sizeFactor = 1.0 + (countDetected - 1) * 0.2;
  }
  
  // Step 3: Apply the size factor to each sensor's target PWM (if the sensor detects an object)
  for (int i = 0; i < NUM_SENSORS; i++) {
    if (distances[i] < maxEffectiveDistance) {
      int adjustedPWM = targetPWM[i] * sizeFactor;
      if (adjustedPWM > 255) adjustedPWM = 255;
      targetPWM[i] = adjustedPWM;
    }
  }
  
  // Step 4: Smoothly adjust the current PWM value toward the target PWM and update the motor.
  for (int i = 0; i < NUM_SENSORS; i++) {
    if (currentPWM[i] < targetPWM[i]) {
      currentPWM[i] = min(currentPWM[i] + pwmStep, targetPWM[i]);
    } else if (currentPWM[i] > targetPWM[i]) {
      currentPWM[i] = max(currentPWM[i] - pwmStep, targetPWM[i]);
    }
    analogWrite(motorPins[i], currentPWM[i]);
  }
  
  // Slow down sensor updates to let the user feel the gradual changes
  delay(15);
}