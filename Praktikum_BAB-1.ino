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
#include "base64.h"  // Pastikan pustaka Base64 yang benar

// ====== KONFIGURASI WIFI & FIREBASE ======
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASSWORD"
#define API_KEY "API KEY"
#define DATABASE_URL "FIREBASE URL"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long timer = 0;
int count = 0;
bool signupOK = false;

AESLib aesLib;

// ====== KUNCI AES-128 ======
const byte key[16] = { 
    '1', '2', '3', '4', '5', '6', '7', '8', 
    '9', '0', '1', '2', '3', '4', '5', '6' 
}; // 16-byte key untuk AES-128

byte iv[16] = {0}; // IV tidak digunakan untuk mode ECB, tetapi tetap harus direset

// ====== FUNGSI RESET BUFFER & IV ======
void resetBuffer(byte* buffer, int length) {
    memset(buffer, 0, length);
}

void resetIV() {
    memset(iv, 0, sizeof(iv)); // Reset IV ke 0 untuk setiap iterasi
}

// ====== FUNGSI ENKRIPSI AES-128 ECB ======
String encryptAES(String plainText) {
    int plainLen = plainText.length();
    int paddedLen = ((plainLen + 15) / 16) * 16; // Pastikan ukuran kelipatan 16 byte

    byte plain[paddedLen];
    byte encrypted[paddedLen];
    char base64_output[paddedLen * 2]; // Buffer untuk hasil Base64

    resetBuffer(plain, paddedLen);
    resetBuffer(encrypted, paddedLen);
    resetIV();  // Pastikan IV selalu di-reset

    memcpy(plain, plainText.c_str(), plainLen);
    aesLib.encrypt(plain, paddedLen, encrypted, key, 128, iv);

    base64_encode(base64_output, (char*)encrypted, paddedLen);
    return String(base64_output);
}

// ====== FUNGSI DEKRIPSI AES-128 ECB ======
String decryptAES(String encryptedText) {
    int encryptedLen = encryptedText.length();

    byte decodedText[encryptedLen];
    byte decrypted[encryptedLen];

    resetBuffer(decodedText, encryptedLen);
    resetBuffer(decrypted, encryptedLen);
    resetIV();  // Pastikan IV selalu di-reset

    int decodedLen = base64_decode((char*)decodedText, encryptedText.c_str(), encryptedLen);
    aesLib.decrypt(decodedText, decodedLen, decrypted, key, 128, iv);

    return String((char*)decrypted);
}

// ====== SETUP ESP & FIREBASE ======
void setup() {
    Serial.begin(115200);
    Serial.println("\nMasukkan teks yang ingin dienkripsi:");

    // Koneksi WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println("\nConnected with IP: " + WiFi.localIP().toString());

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    // Konfigurasi Firebase
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

// ====== LOOP UTAMA ======
void loop() {
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();

    Serial.printf("Set bool... %s\n", Firebase.RTDB.setBool(&fbdo, F("/test/bool"), count % 2 == 0) ? "ok" : fbdo.errorReason().c_str());

    Serial.printf("Get bool... %s\n", Firebase.RTDB.getBool(&fbdo, FPSTR("/test/bool")) ? fbdo.to<bool>() ? "true" : "false" : fbdo.errorReason().c_str());

    bool bVal;
    Serial.printf("Get bool ref... %s\n", Firebase.RTDB.getBool(&fbdo, F("/test/bool"), &bVal) ? bVal ? "true" : "false" : fbdo.errorReason().c_str());

    Serial.printf("Set int... %s\n", Firebase.RTDB.setInt(&fbdo, F("/test/int"), count) ? "ok" : fbdo.errorReason().c_str());

    Serial.printf("Get int... %s\n", Firebase.RTDB.getInt(&fbdo, F("/test/int")) ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());

    int iVal = 0;
    Serial.printf("Get int ref... %s\n", Firebase.RTDB.getInt(&fbdo, F("/test/int"), &iVal) ? String(iVal).c_str() : fbdo.errorReason().c_str());

    Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/test/float"), count + 10.2) ? "ok" : fbdo.errorReason().c_str());

    Serial.printf("Get float... %s\n", Firebase.RTDB.getFloat(&fbdo, F("/test/float")) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());

    Serial.printf("Set double... %s\n", Firebase.RTDB.setDouble(&fbdo, F("/test/double"), count + 35.517549723765) ? "ok" : fbdo.errorReason().c_str());

    Serial.printf("Get double... %s\n", Firebase.RTDB.getDouble(&fbdo, F("/test/double")) ? String(fbdo.to<double>()).c_str() : fbdo.errorReason().c_str());

    Serial.printf("Set string... %s\n", Firebase.RTDB.setString(&fbdo, F("/test/string"), F("Hello World!")) ? "ok" : fbdo.errorReason().c_str());

    Serial.printf("Get string... %s\n", Firebase.RTDB.getString(&fbdo, F("/test/string")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str());

    // For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create_Edit_Parse.ino
    FirebaseJson json;

    if (count == 0)
    {
      json.set("value/round/" + String(count), F("cool!"));
      json.set(F("value/ts/.sv"), F("timestamp"));
      Serial.printf("Set json... %s\n", Firebase.RTDB.set(&fbdo, F("/test/json"), &json) ? "ok" : fbdo.errorReason().c_str());
    }
    else
    {
      json.add(String(count), F("smart!"));
      Serial.printf("Update node... %s\n", Firebase.RTDB.updateNode(&fbdo, F("/test/json/value/round"), &json) ? "ok" : fbdo.errorReason().c_str());
    }

    Serial.println();
    count++;
    delay (2000);
  }
    if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();

        // Membaca teks terenkripsi dari Firebase (/test/decrypted)
        if (Firebase.RTDB.getString(&fbdo, "/test/decrypted")) {
            String encryptedText = fbdo.to<String>();
            Serial.println("\n📥 Data terenkripsi dari Firebase: " + encryptedText);

            // Dekripsi
            String decryptedText = decryptAES(encryptedText);
            Serial.println("🔓 Hasil Dekripsi: " + decryptedText);
        } else {
            Serial.println("❌ Gagal membaca /test/decrypted dari Firebase: " + fbdo.errorReason());
        }
    }

    // Membaca input dari Serial Monitor untuk dienkripsi & dikirim ke Firebase
    if (Serial.available()) {
        String text = Serial.readStringUntil('\n');
        text.trim();

        if (text.length() > 0) {
            Serial.println("\n📤 Plaintext: " + text);
            String encryptedText = encryptAES(text);
            Serial.println("🔐 Encrypted (Base64): " + encryptedText);

            // Mengirim teks terenkripsi ke Firebase (/test/encrypted_string)
            if (Firebase.RTDB.setString(&fbdo, "/test/encrypted_string", encryptedText)) {
                Serial.println("✅ Data terenkripsi berhasil dikirim ke Firebase");
            } else {
                Serial.println("❌ Gagal mengirim data ke Firebase: " + fbdo.errorReason());
            }

            Serial.println("\nMasukkan teks lagi untuk dienkripsi:");
            delay(2000);
        }
    }
}
