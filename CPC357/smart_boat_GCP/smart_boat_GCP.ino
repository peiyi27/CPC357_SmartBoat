#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

const char* WIFI_SSID = "TAN@unifi";
const char* WIFI_PASSWORD = "abcdef5022187";
const char* MQTT_BROKER = "34.133.5.85";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "smart-boat-client";
const int INTERVAL = 5000;  // Define the interval for telemetry data publication

const int infraredSensorPin = A2; // Infrared Sensor OUT pin
const int ledPin = 38;            // LED pin
const int buzzerPin = 12;         // Onboard Buzzer pin
const int relayMotorPin = 47;     // Relay control pin
const int dht11Pin = 42;
const int rainPin = 4;

unsigned long obstacleStartTime = 0;  // Variable to store the start time of obstacle detection

// Define device ID for MQTT topics
const char* infraredSensorDeviceId = "smart-boat/infrared";
const char* DHT11Sensor = "smart-boat/dht11";
const char* RainSensor = "smart-boat/rain";

// Input sensor
#define DHTTYPE DHT11
DHT dht(dht11Pin, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

// Last message time
unsigned long lastMsgTime = 0;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Handle MQTT messages if needed
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected");
      // Subscribe to MQTT topics if needed
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);

  dht.begin();
  pinMode(rainPin, INPUT);
  pinMode(infraredSensorPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(relayMotorPin, OUTPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long cur = millis();
  int infraredValue = digitalRead(infraredSensorPin);
  if (cur - lastMsgTime > INTERVAL) {
    lastMsgTime = cur;

    // Publish telemetry data
    float h = dht.readHumidity();
    int t = dht.readTemperature();

    String payload = "{\"Humidity\":" + String(h) + ",\"Temperature\":" + String(t) + "}";
    client.publish(DHT11Sensor, payload.c_str());

    int rainValue = analogRead(rainPin);
    client.publish(RainSensor, String(rainValue).c_str());
  }

  // Check for obstacle using the infrared sensor
  if (digitalRead(infraredSensorPin) == LOW) {
    // Obstacle detected, activate LED and buzzer
    digitalWrite(ledPin, HIGH);
    tone(buzzerPin, 1000); // Activate the onboard buzzer
    Serial.println("Obstacle detected!");
    client.publish(infraredSensorDeviceId, String(infraredValue).c_str());
    digitalWrite(relayMotorPin, false); // Stop the motor

  } else {
    // No obstacle, turn off LED and buzzer
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);
    digitalWrite(relayMotorPin, true); 
    client.publish(infraredSensorDeviceId, String(infraredValue).c_str());
  }

  delay(1000); // Delay for better readability in Serial Monitor
}

