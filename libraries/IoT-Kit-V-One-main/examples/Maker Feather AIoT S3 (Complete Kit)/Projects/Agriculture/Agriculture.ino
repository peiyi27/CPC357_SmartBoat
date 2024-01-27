/*
  ESP32 publish telemetry data to VOne Cloud (Agriculture)
*/

#include "VOneMqttClient.h"
#include "DHT.h"

int MinMoistureValue = 4095;
int MaxMoistureValue = 1800;
int MinMoisture = 0;
int MaxMoisture = 100;
int Moisture = 0;

//define device id
const char* DHT11Sensor = "26047d64-3cc8-4c1b-b5bb-de89671b0fca";     //Replace this with YOUR deviceID for the DHT11 sensor
const char* RainSensor = "f2b7151e-924b-4450-8d0c-2566c7c3ddc7";      //Replace this with YOUR deviceID for the rain sensor
const char* MoistureSensor = "905a46e7-6f87-47dd-ae33-c27e75e25a23";  //Replace this with YOUR deviceID for the moisture sensor

//Used Pins
const int dht11Pin = 4;//38;         //Right side Maker Port
const int rainPin = 42;           //Left side Maker Port
const int moisturePin =A2;///42;      //Middle Maker Port

//input sensor
#define DHTTYPE DHT11
DHT dht(dht11Pin, DHTTYPE);

//Create an instance of VOneMqttClient
VOneMqttClient voneClient;

//last message time
unsigned long lastMsgTime = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
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

void setup() {

  setup_wifi();
  voneClient.setup();

  //sensor
  dht.begin();
  pinMode(rainPin, INPUT);
}

void loop() {

  if (!voneClient.connected()) {
    voneClient.reconnect();
    String errorMsg = "DHTSensor Fail";
    voneClient.publishDeviceStatusEvent(DHT11Sensor, true);
    voneClient.publishDeviceStatusEvent(RainSensor, true);
    voneClient.publishDeviceStatusEvent(MoistureSensor, true);
  }
  voneClient.loop();

  unsigned long cur = millis();
  if (cur - lastMsgTime > INTERVAL) {
    lastMsgTime = cur;

    //Publish telemetry data 1
    float h = dht.readHumidity();
    int t = dht.readTemperature();

    JSONVar payloadObject;
    payloadObject["Humidity"] = h;
    payloadObject["Temperature"] = t;
    voneClient.publishTelemetryData(DHT11Sensor, payloadObject);

    //Sample sensor fail message
    //String errorMsg = "DHTSensor Fail";
    //voneClient.publishDeviceStatusEvent(DHT22Sensor, false, errorMsg.c_str());

    //Publish telemetry data 2
    int raining = !digitalRead(rainPin);
    voneClient.publishTelemetryData(RainSensor, "Raining", raining);

    //Publish telemetry data 3
    int sensorValue = analogRead(moisturePin);
    Moisture = map(sensorValue, MinMoistureValue, MaxMoistureValue, MinMoisture, MaxMoisture);
    voneClient.publishTelemetryData(MoistureSensor, "Soil moisture", Moisture);
  }
}
