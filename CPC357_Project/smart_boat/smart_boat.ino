
#include "VOneMqttClient.h"
#include "DHT.h"

const int infraredSensorPin = A2; // Infrared Sensor OUT pin
const int ledPin = 38;            // LED pin
const int buzzerPin = 12;         // Onboard Buzzer pin
const int relayMotorPin = 47;     // Relay control pin
const int dht11Pin = 42;         
const int rainPin = 4;  

unsigned long obstacleStartTime = 0;  // Variable to store the start time of obstacle detection

// Define device ID for VOne
const char* infraredSensorDeviceId = "4a015dbf-9aa6-4825-971f-a389971649e9";
const char* relayMotorDeviceId = "a3f5aa57-4390-463b-81fb-1e456ead4321";
const char* DHT11Sensor = "2de67a72-7610-4332-a25a-b8c87bf287e4";     //Replace this with YOUR deviceID for the DHT11 sensor
const char* RainSensor = "bde8ef1a-2d07-4fc1-8e74-6658738749bf"; 

//input sensor
#define DHTTYPE DHT11
DHT dht(dht11Pin, DHTTYPE);

// Relay motor state
bool relayMotorState = false;

// Create an instance of VOneMqttClient
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

void triggerActuator_callback(const char* actuatorDeviceId, const char* actuatorCommand) {
  //actuatorCommand format {"servo":90}
  Serial.print("Main received callback : ");
  Serial.print(actuatorDeviceId);
  Serial.print(" : ");
  Serial.println(actuatorCommand);

  String errorMsg = "";

  JSONVar commandObjct = JSON.parse(actuatorCommand);
  JSONVar keys = commandObjct.keys();

  if (String(actuatorDeviceId) == relayMotorDeviceId) {

    String key = "";
    bool commandValue = "";
    for (int i = 0; i < keys.length(); i++) {
      key = (const char*)keys[i];
      commandValue = (bool)commandObjct[keys[i]];
      Serial.print("Key : ");
      Serial.println(key.c_str());
      Serial.print("value : ");
      Serial.println(commandValue);
    }

    relayMotorState = commandValue;

    if (relayMotorState) {
      Serial.println("Relay ON");
      digitalWrite(relayMotorPin, true);
    } else {
      Serial.println("Relay OFF");
      digitalWrite(relayMotorPin, false);  // Stop the motor
    }

    voneClient.publishActuatorStatusEvent(actuatorDeviceId, actuatorCommand, true);
  }
}

void setup() {
  setup_wifi();
  voneClient.setup();
  voneClient.registerActuatorCallback(triggerActuator_callback);

  dht.begin();
  pinMode(rainPin, INPUT);
  pinMode(infraredSensorPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(relayMotorPin, OUTPUT);

  Serial.begin(9600); // Initialize Serial communication
}

void loop() {
  if (!voneClient.connected()) {
    voneClient.reconnect();
    String errorMsg = "Sensor Fail";
    voneClient.publishDeviceStatusEvent(infraredSensorDeviceId, true);
    voneClient.publishDeviceStatusEvent(DHT11Sensor, true);
    voneClient.publishDeviceStatusEvent(RainSensor, true);
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

    int rainValue = analogRead(rainPin);
    voneClient.publishTelemetryData(RainSensor, "Raining", rainValue);

  }

  // Check for obstacle using the infrared sensor
  if (digitalRead(infraredSensorPin) == LOW) {
    // Obstacle detected, activate LED and buzzer
    digitalWrite(ledPin, HIGH);
    tone(buzzerPin, 1000); // Activate the onboard buzzer
    Serial.println("Obstacle detected!");

    // Check if the timer is not started
    if (obstacleStartTime == 0) {
      obstacleStartTime = millis();  // Start the timer
    }

    voneClient.publishTelemetryData(infraredSensorDeviceId, "Obstacle", 1);
  } else {
    // No obstacle, turn off LED and buzzer
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);

    // Check if relayMotorDeviceId is true before running the motor
    if (relayMotorState) {
      digitalWrite(relayMotorPin, true);  // Motor runs at full speed
      Serial.println("Motor running.");
    } else {
      digitalWrite(relayMotorPin, false);  // Stop the motor
      Serial.println("Motor not running.");
    }

    voneClient.publishActuatorStatusEvent(relayMotorDeviceId, "Relay", relayMotorState);
    voneClient.publishTelemetryData(infraredSensorDeviceId, "Obstacle",0);
  }

  // Check if the obstacle has been detected for more than 10 seconds
  if (obstacleStartTime != 0 && millis() - obstacleStartTime > 10000) {
    digitalWrite(relayMotorPin, false);  // Stop the motor
    Serial.println("Obstacle detected for more than 10 seconds. Stopping motor.");
    obstacleStartTime = 0; // Reset the obstacle detection timer

    // Publish actuator status to VOne
    voneClient.publishActuatorStatusEvent(relayMotorDeviceId, "Relay", false);
  }

  delay(1000); // Delay for better readability in Serial Monitor
}




