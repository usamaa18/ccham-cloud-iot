/*
    This sketch sends data via HTTP GET requests to data.sparkfun.com service.

    You need to get streamId and privateKey at data.sparkfun.com and paste them
    below. Or just customize this script to talk to other HTTP servers.

*/
#include <NTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include "base64.hpp"
//#include <ArduinoJson.h>
#include "CloudIoTCoreDevice.h"
#include "ciotc_config.h" // Update this file with your configuration

#define CLOUD_IOT_CORE_HTTP_HOST "cloudiotdevice.googleapis.com"
//#define CLOUD_IOT_CORE_HTTP_HOST "ptsv2.com"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
CloudIoTCoreDevice *device;

const char* ssid     = "SM-G973W3730";
const char* password = "7802983369";

const int httpsPort = 443;
String apiPath;

void setup()
{
  ESP.wdtDisable();
  Serial.begin(115200);
  delay(10);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  device = new CloudIoTCoreDevice(
    project_id, location, registry_id, device_id,
    private_key_str);
  apiPath = device->getSendTelemetryPath();
  //    apiPath = "/t/mthmc-1647814307/post";

}

void sendToCloud(unsigned char string[]) {
  WiFiClientSecure wifi;
  wifi.setInsecure();//skip verification
  long long int timeNow = timeClient.getEpochTime();
  
  wdt_reset();
  wdt_disable();
  String jwt = device->createJWT(timeNow, jwt_exp_secs);
  wdt_enable(0);
  wdt_reset();
  unsigned char base64[21]; // 20 bytes for output + 1 for null terminator
  // encode_base64() places a null terminator automatically, because the output is a string
  
  int base64_length = encode_base64(string, strlen((char *) string), base64);
  String body = String("{\"binary_data\":\"") + (char *) base64 + "\"}";

  if (!wifi.connect(CLOUD_IOT_CORE_HTTP_HOST, httpsPort)) {
    Serial.println("Connection failed!");
  }
  else {
    Serial.println("Connected to server!");
    wifi.println("POST " + apiPath + " HTTP/1.1");
    wifi.println(String("host: ") + CLOUD_IOT_CORE_HTTP_HOST);
    wifi.println("authorization: " + String("Bearer ") + jwt);
    wifi.println("content-type: application/json");
    wifi.println("Connection: close");
    wifi.println("cache-control: no-cache");
    wifi.print("Content-Length: ");
    wifi.println(body.length());
    wifi.println();
    wifi.println(body);


    while (wifi.connected()) {
      String line = wifi.readStringUntil('\n');
      if (line == "\r") {
        Serial.println("headers received");
        break;
      }
    }

    while (wifi.available()) {
      char c = wifi.read();
      Serial.write(c);
    }
    wifi.stop();
  }

}

void loop()
{
  timeClient.update();

  unsigned char string[] = "String example";
  sendToCloud(string);
  delay(5000);

}
