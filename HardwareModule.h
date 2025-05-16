#include "esp32-hal.h"
#include <LiquidCrystal.h>
#include <ESP32Servo.h>
#include <MFRC522.h>
#include <SPI.h>

// IR Sensors
#define IR1 32
#define IR2 33
#define IR3 34
#define IR4 35
#define NUM_SLOTS 4
#define IR_THRESHOLD 500

// LCD
LiquidCrystal lcd(13, 12, 14, 27, 26, 25);

// RFID
#define RFID1_SDA 16
#define RFID1_RST 22
#define RFID2_SDA 17
#define RFID2_RST 5
MFRC522 rfid1(RFID1_SDA, RFID1_RST);
MFRC522 rfid2(RFID2_SDA, RFID2_RST);

// Servo
Servo servo1;
Servo servo2;
#define SERVO1_PIN 15
#define SERVO2_PIN 4

bool slotStatus[NUM_SLOTS] = {false, false, false, false};
unsigned long gate1opentime = 0;
unsigned long gate2opentime = 0;
unsigned long lastDispUpdate = 0;
bool isgate1open=false;
bool isgate2open=false;


void initHardware() {
  pinMode(IR1, INPUT); pinMode(IR2, INPUT); pinMode(IR3, INPUT); pinMode(IR4, INPUT);
  servo1.attach(SERVO1_PIN); servo1.write(0);
  servo2.attach(SERVO2_PIN); servo2.write(0);
  SPI.begin();
  rfid1.PCD_Init(); rfid2.PCD_Init();
  lcd.begin(20, 4);
  lcd.print("Smart Parking System");
  delay(2000);
}

void updateSlotFromIR() {
  int values[NUM_SLOTS] = {
    analogRead(IR1),
    analogRead(IR2),
    analogRead(IR3),
    analogRead(IR4)
  };
  for (int i = 0; i < NUM_SLOTS; i++) {
    slotStatus[i] = (values[i] < IR_THRESHOLD);
  }
}

void displaySlots() {
  lcd.clear();
  lastDispUpdate=millis();
  lcd.setCursor(0, 0); lcd.print("S1:"); lcd.print(slotStatus[0] ? "Full " : "Free ");
  lcd.print("S2:"); lcd.print(slotStatus[1] ? "Full" : "Free");
  lcd.setCursor(0, 1); lcd.print("S3:"); lcd.print(slotStatus[2] ? "Full " : "Free ");
  lcd.print("S4:"); lcd.print(slotStatus[3] ? "Full" : "Free");
}

String readRFID(MFRC522 &reader) {
  if (reader.PICC_IsNewCardPresent() && reader.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < reader.uid.size; i++) {
      uid += String(reader.uid.uidByte[i] < 0x10 ? "0" : "");
      uid += String(reader.uid.uidByte[i], HEX);
    }
    reader.PICC_HaltA();
    reader.PCD_StopCrypto1();
    uid.toUpperCase();
    return uid;
  }
  return "";
}

void openGate(Servo &servo, const String &uid, int gate) {
  servo.write(0);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(gate == 1 ? "Welcome UID: " : "Goodbye UID: ");
  lcd.setCursor(0, 1);
  lcd.print(uid.substring(0, 8));
  delay(100);
  servo.write(90);
  lastDispUpdate=millis();
  if(gate==1) isgate1open=true,gate1opentime=millis();
  else isgate2open=true,gate2opentime=millis();
  delay(50);
}

void closegate(Servo &servo, int gate) {
  if(gate==1) isgate1open=false;
  else isgate2open=false;
  servo.write(0);
  displaySlots();
  delay(50);
}