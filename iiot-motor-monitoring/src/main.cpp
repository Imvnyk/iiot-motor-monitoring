#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define DHT_PIN 15
#define DHT_TYPE DHT22
#define LED_PIN 2

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);

  dht.begin();
  pinMode(LED_PIN, OUTPUT);

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1) {
      delay(10);
    }
  }

  Serial.println("IIoT motor monitoring started");
}

void loop() {
  // Read temperature
  float temperature = dht.readTemperature();

  // Read MPU6050 acceleration
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Calculate acceleration magnitude
  float accelerationMagnitude = sqrt(
    a.acceleration.x * a.acceleration.x +
    a.acceleration.y * a.acceleration.y +
    a.acceleration.z * a.acceleration.z
  );

  // Simple vibration indicator
  float vibration = abs(accelerationMagnitude - 9.81);

  // Display values
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Accel X: ");
  Serial.println(a.acceleration.x);

  Serial.print("Accel Y: ");
  Serial.println(a.acceleration.y);

  Serial.print("Accel Z: ");
  Serial.println(a.acceleration.z);

  Serial.print("Vibration: ");
  Serial.print(vibration);
  Serial.println(" m/s2");

  // Alert conditions
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