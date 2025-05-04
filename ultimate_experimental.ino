#include "arduino_secrets.h"

const int NUM_SENSORS = 5;
const int trigPins[NUM_SENSORS]  = {2, 4, 13, 8, 12};
const int echoPins[NUM_SENSORS]  = {3, 7, A1, A2, A3};
const int motorPins[NUM_SENSORS] = {5, 6, 9, 10, 11};

const int minEffectiveDistance = 5;
const int maxEffectiveDistance = 100;
const float NO_DETECTION_DISTANCE = 1000.0;

const int threshold = 1;
const int maxBurstTriggerDistance = 200;
const long TIMEOUT_US = 30000;
const int pwmStep = 5;

const int minPulseInterval = 100;
const int maxPulseInterval = 400;

float distances[NUM_SENSORS];
int targetPWM[NUM_SENSORS];
int currentPWM[NUM_SENSORS] = {0, 0, 0, 0, 0};

bool burstActive[NUM_SENSORS] = {false, false, false, false, false};
int burstPulseCount[NUM_SENSORS] = {0, 0, 0, 0, 0};
unsigned long lastBurstTime[NUM_SENSORS] = {0, 0, 0, 0, 0};
bool burstPulseState[NUM_SENSORS] = {false, false, false, false, false};
int burstInterval[NUM_SENSORS] = {200, 200, 200, 200, 200};

int burstTriggerCounter[NUM_SENSORS - 1] = {0};  // One less because we check sensor pairs

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
  return distance;
}

int calculateTargetPWM(float distance) {
  if (distance < minEffectiveDistance || distance > maxEffectiveDistance) return 0;
  return constrain(map(distance, minEffectiveDistance, maxEffectiveDistance, 255, 0), 0, 255);
}

void startBurst(int sensorIndex, float avgDistance) {
  burstActive[sensorIndex] = true;
  burstPulseCount[sensorIndex] = 0;
  burstPulseState[sensorIndex] = true;
  lastBurstTime[sensorIndex] = millis();
  burstInterval[sensorIndex] = map(avgDistance, minEffectiveDistance, maxEffectiveDistance, minPulseInterval, maxPulseInterval);
  analogWrite(motorPins[sensorIndex], 255);

  Serial.print("ð Burst triggered on motor ");
  Serial.print(sensorIndex);
  Serial.print(" (avg distance: ");
  Serial.print(avgDistance);
  Serial.println(" cm)");
}

void handleBurst(int sensorIndex) {
  if (!burstActive[sensorIndex]) return;

  unsigned long now = millis();
  if (now - lastBurstTime[sensorIndex] >= burstInterval[sensorIndex]) {
    lastBurstTime[sensorIndex] = now;

    if (burstPulseState[sensorIndex]) {
      analogWrite(motorPins[sensorIndex], 0);
      burstPulseState[sensorIndex] = false;
    } else {
      burstPulseCount[sensorIndex]++;
      if (burstPulseCount[sensorIndex] >= 3) {
        burstActive[sensorIndex] = false;
        analogWrite(motorPins[sensorIndex], currentPWM[sensorIndex]);
      } else {
        analogWrite(motorPins[sensorIndex], 255);
        burstPulseState[sensorIndex] = true;
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    pinMode(motorPins[i], OUTPUT);
  }
}

void loop() {
  unsigned long now = millis();

  for (int i = 0; i < NUM_SENSORS; i++) {
    distances[i] = readDistance(i);
    targetPWM[i] = calculateTargetPWM(distances[i]);

    Serial.print("Sensor ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(distances[i]);
    Serial.println(" cm");
  }

  // Check adjacent pairs
  for (int i = 0; i < NUM_SENSORS - 1; i++) {
    bool valid = distances[i] != 0 && distances[i + 1] != 0 &&
                 distances[i] < maxBurstTriggerDistance &&
                 distances[i + 1] < maxBurstTriggerDistance &&
                 abs(distances[i] - distances[i + 1]) <= threshold;

    if (valid) {
      burstTriggerCounter[i]++;
      if (burstTriggerCounter[i] >= 3) {
        float avgDist = (distances[i] + distances[i + 1]) / 2.0;
        if (!burstActive[i]) startBurst(i, avgDist);
        if (!burstActive[i + 1]) startBurst(i + 1, avgDist);
        burstTriggerCounter[i] = 0; // Reset after triggering
      }
    } else {
      burstTriggerCounter[i] = 0; // Reset if not continuous
    }
  }

  for (int i = 0; i < NUM_SENSORS; i++) {
    handleBurst(i);
  }

  for (int i = 0; i < NUM_SENSORS; i++) {
    if (!burstActive[i]) {
      if (currentPWM[i] < targetPWM[i]) {
        currentPWM[i] = min(currentPWM[i] + pwmStep, targetPWM[i]);
      } else if (currentPWM[i] > targetPWM[i]) {
        currentPWM[i] = max(currentPWM[i] - pwmStep, targetPWM[i]);
      }
      analogWrite(motorPins[i], currentPWM[i]);
    }
  }

  delay(20);
}