//Include relevant libraries
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>


//Enable secure connection with ThingSpeak and include ThingSpeak library
#define TS_ENABLE_SSL 
#include <ThingSpeak.h>

//TMP36 temperature sensor attached to ESP8266 Pin A0
#define TMPPin A0

//Create an interger variable for motion count
int motionCount;

//PIR sensor attached to ESP8266 Pin 16
int sensor = 16;

//Local Wifi name and password
const char* ssid = "*****";
const char* password = "*****";

//Webhook server names for espON and espOFF
const char* onIFTTTserverName = "http://maker.ifttt.com/trigger/espON/with/key/*****";
const char* offIFTTTserverName = "http://maker.ifttt.com/trigger/espOFF/with/key/*****";

//Start wifi client
WiFiClient  client;

//Thingspeak channel number and api key
unsigned long myChannelNumber = *****;
const char * myWriteAPIKey = "*****";

// For Timer (16 seconds)
unsigned long lastTime = 0;
unsigned long timerDelay = 16000;

// Outside Temperature variable
float outsideTemp; //Outside Temperature

// Openweather API key, city and country coce
String openWeatherMapApiKey = "******";
String city = "London";
String countryCode = "GB";

// jsonBuffer variabl for handling openweather JSON
String jsonBuffer;

void setup() {
  //Start Serial
  Serial.begin(115200);
  //Set sensor pin pinmode to input
  pinMode(sensor, INPUT);
  //Setup Wifi Connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
 }

void loop() {
  
    // Check WiFi connection status and begin loop
    if(WiFi.status()== WL_CONNECTED){

      //Get JSON file from openweather API
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
      jsonBuffer = httpGETRequest(serverPath.c_str());
      JSONVar myObject = JSON.parse(jsonBuffer);
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }

      //For 16 seconds sample motion detected and save value as 'motioncount'
      for(int i=0; i<16; i++){
        long state = digitalRead(sensor);
        if (state == HIGH) {
          Serial.println("Motion");
          motionCount += 1;
        }
        delay(1000);
      }
      Serial.println(motionCount);

      //Parse the JSON file and save temperature at 'outsideTemp'
      outsideTemp = int(myObject["main"]["temp"])- 275;
      Serial.print("Outside Temperature (C): ");
      Serial.println(outsideTemp);

      //Get internal temperature value
      int tmpValue = analogRead(TMPPin);
      float roomTemp = (((tmpValue * 3.3)/1024)-0.5)*10;// converting TMP Reading to C;
      Serial.print("Room Temperature (C): ");
      Serial.println(roomTemp);

      //Send data to ThingSpeak
      ThingSpeak.begin(client);
      ThingSpeak.setField(1, outsideTemp);
      ThingSpeak.setField(2, roomTemp);
      ThingSpeak.setField(3, motionCount);
      int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

      //If temperature is above 20 or below 16, a HTTP POST request is sent to IFTTT via webhook
      if(roomTemp > 20){
        WiFiClient client;
        HTTPClient http; 
        http.begin(client, onIFTTTserverName);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        String httpRequestData = "value1=" + String(random(40)) + "&value2=" + String(random(40))+ "&value3=" + String(random(40));           
        int httpResponseCode = http.POST(httpRequestData);
        Serial.println(httpResponseCode);
        http.end();
      }
      if(roomTemp < 16){
        WiFiClient client;
        HTTPClient http; 
        http.begin(client, offIFTTTserverName);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        String httpRequestData = "value1=" + String(random(40)) + "&value2=" + String(random(40))+ "&value3=" + String(random(40));           
        int httpResponseCode = http.POST(httpRequestData);
        Serial.println(httpResponseCode);
        http.end();
      }

      //reset motion Count to zero
      motionCount = 0;
    }
    else {
      Serial.println("WiFi Disconnected");
    }
}

//Code for sending HTTP GET Request to openweather
String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    //Serial.print("HTTP Response code: ");
    //Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code (httepResponseCode): ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}
