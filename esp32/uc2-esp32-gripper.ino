/*
  ESP32 Servo Control with LEDC (No External Servo Library),
  extended to receive JSON commands over Serial such as:
    {"task":"/gripper_act","action":"close"}
    {"task":"/gripper_act","action":"open"}
    {"task":"/gripper_act","action":"degree","value":90}
  "close"  => 0°
  "open"   => 180°
  "degree" => uses the "value" field as 0..180 angle
*/

#include <Arduino.h>
#include <ArduinoJson.h>

// Adjust these as needed for your servo
static const int SERVO_PIN      = GPIO_NUM_12; // PWM-capable GPIO
static const int SERVO_CHANNEL  = 0;           // LEDC channel
static const int SERVO_FREQ     = 50;          // 50 Hz (standard servo frequency)
static const int SERVO_RES      = 16;          // 16-bit resolution

// Typical servo pulse widths (in microseconds)
static const int MIN_PULSE_WIDTH = 500;   // 0 degrees
static const int MAX_PULSE_WIDTH = 2400;  // 180 degrees
static const int REFRESH_PERIOD  = 20000; // 20 ms period for 50 Hz

// Convert desired angle (0–180) to a LEDC duty value
uint32_t angleToDutyCycle(int angle) {
  if (angle < 0)   angle = 0;
  if (angle > 180) angle = 180;
  int pulseWidth = map(angle, 0, 180, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
  // Convert pulse width to duty (out of (2^SERVO_RES - 1))
  uint32_t duty = (uint32_t)((((uint64_t)pulseWidth) * ((1 << SERVO_RES) - 1)) / REFRESH_PERIOD);
  return duty;
}

void moveServo(int angle) {
  uint32_t duty = angleToDutyCycle(angle);
  ledcWrite(SERVO_CHANNEL, duty);
}

// Example JSON:
//   {"task":"/gripper_act","action":"close"}
//   {"task":"/gripper_act","action":"open"}
//   {"task":"/gripper_act","action":"degree","value":90}
void parseSerialJson(const String& input) {
  StaticJsonDocument<200> doc;
  DeserializationError err = deserializeJson(doc, input);
  if (err) {
    Serial.println("JSON parse error.");
    return;
  }
  // Check if it's the correct task
  const char* task = doc["task"];
  if (!task || strcmp(task, "/gripper_act") != 0) {
    // Not our command
    return;
  }
  // Read "action"
  const char* action = doc["action"];
  if (!action) {
    Serial.println("No action specified.");
    return;
  }

  if (strcmp(action, "close") == 0) {
    moveServo(0);      // 0°
    Serial.println("Gripper closed (0°).");
  }
  else if (strcmp(action, "open") == 0) {
    moveServo(180);    // 180°
    Serial.println("Gripper opened (180°).");
  }
  else if (strcmp(action, "degree") == 0) {
    int angle = doc["value"] | -1;  // read "value" or -1 if missing
    if (angle >= 0 && angle <= 180) {
      moveServo(angle);
      Serial.printf("Gripper set to %d°\n", angle);
    } else {
      Serial.println("Invalid angle value.");
    }
  }
  else {
    Serial.println("Unknown action.");
  }
}

void setup() {
  Serial.begin(115200);
  // Give a moment for the Serial monitor to connect
  delay(1000);

  // Set up the LEDC channel at 50 Hz, 16-bit resolution
  ledcSetup(SERVO_CHANNEL, SERVO_FREQ, SERVO_RES);
  // Attach the channel to the servo pin
  ledcAttachPin(SERVO_PIN, SERVO_CHANNEL);

  // Start at 90°
  moveServo(90);
}

void loop() {
  // If we get an entire line over Serial, parse it
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // read one line
    input.trim();
    if (input.length() > 0) {
      parseSerialJson(input);
    }
  }
}
