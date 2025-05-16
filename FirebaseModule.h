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

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void syncTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

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
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
}

void initFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = "mamunpgx@gmail.com";  // Use your Firebase Auth email
  auth.user.password = "12345678";       // Use your Firebase Auth password

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
