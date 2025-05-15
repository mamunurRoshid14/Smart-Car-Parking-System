#ifndef IRSENSORMODULE_H
#define IRSENSORMODULE_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>

// === Constants ===
#define NUM_SLOTS 4 // Adjust as needed
const int IR_PINS[NUM_SLOTS] = {34, 35, 32, 33}; // GPIOs connected to IR sensors

// === Function Prototypes ===
void initIRSensors();
String readIRSensors(FirebaseJson &json);

#endif
