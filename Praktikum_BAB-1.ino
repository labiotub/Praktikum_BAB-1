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

    Serial.println("Sistem IoT Lokal Siap");
    Serial.println("Perintah:");
    Serial.println("SERVO <0-180>");
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
    jarak = duration * 0.034 / 2;

    // ===== BACA SUHU =====
    sensors.requestTemperatures();
    float suhu = sensors.getTempCByIndex(0);

    // ===== TAMPILKAN =====
    Serial.print("Jarak: ");
    Serial.print(jarak);
    Serial.println(" cm");

    Serial.print("Suhu: ");
    Serial.print(suhu);
    Serial.println(" °C");

    // ===== INPUT SERIAL =====
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (input.startsWith("SERVO")) {
            int sudut = input.substring(6).toInt();

            if (sudut >= 0 && sudut <= 180) {
                myServo.write(sudut);
                Serial.print("Servo ke: ");
                Serial.println(sudut);
            } else {
                Serial.println("Sudut tidak valid (0-180)");
            }
        }
    }

    delay(1000);
}
