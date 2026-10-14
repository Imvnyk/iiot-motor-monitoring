#include <Arduino.h>
#include <DHT.h>

#define DHT_PIN 15
#define DHT_TYPE DHT22
#define LED_PIN 2

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(LED_PIN, OUTPUT);

  Serial.println("System starting...");
}

void loop() {
  float temperature = dht.readTemperature();

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  if (temperature >= 40) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("ALERT: High temperature!");
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("Temperature normal.");
  }

  delay(2000);
}