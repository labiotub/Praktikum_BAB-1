#include <Arduino.h>
#include <ESP32Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pin
const int oneWireBus = 5;
const int trigPin = 12;
const int echoPin = 13;
int servoPin = 18;

// Sensor suhu
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

// Servo
Servo myServo;

void setup() {
    Serial.begin(115200);

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    myServo.attach(servoPin);
    myServo.write(0);

    sensors.begin();

    Serial.println("====================================");
    Serial.println("   SISTEM IoT LOKAL - BAB 1");
    Serial.println("====================================");
    Serial.println("Perintah:");
    Serial.println("Ketik: SERVO <0-180>");
    Serial.println("Contoh: SERVO 90");
    Serial.println("====================================");
}

void loop() {

    // ===== BACA HC-SR04 =====
    long duration;
    float jarak;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);

    if (duration == 0) {
        Serial.println("[ERROR] Sensor jarak tidak terbaca!");
        jarak = 0;
    } else {
        jarak = duration * 0.034 / 2;
    }

    // ===== BACA SUHU =====
    sensors.requestTemperatures();
    float suhu = sensors.getTempCByIndex(0);

    // ===== TAMPILKAN DATA =====
    Serial.println("\n--------- DATA SENSOR ---------");

    Serial.print("Jarak : ");
    Serial.print(jarak);
    Serial.println(" cm");

    Serial.print("Suhu  : ");
    Serial.print(suhu);
    Serial.println(" °C");

    Serial.println("-------------------------------");

    // ===== INPUT SERIAL =====
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        Serial.print("\n[INPUT] ");
        Serial.println(input);

        if (input.startsWith("SERVO")) {
            int sudut = input.substring(6).toInt();

            if (sudut >= 0 && sudut <= 180) {
                myServo.write(sudut);

                Serial.print("[OK] Servo bergerak ke: ");
                Serial.print(sudut);
                Serial.println(" derajat");
            } else {
                Serial.println("[ERROR] Sudut tidak valid (0-180)");
            }
        } else {
            Serial.println("[ERROR] Format salah! Gunakan: SERVO <0-180>");
        }
    }

    delay(2000);
}
