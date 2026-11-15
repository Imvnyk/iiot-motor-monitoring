#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>
#include <time.h>

// ===== WIFI =====
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// ===== EMQX CLOUD =====
const char* MQTT_SERVER = "youremqxhost";
const int MQTT_PORT = 8883;

const char* MQTT_USERNAME = "your username";
const char* MQTT_PASSWORD = "your password"; 
const char* MQTT_CLIENT_ID = "clientid";

// ===== CA CERTIFICATE =====
static const char MQTT_CA[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----

)EOF";


const bool TEST_INVALID_TEMPERATURE = false;

// ===== MQTT TOPICS =====
const char* TOPIC_STATUS = "factory/motor/status";
const char* TOPIC_TEMP = "factory/motor/temperature";
const char* TOPIC_VIB = "factory/motor/vibration";
const char* TOPIC_COMMAND = "factory/motor/command";

// ===== PINS =====
#define DHT_PIN 15
#define LED_PIN 2
#define SDA_PIN 21
#define SCL_PIN 22

// ===== THRESHOLDS =====
const float TEMP_LIMIT = 40.0;
const float VIB_LIMIT = 4.0;

// ===== SENSORS =====
DHT dht(DHT_PIN, DHT22);
Adafruit_MPU6050 mpu;
bool mpuReady = false;

// ===== MQTT =====
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 5000;

// ===== LED CONTROL =====
bool manualLedOn = false;
bool automaticAlert = false;

void updateLED() {
  bool ledOn = manualLedOn || automaticAlert;

  digitalWrite(LED_PIN, ledOn ? HIGH : LOW);

  Serial.print("LED: ");
  Serial.println(ledOn ? "ON" : "OFF");
}

// ===== RECEIVE NODE-RED COMMANDS =====
void mqttCallback(char* topic, byte* payload, unsigned int length) {

  if (String(topic) != TOPIC_COMMAND) {
    return;
  }

  String command = "";

  for (unsigned int i = 0; i < length; i++) {
    command += (char)payload[i];
  }

  command.trim();
  command.toUpperCase();

  Serial.print("Command received from Node-RED: ");
  Serial.println(command);

  if (command == "ON") {
    manualLedOn = true;
  }
  else if (command == "OFF") {
    manualLedOn = false;
  }
  else {
    Serial.println("Unknown command!");
    return;
  }

  updateLED();
}

// ===== WIFI =====
void connectWiFi() {
  Serial.print("Connecting to WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
}

// ===== CLOCK SYNCHRONIZATION =====
bool synchronizeClock() {
  Serial.print("Synchronizing time");

  configTime(0, 0, "pool.ntp.org", "time.google.com");

  unsigned long start = millis();

  while (time(nullptr) < 1700000000L &&
         millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (time(nullptr) < 1700000000L) {
    Serial.println(" FAILED!");
    return false;
  }

  Serial.println(" OK!");
  return true;
}

// ===== MQTT =====
void connectMQTT() {
  while (!mqttClient.connected()) {

    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    }

    Serial.print("Connecting to EMQX...");

    bool connected = mqttClient.connect(
      MQTT_CLIENT_ID,
      MQTT_USERNAME,
      MQTT_PASSWORD,
      TOPIC_STATUS,
      1,
      true,
      "OFFLINE"
    );

    if (connected) {
      Serial.println("connected!");

      mqttClient.publish(
        TOPIC_STATUS,
        "ONLINE",
        true
      );

      Serial.println("STATUS PUBLISHED: ONLINE");

      if (mqttClient.subscribe(TOPIC_COMMAND)) {
        Serial.println("Subscribed to LED commands!");
      } else {
        Serial.println("Failed to subscribe to commands!");
      }

    } else {
      Serial.print("FAILED. MQTT state = ");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

// ===== TEMPERATURE AND VIBRATION =====
void readSensors() {

  // Normal DHT22 reading
  float temperature = dht.readTemperature();

  // ===== NEW: SIMULATE SENSOR FAILURE =====
  if (TEST_INVALID_TEMPERATURE) {
    temperature = NAN;
    Serial.println("TEST: DHT22 failure simulated");
  }

  // ===== TEMPERATURE VALIDATION =====
  bool tempValid =
    isfinite(temperature) &&
    temperature >= -40.0 &&
    temperature <= 80.0;

  // ===== MPU6050 =====
  float vibration = NAN;

  if (mpuReady) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float x = a.acceleration.x;
    float y = a.acceleration.y;
    float z = a.acceleration.z;

    float magnitude = sqrt(x*x + y*y + z*z);
    vibration = fabs(magnitude - 9.81);
  }

  bool vibValid =
    isfinite(vibration) &&
    vibration >= 0 &&
    vibration <= 100;

  // ===== AUTOMATIC LED =====
  bool alert =
    !tempValid ||
    !vibValid ||
    (tempValid && temperature >= TEMP_LIMIT) ||
    (vibValid && vibration >= VIB_LIMIT);

  automaticAlert = alert;
  updateLED();

  // ===== SERIAL MONITOR =====
  Serial.println("------------------------");

  if (tempValid) {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");
  } else {
    Serial.println("Temperature: SENSOR ERROR");
  }

  if (vibValid) {
    Serial.print("Vibration: ");
    Serial.print(vibration);
    Serial.println(" m/s2");
  } else {
    Serial.println("Vibration: SENSOR ERROR");
  }

  // ===== PUBLISH TO EMQX =====
  if (mqttClient.connected()) {
    char message[20];

    if (tempValid) {
      snprintf(message, sizeof(message), "%.2f", temperature);
      mqttClient.publish(TOPIC_TEMP, message);
    } else {
      Serial.println(
        "Invalid temperature: MQTT publish skipped"
      );
    }

    if (vibValid) {
      snprintf(message, sizeof(message), "%.2f", vibration);
      mqttClient.publish(TOPIC_VIB, message);
    } else {
      Serial.println(
        "Invalid vibration: MQTT publish skipped"
      );
    }

    Serial.println("Sensor validation completed");
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  dht.begin();

  Wire.begin(SDA_PIN, SCL_PIN);
  mpuReady = mpu.begin();

  if (mpuReady) {
    Serial.println("MPU6050 ready!");
  } else {
    Serial.println("MPU6050 not detected!");
  }

  connectWiFi();

  // ===== TLS CERTIFICATE VERIFICATION =====
  if (!synchronizeClock()) {
    Serial.println("TLS setup stopped: invalid clock.");
    while (true) {
      delay(1000);
    }
  }

  espClient.setCACert(MQTT_CA);

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setKeepAlive(60);
  mqttClient.setCallback(mqttCallback);

  connectMQTT();

  Serial.println("Motor monitoring started!");
}

// ===== LOOP =====

void loop() {

  // ===== WIFI INTERRUPTION TEST =====
  static bool testDone = false;

  // Start test once, after 20 seconds
  if (!testDone && millis() >= 20000 &&
      mqttClient.connected()) {

    testDone = true;

    Serial.println("TEST: WiFi disconnected!");

    // Turn off the ESP32 WiFi
    WiFi.disconnect(true);

    // Keep WiFi off long enough for MQTT Last Will
    delay(110000);

    Serial.println("TEST: WiFi restored!");

    // Restore WiFi radio.
    // Existing reconnection code will connect automatically.
    WiFi.mode(WIFI_STA);
  }

  // ===== ORIGINAL CODE =====
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    connectMQTT();
  }

  mqttClient.loop();

  if (millis() - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = millis();
    readSensors();
  }

  delay(10);
}
