# Multi-Sensor Haptic Headset

An Arduino-powered wearable that provides real-time haptic feedback for obstacle detection and size estimation. Five ultrasonic sensors around the head each drive a dedicated vibration motor. When two adjacent sensors detect a similar object, a brief "burst" alert communicates overlap or size.

---

## Table of Contents

1. [Features](#features)  
2. [Hardware Requirements](#hardware-requirements)  
3. [Software Requirements](#software-requirements)  
4. [Usage](#usage)  
5. [Future Enhancements](#future-enhancements)  
6. [Contact](#contact)  

---

## Features

- **5-Sensor Array**  
  Covers front, sides, and rear for 360° awareness.  
- **Three Vibration Modes**  
  1. **Normal** – continuous vibration strength ∝ proximity  
  2. **Change-Detect** – pulses on abrupt distance changes  
  3. **Radar** – pulsing rate increases as you get closer  
- **Burst Alerts**  
  Full-power pulses when obstacles span two adjacent sensors.  
- **2-Second Hold Mode Switch**  
  Hold hand within 5 cm of any sensor for 2 s to cycle modes.  
- **Serial Debugging**  
  Live logs of distances, PWM levels, mode, and burst status.

---

## Hardware Requirements

- Arduino Nano (or UNO)  
- 5× HC-SR04 ultrasonic sensors  
- 5× 5 V vibration motors (PWM-drivable)  
- Breadboard and jumper wires  
- 5 V power source (USB or battery pack)


2	Open the sketch
	- Launch Arduino IDE
	- File → Open → multiSensorVibe.ino
	3	Select board & port
	- Tools → Board → "Arduino Nano" (or UNO)
	- Tools → Port → your device port
	4	Upload
	- Click the Upload ► button

## Usage
	1	Power on the headset.
	2	Open the Serial Monitor at 9600 baud.
	3	Approach an obstacle → motors vibrate in Normal mode.
	4	Hold hand < 5 cm from any sensor for 2 s → two beeps indicate mode change.
	5	Observe new haptic pattern in Change-Detect or Radar modes.

| Mode | Name              | Behavior                                                                                  |
| :--: | ----------------- | ----------------------------------------------------------------------------------------- |
|   0  | **Normal**        | Continuous vibration strength ∝ proximity                                                 |
|   1  | **Change-Detect** | Pulses when distance changes by > 10 cm                                                   |
|   2  | **Radar**         | Pulses at a rate inversely ∝ distance; closer → faster pulses with strength mapped to PWM |


## Future Enhancements
	- Replace blocking delay() calls with a millis()-based scheduler for non-blocking updates
	- Add EEPROM storage to remember the last used mode across power cycles
	- Integrate a manual mode-select button for instant control
	- Design a compact PCB or shield for a polished wearable prototype
	- Explore IMU integration for head-movement awareness

## Contact
	- Maintainer: Parth Chougule (peerless_parth@outlook.com)
	- Club: PSU ASME Assistive-Tech Team
	- Slack Channel: #assistive-tech
For support or questions, open an issue on GitHub or message us on Slack.
---

## Software Requirements

- Arduino IDE (v1.8+ or Arduino CLI)  
