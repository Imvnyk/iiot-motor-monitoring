#include <Arduino.h>
#include <DHT.h>

#define DHT_PIN 15
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();

  Serial.println("IIoT motor monitoring started");
}

void loop() {
  float temperature = dht.readTemperature();

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  delay(2000);
}