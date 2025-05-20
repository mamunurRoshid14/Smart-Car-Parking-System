#include "HardwareSerial.h"
#include "esp32-hal.h"
#include <LiquidCrystal.h>
#include <ESP32Servo.h>
#include <MFRC522.h>
#include <SPI.h>

#define MUX_SIG_PIN 35

#define MUX_S0 2
#define MUX_S1 21
#define MUX_S2 3
#define MUX_S3 1

#define NUM_SLOTS 8 
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



void initMUX() {
  pinMode(MUX_SIG_PIN, INPUT);
  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S3, OUTPUT);
}

// Function to set MUX channel (0–15)
void selectMUXChannel(uint16_t channel) {
  digitalWrite(MUX_S0, channel & 0x01);
  digitalWrite(MUX_S1, (channel >> 1) & 0x01);
  digitalWrite(MUX_S2, (channel >> 2) & 0x01);
  digitalWrite(MUX_S3, (channel >> 3) & 0x01);
}



void initHardware() {
  servo1.attach(SERVO1_PIN); servo1.write(0);
  servo2.attach(SERVO2_PIN); servo2.write(0);
  initMUX();
  SPI.begin();
  rfid1.PCD_Init(); rfid2.PCD_Init();
  lcd.begin(20, 4);
  lcd.print("Smart Parking System");
  delay(2000);
}

// Read IR sensor values from MUX
void updateSlotFromIR() {
  for (int i = 0; i < NUM_SLOTS; i++) {
    selectMUXChannel(i);
    delay(20); // allow signal to settle
    int value = analogRead(MUX_SIG_PIN);
    Serial.println(value);
    slotStatus[i] = (value < IR_THRESHOLD);
  }
  delay(50);
}

void displaySlots() {
  lcd.clear();
  lastDispUpdate = millis();
  lcd.print("------Wellcome------");
  lcd.setCursor(0, 1);
  lcd.print("S1:");
  lcd.print(slotStatus[0]);
  lcd.print(" S2:");
  lcd.print(slotStatus[1]);
  lcd.print(" S3:");
  lcd.print(slotStatus[2]);
  lcd.print(" S4:");
  lcd.print(slotStatus[3]);

  lcd.setCursor(0, 2);
  lcd.print("S5:");
  lcd.print(slotStatus[4]);
  lcd.print(" S6:");
  lcd.print(slotStatus[5]);
  lcd.print(" S7:");
  lcd.print(slotStatus[6]);
  lcd.print(" S8:");
  lcd.print(slotStatus[7]);
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
  delay(50);
  servo.write(90);
  lastDispUpdate=millis();
  if(gate==1) isgate1open=true,gate1opentime=millis();
  else isgate2open=true,gate2opentime=millis();
}

void closegate(Servo &servo, int gate) {
  if(gate==1) isgate1open=false;
  else isgate2open=false;
  servo.write(0);
  displaySlots();
  delay(50);
}

void displayNotification(String message) {
    lcd.setCursor(0, 3); 
    lcd.print("                    ");
    lcd.setCursor(0, 3);
    lcd.print(message.substring(0, 20));
    lastDispUpdate=millis();
}
