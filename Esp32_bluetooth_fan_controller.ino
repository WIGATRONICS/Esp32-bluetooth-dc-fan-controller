#include <Arduino.h>
#include "BluetoothSerial.h"
#include <Preferences.h>

BluetoothSerial SerialBT;
Preferences prefs;

// L298D pins
#define IN1 25
#define IN2 26
#define ENA 27   // PWM pin (to ENA)

// PWM settings
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// Current speed (0-255)
int currentSpeed = 0;

// -------- helper: set speed ----------
void setFanSpeed(int pwm) {
  pwm = constrain(pwm, 0, 255);

  // One direction
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  // PWM to ENA
  ledcWrite(PWM_CHANNEL, pwm);
  currentSpeed = pwm;

  // Bluetooth feedback
  if (pwm == 255) {
    SerialBT.println("SPEED IS HIGH");
  }
  else if (pwm == 250) {
    SerialBT.println("SPEED IS MEDIUM");
  }
  else if (pwm == 230) {
    SerialBT.println("SPEED IS LOW");
  }
  else if (pwm == 0) {
    SerialBT.println("FAN WAS STOPPED");
  }
}

// -------- helper: parse bluetooth command ----------
int parseCommand(String s) {
  s.trim();
  if (s.length() == 0) return -1;

  String u = s;
  u.toUpperCase();

  if (u == "OFF")    return 0;
  if (u == "LOW")    return 230;
  if (u == "MEDIUM") return 250;
  if (u == "HIGH")   return 255;

  // numeric?
  bool ok = true;
  for (int i = 0; i < (int)s.length(); i++) {
    if (!isDigit(s[i])) { ok = false; break; }
  }
  if (!ok) return -1;

  int val = s.toInt();

  if (val >= 0 && val <= 100) return map(val, 0, 100, 0, 255);
  if (val >= 0 && val <= 255) return val;

  return -1;
}

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  // Correct PWM setup for ESP32
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(ENA, PWM_CHANNEL);

  // Start Preferences (memory)
  prefs.begin("fan", false);

  // Restore last speed
  int savedSpeed = prefs.getInt("speed", 0);
  currentSpeed = savedSpeed;

  SerialBT.begin("A_Smart_DC_Fan_Speed_Controller");
  Serial.println("Pair Bluetooth: A_Smart_DC_Fan_Speed_Controller");

  // Apply saved speed
  setFanSpeed(savedSpeed);
}

void loop() {
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();

    int pwm = parseCommand(cmd);

    if (pwm >= 0) {
      // Save speed
      prefs.putInt("speed", pwm);

      setFanSpeed(pwm);
    }
  }
}
