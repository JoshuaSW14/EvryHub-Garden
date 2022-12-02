// Software Capstone - EvryHub Garden
//
// @author  Joshua Symons-Webb
// @id      000812836
//
// I, Joshua Symons-Webb, 000812836 certify that this material is my original work. No
// other person's work has been used without due acknowledgement.

#include <Arduino.h>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// AWS Pub/Sub Topics & Wifi Config
#define AWS_IOT_PUBLISH_TOPIC "evryhub/garden"
#define AWS_IOT_SUBSCRIBE_TOPIC "evryhub/garden/sub"
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

// Moisture Sensor Pin
#define Moisture_PIN 32

// Motor Driver
#define MOTOR_PIN1 25 // GPIO for control input 1
#define MOTOR_PIN2 26 // GPIO for control input 2

// LED Pin (digital)
#define LED_PIN 21

int moisture = 0;
int gardenConfig = 0;
int moistureThreshold = 0;

// ***********************************************************
void waterGarden()
{
  digitalWrite(MOTOR_PIN1, HIGH);
  digitalWrite(MOTOR_PIN2, LOW);
  delay(3000);
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
}

// ***********************************************************
void configGarden(int config)
{
  gardenConfig = config;
}

// ***********************************************************
void gardenThreshold(int threshold)
{
  moistureThreshold = threshold;
}

// ***********************************************************
void messageHandler(char *topic, byte *payload, unsigned int length)
{
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);

  const char *device = doc["device"];
  const char *action = doc["action"];
  const char *value = doc["value"];

  Serial.printf("\n Device: %s | Action: %s | Value: %s\n", String(device), String(action), String(value));

  if (String(device) == "garden" && String(action) == "config")
  {
    Serial.println("Configure EvryHub Garden");
    configGarden(atoi(value));
  }
  else if (String(device) == "garden" && String(action) == "threshold")
  {
    configGarden(atoi(value));
  }
  else if (String(device) == "garden" && String(action) == "water")
  {
    Serial.println("Water Garden");
    waterGarden();
  }
}

// ***********************************************************
void connectAWS()
{
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
  digitalWrite(LED_PIN, LOW);

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("AWS IoT Connected!");
}

// ***********************************************************
void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["device"] = "garden";
  doc["action"] = "moisture";
  doc["value"] = String(moisture);
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

// ***********************************************************
void wifiSetup()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    Serial.print(".");
  }
  digitalWrite(LED_PIN, HIGH);
}

// ***********************************************************
void otaSetup()
{
  ArduinoOTA
      .onStart([]()
               {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ***********************************************************
void checkInput()
{
  if (gardenConfig == 2)
  { // Water with threshold
    if (moistureThreshold > moisture)
    {
      waterGarden();
    }
  }

  float slope = 2.48;      // slope from linear fit
  float intercept = -0.72; // intercept from linear fit
  float voltage = (float(analogRead(Moisture_PIN)) / 1023.0) * 3.3;
  moisture = ((1.0 / voltage) * slope) + intercept;
}

// ***********************************************************
void setup()
{
  Serial.begin(115200);

  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  wifiSetup();
  otaSetup();
  connectAWS();
}

// ***********************************************************
void loop()
{
  ArduinoOTA.handle();
  checkInput();
  publishMessage();
  client.loop();
  delay(5000);
}