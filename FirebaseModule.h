#include "esp32-hal.h"
#include <Firebase_ESP_Client.h>
#include <WiFi.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <time.h>

// Wi-Fi credentials
#define WIFI_SSID "R8202GHZ"
#define WIFI_PASSWORD "%A4f%t^ejAGiMP"

// Firebase credentials
#define API_KEY "AIzaSyA34JedpoXjgOjT6PJgN06yQBIL8b4Gb24"
#define DATABASE_URL "smart-car-parking-f6231-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_PROJECT_ID "smart-car-parking-f6231"

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
  while(WiFi.status() != WL_CONNECTED);
  Serial.print("Connecting to WiFi");
  Serial.println("\nWiFi Connected!");
  displayNotification("WiFi Connected!");
}

void checkWiFi() {
    lastWifiCheck=millis();
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
  String documentPath = "authorizedRFIDs/" + uid;

  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())) {
    Serial.println("UID is authorized.");
    Serial.println(fbdo.payload());

    FirebaseJson doc;
    doc.setJsonData(fbdo.payload());

    FirebaseJsonData balanceData, minBalanceData;

    if (doc.get(balanceData, "fields/balance/integerValue") &&
        doc.get(minBalanceData, "fields/min_balance_required/integerValue")) {

      int balance = balanceData.to<int>();
      int minBalance = minBalanceData.to<int>();

      if (balance >= minBalance) {
        // Update entry time in Firestore
        String entryTime = getCurrentTime();
        FirebaseJson update;
        update.set("fields/entry_time/stringValue", entryTime);

        if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), update.raw(), "entry_time")) {
          Serial.println("Entry time updated: " + entryTime);
          return true;
        } else {
          Serial.println("Failed to update entry time: " + fbdo.errorReason());
          return false;
        }
      } else {
        Serial.println("Insufficient balance.");
        return false;
      }

    } else {
      Serial.println("Required fields missing in document.");
      return false;
    }

  } else {
    Serial.println("UID is NOT authorized.");
    Serial.println(fbdo.errorReason());
    return false;
  }
}

