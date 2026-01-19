# Smart Bicycle Alarm with Edge AI

This project is a smart bicycle alarm developed for an IoT and Edge AI module.
The system detects suspicious vibrations and activates an alarm when a real threat is detected.

## Hardware Used
- Arduino UNO R4 WiFi
- SW-420 Vibration Sensor
- Active Buzzer
- 18650 Lithium Battery
- TP4056 Charging Module
- Switch
- Jumper Wires
- Plastic Enclosure

## How the System Works
The vibration sensor detects movement and sends both digital and analog signals to the Arduino.
A simplified Edge AI logic analyses the intensity and history of vibrations to reduce false alarms.

If a real event is detected:
- The buzzer is activated
- Data is sent to ThingSpeak (if Wi-Fi is available)

The system works fully offline when there is no internet connection.

## Cloud Platform
ThingSpeak is used to visualize:
- Alarm activations
- Vibration intensity
- Edge AI confirmation
- System statistics



## Files
- `src/`: Arduino source code
- `docs/`: Circuit and system diagrams
- `images/`: Final prototype images

## Author
Pablo Rodriguez
