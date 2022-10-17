#include <Arduino.h>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Wifi.h"

//LED Pins
#define PIN_RED    23 // GIOP23
#define PIN_GREEN  22 // GIOP22
#define PIN_BLUE   21 // GIOP21

//AWS & WiFi Config
#define AWS_IOT_PUBLISH_TOPIC   "evryhub/vent"
#define AWS_IOT_SUBSCRIBE_TOPIC "evryhub/vent"
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

void waterGarden(){
  delay(500);
}

void configGarden(){
  delay(500);
}

void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);

  const char* device = doc["device"];
  const char* action = doc["action"];

  if(String(device) == "garden" && String(action) == "config"){
    Serial.println("Configure EvryHub Garden");
    configGarden();
  }else if(String(device) == "garden" && String(action) == "water"){
    Serial.println("Water Garden");
    waterGarden();
  }
}

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // LED Turns Green When Connected to WiFi
  analogWrite(PIN_RED, 0);
  analogWrite(PIN_GREEN, 255);
  analogWrite(PIN_BLUE, 0);

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  // Create a message handler
  client.setCallback(messageHandler);
  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("AWS IoT Connected!");

  // LED Turns Blue When Connected to AWS
  analogWrite(PIN_RED, 0);
  analogWrite(PIN_GREEN, 0);
  analogWrite(PIN_BLUE, 255);
}
 
void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["device"] = "garden";
  doc["action"] = "moisture:";
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);

  // LED Turns RED When Device is Turned On
  analogWrite(PIN_RED, 255);
  analogWrite(PIN_GREEN, 0);
  analogWrite(PIN_BLUE, 0);
  
  connectAWS();
}

void loop() {
  //Read Soil Moisture


  publishMessage();
  client.loop();
  delay(5000);
}