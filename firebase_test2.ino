#include "FirebaseModule.h"
#include "HardwareModule.h"

unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(115200);
  initHardware();
  delay(200);
  initWiFi();
  syncTime();
  initFirebase();
  delay(1000);
}

void loop() {
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  updateSlotFromIR();
  if(millis()-lastDispUpdate>3000) displaySlots();
  if(isgate1open && millis()-gate1opentime>10000)closegate(servo1,1);
  if(isgate2open && millis()-gate2opentime>10000)closegate(servo2,2);

  // RFID 1 = Entry
  String uid1 = readRFID(rfid1);
  if (uid1 != "") {
    openGate(servo1, uid1, 1);
  }

  // RFID 2 = Exit
  String uid2 = readRFID(rfid2);
  if (uid2 != "") {
    openGate(servo2, uid2, 2);
  }

  if (millis() - lastUpdate > 3000) {
    updateSlotStatus(slotStatus, NUM_SLOTS);
    lastUpdate = millis();
  }

  delay(500);
}
