#include "esp32-hal.h"
#include <Firebase_ESP_Client.h>
#include <WiFi.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <time.h>

// Wi-Fi credentials
//#define WIFI_SSID "R8202GHZ"
//#define WIFI_PASSWORD "%A4f%t^ejAGiMP"

#define WIFI_SSID "Ummay"
#define WIFI_PASSWORD "123456788"

// Firebase credentials
#define API_KEY "AIzaSyA34JedpoXjgOjT6PJgN06yQBIL8b4Gb24"
#define DATABASE_URL "smart-car-parking-f6231-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_PROJECT_ID "smart-car-parking-f6231"

const int COST_PER_MIN = 2;  // You can adjust this
const int BASE_CHARGE = 50;


// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long lastWifiCheck = 0;

void syncTime() {
  configTime(6 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Syncing time");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nTime synced!");
}

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  displayNotification("Connecting to WiFi....");
  while (WiFi.status() != WL_CONNECTED)
    ;
  Serial.print("Connecting to WiFi");
  Serial.println("\nWiFi Connected!");
  displayNotification("WiFi Connected!");
}

void checkWiFi() {
  lastWifiCheck = millis();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected! Reconnecting...");
    initWiFi();
  } else {
    Serial.println("WiFi is connected.");
  }
}


void initFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = "mamunpgx@gmail.com";
  auth.user.password = "12345678";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void updateSlotStatus(bool slotStatus[], int size) {
  FirebaseJson json;
  for (int i = 0; i < size; i++) {
    json.set("slot" + String(i + 1), slotStatus[i] ? 1 : 0);
  }

  if (Firebase.RTDB.setJSON(&fbdo, "/slots", &json)) {
    Serial.println("Firebase Update Success");
  } else {
    Serial.println("Firebase Update Failed");
    Serial.println(fbdo.errorReason());
  }
}

// Get current time string
String getCurrentTime() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStr[30];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(timeStr);
  } else {
    return "N/A";
  }
}

