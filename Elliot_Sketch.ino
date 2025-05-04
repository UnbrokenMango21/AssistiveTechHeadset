#include "arduino_secrets.h"

const int NUM_SENSORS = 5;
const int trigPins[NUM_SENSORS]  = {2, 4, 13, 8, 12};    // Trigger pins for sensors
const int echoPins[NUM_SENSORS]  = {3, 7, A1, A2, A3};     // Echo pins for sensors
const int motorPins[NUM_SENSORS] = {5, 6, 9, 10, 11};      // PWM-capable pins for motors

// Effective range for haptic feedback (in cm)
const int minEffectiveDistance = 5;
const int maxEffectiveDistance = 100;
const float NO_DETECTION_DISTANCE = 1000.0;

// Burst effect parameters
const int burstDuration = 100;
const int threshold = 5; // Distance similarity threshold for triggering burst

// Timing and smoothing parameters
const long TIMEOUT_US = 30000;
int currentPWM[NUM_SENSORS] = {0, 0, 0, 0, 0};
const int pwmStep = 5;
float distances[NUM_SENSORS];
int targetPWM[NUM_SENSORS];

// Function to read distance from a sensor
float readDistance(int sensorIndex) {
    long duration;
    float distance;
    digitalWrite(trigPins[sensorIndex], LOW);
    delayMicroseconds(2);
    digitalWrite(trigPins[sensorIndex], HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPins[sensorIndex], LOW);
    duration = pulseIn(echoPins[sensorIndex], HIGH, TIMEOUT_US);
    distance = (duration == 0) ? 0 : (duration * 0.034) / 2.0;
    return (distance > maxEffectiveDistance) ? 0 : distance;
}

// Function to calculate PWM based on distance
int calculateTargetPWM(float distance) {
    if (distance < minEffectiveDistance) return 255;
    if (distance == 0 || distance > maxEffectiveDistance) return 0;
    return constrain(map(distance, minEffectiveDistance, maxEffectiveDistance, 255, 0), 0, 255);
}

// Function to trigger a burst effect
void triggerVibrationBurst(int motorPin) {
    analogWrite(motorPin, 255);
    delay(burstDuration);
    analogWrite(motorPin, 0);
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
    Serial.println("Sensor Distances:");
    // Read distances and compute target PWM values
    for (int i = 0; i < NUM_SENSORS; i++) {
        distances[i] = readDistance(i);
        targetPWM[i] = calculateTargetPWM(distances[i]);
        Serial.print("Sensor ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(distances[i]);
        Serial.println(" cm");
    }

    // Check for adjacent sensors with similar distances to trigger burst effect
    for (int i = 0; i < NUM_SENSORS - 1; i++) {
        if (abs(distances[i] - distances[i + 1]) <= threshold) {
            triggerVibrationBurst(motorPins[i]);
            triggerVibrationBurst(motorPins[i + 1]);
        }
    }

    // Smoothly adjust the motor power
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (currentPWM[i] < targetPWM[i]) {
            currentPWM[i] = min(currentPWM[i] + pwmStep, targetPWM[i]);
        } else if (currentPWM[i] > targetPWM[i]) {
            currentPWM[i] = max(currentPWM[i] - pwmStep, targetPWM[i]);
        }
        analogWrite(motorPins[i], currentPWM[i]);
    }

    delay(15); // Control update rate
}