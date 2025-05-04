/*
  ESP32 Servo Control with LEDC (No External Servo Library),
  extended to receive JSON commands over Serial such as:
    {"task":"/gripper_act","action":"close"}
    {"task":"/gripper_act","action":"open"}
    {"task":"/gripper_act","action":"degree","value":90}
    {"task":"/state_get","qid":1}
  "close"  => 0°
  "open"   => 180°
  "degree" => uses the "value" field as 0..180 angle
e
  Additional return behavior:
    - For /state_get, returns JSON with firmware identifiers and config info.
    - For /gripper_act or /state_get, returns {"success":true,"qid":<the QID>} if a QID was provided.
*/

#include <Arduino.h>
#include <ArduinoJson.h>

// Adjust these as needed for your servo
static const int SERVO_PIN      = GPIO_NUM_4; // PWM-capable GPIO
static const int SERVO_CHANNEL  = 0;           // LEDC channel
static const int SERVO_FREQ     = 50;          // 50 Hz (standard servo frequency)
static const int SERVO_RES      = 16;          // 16-bit resolution

// Typical servo pulse widths (in microseconds)
/*
static const int MIN_PULSE_WIDTH = 500;   // 0 degrees
static const int MAX_PULSE_WIDTH = 2400;  // 180 degrees
static const int MIN_DEGREE = 0; 
static const int MAX_DEGREE = 120; 
static const int REFRESH_PERIOD  = 20000; // 20 ms period for 50 Hz
*/

// for ft5313m: The maximum range pulse: 900 - 2100 µs => 0..120
static const int MIN_DEGREE = 0; 
static const int MAX_DEGREE = 120; 
static const int MIN_PULSE_WIDTH = 900;   // 0 degrees
static const int MAX_PULSE_WIDTH = 2100;  // 180 degrees
static const int REFRESH_PERIOD  = 20000; // 20 ms period for 50 Hz

uint32_t angleToDutyCycle(int angle) {
  int pulseWidth = map(angle, MIN_DEGREE, MAX_DEGREE, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);
  // Convert pulse width to duty (out of (2^SERVO_RES - 1))
  uint32_t duty = (uint32_t)((((uint64_t)pulseWidth) * ((1 << SERVO_RES) - 1)) / REFRESH_PERIOD);
  
  return duty;
}

void moveServo(int angle) {
  uint32_t duty = angleToDutyCycle(angle);
  ledcWrite(SERVO_CHANNEL, duty);
}

// parseSerialJson is called with a single line of input
// Example JSON:
//   {"task":"/gripper_act","action":"close"}
//   {"task":"/gripper_act","action":"open"}
//   {"task":"/gripper_act","action":"degree","value":90}
//   {"task":"/state_get","qid":1}
void parseSerialJson(const String& input) {
  StaticJsonDocument<200> doc;
  DeserializationError err = deserializeJson(doc, input);
  if (err) {
    Serial.println("++\n{\"success\":0,\"error\":\"JSON parse error\"}\n--");
    return;
  }

  const char* task = doc["task"];
  int qid = doc["qid"] | -1;  // if "qid" isn't present, we get -1

  // Handle /state_get
  // e.g. {"task":"/state_get","qid":1}
  if (task && strcmp(task, "/state_get") == 0) {
    // Respond with identifier info
    // Example:
    // {
    //   "identifier_name": "UC2_Feather",
    //   "identifier_id": "V2.0",
    //   "identifier_date": "Mar 20 2025 12:34:56",
    //   "identifier_author": "BD",
    //   "IDENTIFIER_NAME": "uc2-esp",
    //   "configIsSet": 0,
    //   "pindef": "UC2",
    //   "success": true,
    //   "qid": <qid if provided>
    // }
    // Use quotes around strings in JSON
    Serial.print("++\n{\"identifier_name\":\"UC2_Feather\",");
    Serial.print("\"identifier_id\":\"V2.0\",");
    Serial.print("\"identifier_date\":\"");
    Serial.print(__DATE__); 
    Serial.print(" ");
    Serial.print(__TIME__);
    Serial.print("\",");
    Serial.print("\"identifier_author\":\"BD\",");
    Serial.print("\"IDENTIFIER_NAME\":\"uc2-esp\",");
    Serial.print("\"configIsSet\":0,");
    Serial.print("\"pindef\":\"UC2\",");

    Serial.print("\"success\":1");
    if (qid >= 0) {
      Serial.print(",\"qid\":");
      Serial.print(qid);
    }
    Serial.println("}\n--");
    return;
  }

  // Handle /gripper_act
  // e.g. {"task":"/gripper_act","action":"close","qid":2}
  if (!task || strcmp(task, "/gripper_act") != 0) {
    // Not our command
    // Just ignore or respond with an error if desired
    return;
  }

  const char* action = doc["action"];
  if (!action) {
    // No action
    // We can still return something
    Serial.print("{\"success\":0");
    if (qid >= 0) {
      Serial.print(",\"qid\":");
      Serial.print(qid);
    }
    Serial.println(",\"error\":\"No action specified\"}");
    return;
  }

  if (strcmp(action, "close") == 0) {
    moveServo(MIN_DEGREE);    // 0°
  }
  else if (strcmp(action, "open") == 0) {
    moveServo(MAX_DEGREE);  // 180°
  }
  else if (strcmp(action, "degree") == 0) {
    int angle = doc["value"] | -1;  // read "value" or -1 if missing
    if (angle >= MIN_DEGREE && angle <= MAX_DEGREE) {
      moveServo(angle);
    }
    else {
      // Invalid angle
      Serial.print("{\"success\":0");
      if (qid >= 0) {
        Serial.print(",\"qid\":");
        Serial.print(qid);
      }
      Serial.println(",\"error\":\"Invalid angle\"}");
      return;
    }
  }
  else {
    // Unknown action
    Serial.print("++\n{\"success\":0");
    if (qid >= 0) {
      Serial.print(",\"qid\":");
      Serial.print(qid);
    }
    Serial.println(",\"error\":\"Unknown action\"}\n--");
    return;
  }

  // If we get here, the action was handled successfully
  Serial.print("++\n{\"success\":1");
  if (qid >= 0) {
    Serial.print(",\"qid\":");
    Serial.print(qid);
  }
  Serial.println("}\n--");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  ledcSetup(SERVO_CHANNEL, SERVO_FREQ, SERVO_RES);
  ledcAttachPin(SERVO_PIN, SERVO_CHANNEL);

  // Move servo to an initial position (e.g., 90°)
  for (int angle = MIN_DEGREE; angle <= MAX_DEGREE; angle+=10) {
    moveServo(angle);
  }
  // Sweep from 180° back to 0°
  for (int angle = MAX_DEGREE; angle >= MIN_DEGREE; angle-=10) {
    moveServo(angle);
  }  

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