#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define DHT_PIN 15
#define DHT_TYPE DHT22
#define LED_PIN 2

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_MPU6050 mpu;

const char* ssid = "Wokwi-GUEST";
const char* password = "";

void connectWiFi() {
  Serial.print("Connecting to WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);

  dht.begin();
  pinMode(LED_PIN, OUTPUT);

  if (!mpu.begin()) {
    Serial.println("ERROR: MPU6050 not found!");

    while (1) {
      delay(10);
    }
  }

  connectWiFi();

  Serial.println("IIoT motor monitoring started");
}

void loop() {
  // 1. Read temperature
  float temperature = dht.readTemperature();

  // 2. Read MPU6050
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float accelX = a.acceleration.x;
  float accelY = a.acceleration.y;
  float accelZ = a.acceleration.z;

  // 3. Calculate simple vibration value
  float accelerationMagnitude = sqrt(
    accelX * accelX +
    accelY * accelY +
    accelZ * accelZ
  );

  float vibration = abs(accelerationMagnitude - 9.81);

  // 4. Display data
  Serial.println();
  Serial.println("----- MOTOR DATA -----");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Acceleration X: ");
  Serial.println(accelX);

  Serial.print("Acceleration Y: ");
  Serial.println(accelY);

  Serial.print("Acceleration Z: ");
  Serial.println(accelZ);

  Serial.print("Vibration: ");
  Serial.print(vibration);
  Serial.println(" m/s2");

  // 5. Alert conditions
  bool highTemperature = temperature >= 40;
  bool highVibration = vibration >= 4;

  if (highTemperature || highVibration) {
    digitalWrite(LED_PIN, HIGH);

    Serial.println("STATUS: ALERT");

    if (highTemperature) {
      Serial.println("Reason: High temperature");
    }

    if (highVibration) {
      Serial.println("Reason: High vibration");
    }
  } else {
    digitalWrite(LED_PIN, LOW);

    Serial.println("STATUS: NORMAL");
  }

  Serial.println("----------------------");

  delay(2000);
}