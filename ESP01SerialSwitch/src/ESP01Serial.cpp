// Switch with ESP-01 WiFi module and serial control
// chinese board with 8vpin STC 16F104 MCU
// (c) C.Bjoernsen, 2022
// Switch with ESP-01 WiFi module and serial control
// chinese board with 8 pin STC 16F104 MCU
// (c) C.Bjoernsen, 2022

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

// WiFi credentials
const char *ssid = "TheCore";
const char *password = "M1n0tauru5";
const char *mqtt_server = "192.168.0.5";

IPAddress ip(192,168,0,48); // fixed IP for the device
 // gateway and subnet stay the same for all modules !!
IPAddress gateway(192,168,0,1);  // Your router IP
IPAddress subnet(255,255,255,0); // Subnet mask
IPAddress retIP;

WiFiClient ESP01Client;
PubSubClient client(ESP01Client);

char c_val[6];    // buffer for MQTT messages payload
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

char subscribe_buffer[MSG_BUFFER_SIZE+5];
char publish_buffer[MSG_BUFFER_SIZE+5];
char compare_buffer[MSG_BUFFER_SIZE+5];
String subscribe_str;
String publish_str;
String compare_str;
String systemId;
int mqtt_reconnectcnt = 0;

// this is for the ESP Relay module, one relay, which uses a serial connection to a microcontroller
// which controls the relay, baud rate is 9600 baud
  byte relON[] = {0xA0, 0x01, 0x01, 0xA2};  //Hex command to send to serial for open relay
  byte relOFF[] = {0xA0, 0x01, 0x00, 0xA1}; //Hex command to send to serial for close relay

void setupWiFi(){
    // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    WiFi.config(ip, gateway, subnet);
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Connecting to WiFi..");
      //WiFi.begin(ssid, password);
      delay(500);
    }
    Serial.print("WiFi connection successful !! IP:");
    Serial.println(WiFi.localIP());   
    retIP = WiFi.localIP();
}

void callback(char* topic, byte* payload, unsigned int length) {
  for (unsigned int i=0;i<6;i++) c_val[i]=0; 

  publish_str = systemId+"/getState";
  publish_str.toCharArray(publish_buffer,publish_str.length()+1);
  
  compare_str=systemId+"/setstate";
  compare_str.toCharArray(compare_buffer,compare_str.length()+1);
  
  if (strcmp(topic, compare_buffer) == 0){
    //Serial.print("received message: "); Serial.print(topic); Serial.println();
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }
    if (atof(c_val) == 0){
      Serial.write(relOFF, sizeof(relOFF));
      
      snprintf (msg, MSG_BUFFER_SIZE, "%d", 0); // copy payload value into msg buffer
      client.publish(publish_buffer, msg); // publish the status message
    } else {
      Serial.write(relON, sizeof(relON));

      snprintf (msg, MSG_BUFFER_SIZE, "%d", 1); // copy payload value into msg buffer
      client.publish(publish_buffer, msg);
    }
  }  
  // ------ IP request message -----------------------------------------------------------
  compare_str=systemId+"/IP";
  compare_str.toCharArray(compare_buffer,compare_str.length()+1);
  if (strcmp(topic,compare_buffer) == 0){
      for (unsigned int i=0;i<length;i++){
        c_val[i]=(char)(payload[i]);   
      }  
      if (atoi(c_val) == 1){
         String a = retIP.toString();
         Serial.print("IP String: "); Serial.println(a);
         
         snprintf (msg, MSG_BUFFER_SIZE, "%d", retIP[0]); // copy payload value into msg buffer
         publish_str = systemId + "/IP0"; // compose topic
         publish_str.toCharArray(publish_buffer,publish_str.length()+1);
         client.publish(publish_buffer,msg); 
         
         snprintf (msg, MSG_BUFFER_SIZE, "%d", retIP[1]); // copy payload value into msg buffer
         publish_str = systemId + "/IP1"; // compose topic
         publish_str.toCharArray(publish_buffer,publish_str.length()+1);
         client.publish(publish_buffer,msg); 
         Serial.print("Buffer IP1: "); Serial.print(msg);

         snprintf (msg, MSG_BUFFER_SIZE, "%d", retIP[2]); // copy payload value into msg buffer
         publish_str = systemId + "/IP2"; // compose topic
         publish_str.toCharArray(publish_buffer,publish_str.length()+1);
         client.publish(publish_buffer,msg); 

         snprintf (msg, MSG_BUFFER_SIZE, "%d", retIP[3]); // copy payload value into msg buffer
         publish_str = systemId + "/IP3"; // compose topic
         publish_str.toCharArray(publish_buffer,publish_str.length()+1);
         client.publish(publish_buffer,msg); 
      } 
  //------------------------------------------------------------------------------------------------    
  }     
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Create a random client ID
    String clientId = "ESP-01SWI";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      subscribe_str = systemId+"/IP";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);
      subscribe_str = systemId+"/setstate";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);      
    } else {  
      // Wait 2 seconds before retrying
      delay(2000);
      mqtt_reconnectcnt ++;
      if (mqtt_reconnectcnt > 10){     
       ESP.restart();
      }
    }
  }
  Serial.println("MQTT connection established !");
  delay(100);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); // max. baudrate for STC 16F104
  pinMode(2, OUTPUT); // onboard LED, low active
  
  systemId = "SWI"+String(ESP.getChipId());
  Serial.print("System ID: "); Serial.println(systemId);
  digitalWrite(2, LOW);
  setupWiFi();
  digitalWrite(2, HIGH);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}