// Check UID in Firestore and update entry time
bool isUIDAuthorized(String uid) {
  String docPath = "authorizedRFIDs/" + uid;

  if(available_Slot==0){
    displayNotification("Sorry No More Slot.");
    return false;
  }
  if (!Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", docPath.c_str())) {
    Serial.println("Authorization failed.");
    displayNotification("Not reg: " + uid);
    return false;
  }

  FirebaseJson doc;
  doc.setJsonData(fbdo.payload());

  FirebaseJsonData nameData, mobileData, balanceData, entryData, statusData, minBalanceData, licenceData;

  if (!doc.get(nameData, "fields/fullName/stringValue") || !doc.get(mobileData, "fields/mobile_No/stringValue") || !doc.get(balanceData, "fields/balance/integerValue") || !doc.get(entryData, "fields/entry_time/stringValue") || !doc.get(statusData, "fields/status/integerValue") || !doc.get(minBalanceData, "fields/min_balance/integerValue") || !doc.get(licenceData, "fields/licence_No/stringValue")) {
    Serial.println("Authorization failed.");
    displayNotification("Unknown Error");
    //return false;
  }
  // Store field values
  String fullName = nameData.to<String>();
  String mobileNo = mobileData.to<String>();
  int balance = balanceData.to<int>();
  String entryTime = entryData.to<String>();
  int minBalance = minBalanceData.to<int>();
  int status = statusData.to<int>();
  String licenceNo = licenceData.to<String>();

  // Check balance and status
  if (balance < minBalance || status != 0) {
    Serial.println("Authorization failed.");
    if (status) displayNotification("Car already in parking");
    else displayNotification("Balance shortage");
    return false;
  }

  // Update entry_time and status
  String newEntryTime = getCurrentTime();
  FirebaseJson update;
  update.set("fields/entry_time/stringValue", newEntryTime);
  update.set("fields/status/integerValue", 1);

  if (!Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", docPath.c_str(), update.raw(), "entry_time,status")) {
    Serial.println("Authorization failed.");
    displayNotification("Firebase Error");
    return false;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("----Welcome----");
  lcd.setCursor(0, 1);
  lcd.print(fullName.substring(0, 20));
  lcd.setCursor(0, 2);
  lcd.print("Licence No: ");
  lcd.setCursor(0, 3);
  lcd.print(licenceNo);
  Serial.println("Access granted.");
  lastDispUpdate = millis();
  return true;
}

bool handleExit(String uid) {
  String docPath = "authorizedRFIDs/" + uid;

  // Step 1: Fetch user document
  if (!Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", docPath.c_str())) {
    Serial.println("Exit failed.");
    displayNotification("User not found");
    return false;
  }

  FirebaseJson doc;
  doc.setJsonData(fbdo.payload());

  FirebaseJsonData nameData, entryData, balanceData, statusData;
  if (!doc.get(nameData, "fields/fullName/stringValue") || !doc.get(entryData, "fields/entry_time/stringValue") || !doc.get(balanceData, "fields/balance/integerValue") || !doc.get(statusData, "fields/status/integerValue")) {
    Serial.println("Exit failed.");
    displayNotification("Data error");
    //return false;
  }

  // Step 2: Extract values
  String fullName = nameData.to<String>();
  String entryTimeStr = entryData.to<String>();
  int balance = balanceData.to<int>();
  int status = statusData.to<int>();

  if (status == 0) {
    Serial.println("User not in parking.");
    displayNotification("Already exited");
    return false;
  }

  // Step 3: Time calculation
  struct tm entryTm = {};
  strptime(entryTimeStr.c_str(), "%Y-%m-%d %H:%M:%S", &entryTm);
  time_t entryTime = mktime(&entryTm);

  String currentTimeStr = getCurrentTime();
  struct tm nowTm = {};
  strptime(currentTimeStr.c_str(), "%Y-%m-%d %H:%M:%S", &nowTm);
  time_t now = mktime(&nowTm);

  int durationMin = max(1, int((now - entryTime) / 60));  // at least 1 minute
  int cost = BASE_CHARGE + durationMin * COST_PER_MIN;

  int updatedBalance = balance - cost;
//  if (updatedBalance < 0) updatedBalance = 0;

  // Step 4: Update balance and status
  FirebaseJson update;
  update.set("fields/balance/integerValue", updatedBalance);
  update.set("fields/status/integerValue", 0);

  if (!Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", docPath.c_str(), update.raw(), "balance,status")) {
    Serial.println("Exit update failed.");
    displayNotification("Firebase Error");
    return false;
  }

  // Step 5: Display LCD message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("----Goodbye----");
  lcd.setCursor(0, 1);
  lcd.print(fullName.substring(0, 20));
  lcd.setCursor(0, 2);
  lcd.print("Parking fee: ");
  lcd.print(cost);
  lcd.setCursor(0, 3);
  lcd.print("Bal: ");
  lcd.print(updatedBalance);

  Serial.println("Exit successful. Cost: " + String(cost));
  lastDispUpdate = millis();

// Step 6: Store transaction
FirebaseJson transaction;
transaction.set("fields/uid/stringValue", uid);
transaction.set("fields/entry_time/stringValue", entryTimeStr);
transaction.set("fields/exit_time/stringValue", currentTimeStr);
transaction.set("fields/cost/integerValue", cost);

Serial.println("Transaction JSON: ");
Serial.println(transaction.raw());

if (!Firebase.Firestore.createDocument(&fbdo,
                                      FIREBASE_PROJECT_ID,
                                      "(default)",
                                      "transactions",
                                      "", // Auto-generated document ID
                                      transaction.raw(),
                                      "")) { // Empty mask
  Serial.println("Failed to store transaction:");
  Serial.println(fbdo.errorReason());
} else {
  Serial.println("Transaction saved!");
}

  return true;
}