#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <Arduino.h>
#include <AESLib.h>
#include "base64.h"

// ====== KONFIGURASI WIFI & FIREBASE ======
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASSWORD"
#define API_KEY "API_KEY"
#define DATABASE_URL "URL"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

AESLib aesLib;

// ====== KUNCI AES-128 ======
const byte key[16] = {
  '1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6'
};
byte iv[16] = {0}; 

// ====== FUNGSI UTAMA ENKRIPSI/DEKRIPSI ======
void resetBuffer(byte* buffer, int length) {
  memset(buffer, 0, length);
}

void resetIV() {
  memset(iv, 0, sizeof(iv));
}

String encryptAES(String plainText) {
  int plainLen = plainText.length();
  int paddedLen = ((plainLen + 15) / 16) * 16;

  byte plain[paddedLen];
  byte encrypted[paddedLen];
  char base64_output[paddedLen * 2];

  resetBuffer(plain, paddedLen);
  resetBuffer(encrypted, paddedLen);
  resetIV();

  memcpy(plain, plainText.c_str(), plainLen);
  aesLib.encrypt(plain, paddedLen, encrypted, key, 128, iv);

  base64_encode(base64_output, (char*)encrypted, paddedLen);
  return String(base64_output);
}

String decryptAES(String encryptedText) {
  int encryptedLen = encryptedText.length();

  byte decodedText[encryptedLen];
  byte decrypted[encryptedLen];

  resetBuffer(decodedText, encryptedLen);
  resetBuffer(decrypted, encryptedLen);
  resetIV();

  int decodedLen = base64_decode((char*)decodedText, encryptedText.c_str(), encryptedLen);
  aesLib.decrypt(decodedText, decodedLen, decrypted, key, 128, iv);

  return String((char*)decrypted);
}

// ====== TIMER ======
unsigned long prevSendMillis = 0;
unsigned long prevDecryptMillis = 0;
const unsigned long sendInterval = 5000;
const unsigned long decryptInterval = 15000;

int count = 0;
bool signupOK = false;

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  Serial.println("\nMasukkan teks yang ingin dienkripsi:");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected with IP: " + WiFi.localIP().toString());

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase SignUp OK");
    signupOK = true;
  } else {
    Serial.printf("Firebase SignUp Failed: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

// ====== LOOP ======
void loop() {
  unsigned long now = millis();

  // === Kirim data ke Firebase setiap 5 detik ===
  if (Firebase.ready() && (now - prevSendMillis >= sendInterval)) {
    prevSendMillis = now;

    Serial.printf("Set bool... %s\n", Firebase.RTDB.setBool(&fbdo, F("/test/bool"), count % 2 == 0) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Get bool... %s\n", Firebase.RTDB.getBool(&fbdo, F("/test/bool")) ? fbdo.to<bool>() ? "true" : "false" : fbdo.errorReason().c_str());

    int iVal = 0;
    Serial.printf("Set int... %s\n", Firebase.RTDB.setInt(&fbdo, F("/test/int"), count) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Get int... %s\n", Firebase.RTDB.getInt(&fbdo, F("/test/int"), &iVal) ? String(iVal).c_str() : fbdo.errorReason().c_str());

    Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/test/float"), count + 0.75) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Set string... %s\n", Firebase.RTDB.setString(&fbdo, F("/test/string"), F("Hello World!")) ? "ok" : fbdo.errorReason().c_str());

    FirebaseJson json;
    if (count == 0) {
      json.set("value/round/" + String(count), F("cool!"));
      json.set(F("value/ts/.sv"), F("timestamp"));
      Firebase.RTDB.set(&fbdo, F("/test/json"), &json);
    } else {
      json.add(String(count), F("smart!"));
      Firebase.RTDB.updateNode(&fbdo, F("/test/json/value/round"), &json);
    }

    Serial.println();
    count++;
  }

  // === Ambil dan dekripsi data terenkripsi setiap 15 detik ===
  if (Firebase.ready() && (now - prevDecryptMillis >= decryptInterval)) {
    prevDecryptMillis = now;

    if (Firebase.RTDB.getString(&fbdo, "/test/decrypted")) {
      String encryptedText = fbdo.to<String>();
      Serial.println("\n📥 Data terenkripsi dari Firebase: " + encryptedText);

      String decryptedText = decryptAES(encryptedText);
      Serial.println("🔓 Hasil Dekripsi: " + decryptedText);
    } else {
      Serial.println("❌ Gagal membaca /test/decrypted dari Firebase: " + fbdo.errorReason());
    }
  }

  // === Cek input dari Serial ===
  if (Serial.available()) {
    String text = Serial.readStringUntil('\n');
    text.trim();

    if (text.length() > 0) {
      Serial.println("\n📤 Plaintext: " + text);
      String encryptedText = encryptAES(text);
      Serial.println("🔐 Encrypted (Base64): " + encryptedText);

      if (Firebase.RTDB.setString(&fbdo, "/test/encrypted_string", encryptedText)) {
        Serial.println("✅ Data terenkripsi berhasil dikirim ke Firebase");
      } else {
        Serial.println("❌ Gagal mengirim data ke Firebase: " + fbdo.errorReason());
      }

      Serial.println("\nMasukkan teks lagi untuk dienkripsi:");
    }
  }
}
