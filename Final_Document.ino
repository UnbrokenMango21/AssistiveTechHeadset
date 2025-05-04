#include "arduino_secrets.h"

/*
  For 5 ultrasonic sensors + 5 vibration motors,
  With three modes and reliable 2âsecond hold mode switching. 
  Debug info is printed to the Serial Monitor.
*/

const int NUM_SENSORS = 5;

// Pin assignments
const int trigPins[NUM_SENSORS]  = {2, 4, 13, 8, 12};
const int echoPins[NUM_SENSORS]  = {3, 7, A2, A1, A3};
const int motorPins[NUM_SENSORS] = {5, 6, 9, 10, 11};

// Distance limits (cm)
const int minEffectiveDistance = 5;
const int maxEffectiveDistance = 100;

// Mode switching
const unsigned long MODE_HOLD_TIME = 2000; // ms holding hand triggers mode change
int mode = 0;       // 0 = Normal, 1 = Change, 2 = Radar
unsigned long holdStart = 0;

// Thresholds & timing
const int changeThreshold = 10;       // cm difference for Change mode
const int minPulseInterval = 100;     // ms (fastest radar pulse)
const int maxPulseInterval = 1000;    // ms (slowest radar pulse)
const long TIMEOUT_US = 30000;        // pulseIn timeout

// State arrays
float distances[NUM_SENSORS];
float prevDistances[NUM_SENSORS] = {0};
int currentPWM[NUM_SENSORS]    = {0};
unsigned long lastPulseTime[NUM_SENSORS] = {0};
bool        pulseState[NUM_SENSORS]     = {false};

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    pinMode(motorPins[i], OUTPUT);
  }
}

// Read one sensor's distance in cm
float readDistance(int idx) {
  digitalWrite(trigPins[idx], LOW);
  delayMicroseconds(2);
  digitalWrite(trigPins[idx], HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPins[idx], LOW);
  long d = pulseIn(echoPins[idx], HIGH, TIMEOUT_US);
  if (d == 0) return 0;
  float cm = (d * 0.034) / 2.0;
  return (cm > maxEffectiveDistance) ? 0 : cm;
}

// Map distance to PWM 0â255
int distToPWM(float cm) {
  if (cm == 0) return 0;
  if (cm < minEffectiveDistance) return 255;
  int p = map((int)cm, minEffectiveDistance, maxEffectiveDistance, 255, 0);
  return constrain(p, 0, 255);
}

// Beep twice to signal mode change
void beepBeep() {
  for (int i = 0; i < 2; i++) {
    analogWrite(motorPins[0], 255);
    delay(200);
    analogWrite(motorPins[0], 0);
    delay(200);
  }
}

void loop() {
  unsigned long now = millis();
  bool anyClose = false;

  // 1) Read all distances, compute PWMs
  for (int i = 0; i < NUM_SENSORS; i++) {
    distances[i] = readDistance(i);
    if (distances[i] > 0 && distances[i] < minEffectiveDistance) {
      anyClose = true;
    }
  }

  // 2) Handle mode switching (2-second hold)
  if (anyClose) {
    if (holdStart == 0) {
      holdStart = now;
    } else if (now - holdStart >= MODE_HOLD_TIME) {
      holdStart = 0;
      mode = (mode + 1) % 3;
      Serial.print(">>> Mode changed to ");
      Serial.println(mode);
      beepBeep();
    }
  } else {
    holdStart = 0;
  }

  // 3) Act based on current mode
  for (int i = 0; i < NUM_SENSORS; i++) {
    int pwm = distToPWM(distances[i]);

    if (mode == 0) {
      // Normal: continuous vibration
      currentPWM[i] = pwm;
      analogWrite(motorPins[i], currentPWM[i]);
    }
    else if (mode == 1) {
      // Change: pulse when distance change > threshold
      if (prevDistances[i] > 0 &&
          abs(distances[i] - prevDistances[i]) > changeThreshold) {
        analogWrite(motorPins[i], 255);
      } else {
        analogWrite(motorPins[i], 0);
      }
    }
    else {
      // Radar: pulse with interval based on distance
      unsigned long interval = map(
        constrain((int)distances[i], minEffectiveDistance, maxEffectiveDistance),
        minEffectiveDistance, maxEffectiveDistance,
        minPulseInterval, maxPulseInterval
      );
      if (now - lastPulseTime[i] >= interval) {
        lastPulseTime[i] = now;
        pulseState[i] = !pulseState[i];
        analogWrite(motorPins[i], pulseState[i] ? pwm : 0);
      }
    }

    prevDistances[i] = distances[i];
  }

  // 4) Debug output
  Serial.print("Mode "); Serial.print(mode);
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(" | S"); Serial.print(i);
    Serial.print(":"); Serial.print(distances[i],1);
    Serial.print("cm");
  }
  Serial.println();

  delay(15);
}