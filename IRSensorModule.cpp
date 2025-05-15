#include "IRSensorModule.h"

void initIRSensors() {
  for (int i = 0; i < NUM_SLOTS; i++) {
    pinMode(IR_PINS[i], INPUT);
  }
}

String readIRSensors(FirebaseJson &json) {
  String output = "{";
  for (int i = 0; i < NUM_SLOTS; i++) {
    int state = digitalRead(IR_PINS[i]);
    String key = "slot" + String(i + 1);
    bool occupied = (state == LOW);  // Adjust depending on IR sensor logic

    json.set(key.c_str(), occupied);
    output += "\"" + key + "\":" + (occupied ? "true" : "false");
    if (i < NUM_SLOTS - 1) output += ",";
  }
  output += "}";
  return output;
}
