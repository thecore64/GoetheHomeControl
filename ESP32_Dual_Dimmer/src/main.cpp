// =========================================================================================
// -----------------------------------------------------------------
// Framework (kind of) for different devices to control smart home
// (c) C. Bjoernsen, Ingolstadt, May 2022
// -----------------------------------------------------------------
// ESP32 implementation
// -----------------------------------------------------------------
#include <Arduino.h>
#include <ESPmDNS.h>
#include "WiFiUdp.h"
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <stdio.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

// device type definition
// =======================
// SWI = Switch
// CMD = command device (with buttons to perform switching functions)
// DIM = Dimmer with PWM channels
// SNS = Sensor device
//    sensors can be:
//      HUM = humidity 
//      TEM = temperature
//      PRS = pressure
//      BRI = brightness
// ACT = Actor device
//
// combined devices
// ==================
// e.g. SWI2DIM1 two switches combined with one dimmer
//      SWI3SNS1HUM three switch combined with one humidity sensor
//      CMD2 is a device with two command buttons
// a sensor device with a BME280 sensor would be SNS3HUMTEMPRS, three sensor channels
//
// -----------------------------------------------------------------------------------
// this device is SWI2DIM2 2 dimmer channels (which are 2 switches also !)
// -----------------------------------------------------------------------------------

#define DEBUG

#define EEPROM_SIZE 64

#define LED1 21
#define LED2 17
// define constants for PWM control
#define PWMChannel 0
#define PWMResolution 10
#define PWMFrequency 1000

typedef struct {
  String    chipID;
  String    systemID;
  String    type;
  int       numSwitchChannels;
  int       numPWMChannels;
  int       numButtons;
  int      numSubscribeMessages;
  unsigned int  publishInterval;
  int       btnMsgperRelease[6];
  int       btnState[6];
  int       btnPressCnt[6];
  String    btnMessages[4][6];   // messages to be sent on btn. activation 
  String    publishMessages[10]; // general device message, like RSSI, alive, etc.
  String    subscribeMessages[10];
  IPAddress deviceIP;
} iotdevice;

// ESP32 has no ChipId function
// ESP32's chip id is atually it's mac address
// use function getESP32ChipId()
//const String chipId = String(ESP.getChipId());

const char* ssid = "TheCore";
const char* password = "M1n0tauru5";
const char* mqtt_server = "192.168.0.5";

char c_val[6];    // buffer for dealing with MQTT messages payload
#define MSG_BUFFER_SIZE  (50)  
char msg[MSG_BUFFER_SIZE];  // general MQTT message buffer

char publish_buffer[MSG_BUFFER_SIZE+5];
char subscribe_buffer[MSG_BUFFER_SIZE+5];
char compare_buffer[MSG_BUFFER_SIZE+5];

String publish_str;
String subscribeStr;
String compare_str;

int otaRunning = 0;

int bright1 = 0;
int bright2 = 0;
int switch1 = 0;
int switch2 = 0;

iotdevice device[1];

IPAddress ip(192,168,0,60);

 // gateway and subnet stay the same for all modules !!
IPAddress gateway(192,168,0,1);  // Your router IP
IPAddress subnet(255,255,255,0); // Subnet mask
IPAddress retIP;

WiFiClient esp32Client;
PubSubClient client(esp32Client);

uint32_t getESP32ChipId(){
  uint32_t chipId = 0;

  for(int i=0; i<17; i=i+8) {
	  chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
	}
#ifdef DEBUG
  Serial.print("ESP32 Chip ID:"); Serial.println(chipId);
#endif  
  return chipId;
}

void callback(char* topic, byte* payload, unsigned int length) {
  for (unsigned int i=0;i<6;i++) c_val[i]=0; 

  device[0].subscribeMessages[4].toCharArray(compare_buffer,device[0].subscribeMessages[4].length()+1);  
  if (strcmp(topic, compare_buffer) == 0){
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }
    if (atoi(c_val)==1){
        snprintf (msg, MSG_BUFFER_SIZE, "%d", bright1); // copy payload value into msg buffer
        publish_str = device[0].systemID + "/Brightness1"; // compose topic
        publish_str.toCharArray(publish_buffer,publish_str.length()+1);
        client.publish(publish_buffer,msg); 

        snprintf (msg, MSG_BUFFER_SIZE, "%d", bright2); // copy payload value into msg buffer
        publish_str = device[0].systemID + "/Brightness2"; // compose topic
        publish_str.toCharArray(publish_buffer,publish_str.length()+1);
        client.publish(publish_buffer,msg); 
    }
  }

  device[0].subscribeMessages[0].toCharArray(compare_buffer,device[0].subscribeMessages[0].length()+1);  
  if (strcmp(topic, compare_buffer) == 0){
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }
    bright1 = atoi(c_val);
  }

  device[0].subscribeMessages[1].toCharArray(compare_buffer,device[0].subscribeMessages[1].length()+1);  
  if (strcmp(topic, compare_buffer) == 0){
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }    
    bright2 = atoi(c_val);
  }

  device[0].subscribeMessages[2].toCharArray(compare_buffer,device[0].subscribeMessages[2].length()+1);  
  if (strcmp(topic, compare_buffer) == 0){
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }    
    switch1 = atoi(c_val);
    if (switch1 == 0){
      bright1 = 0;
    }
#ifdef DEBUG
  Serial.print("Switch 1:");Serial.println(switch1);    
#endif  
  }

  device[0].subscribeMessages[3].toCharArray(compare_buffer,device[0].subscribeMessages[3].length()+1);  
  if (strcmp(topic, compare_buffer) == 0){
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }    
    switch2 = atoi(c_val);
    if (switch2 == 0){
      bright2 = 0;
    }
#ifdef DEBUG
  Serial.print("Switch 2:");Serial.println(switch2);    
#endif  
  }

  ledcWrite(PWMChannel, bright1);
  EEPROM.writeInt(0, bright1);
  EEPROM.commit();
#ifdef DEBUG
  Serial.print("EEPROM write brightness 1 "); Serial.println(bright1);
#endif  
  ledcWrite(PWMChannel+1, bright2);
  EEPROM.writeInt(sizeof(bright2), bright2);
  EEPROM.commit();
  compare_str=device[0].systemID+"/IP";
  compare_str.toCharArray(compare_buffer,compare_str.length()+1);
#ifdef DEBUG
  Serial.println(compare_str);
#endif  

  if (strcmp(topic,compare_buffer) == 0){
      for (unsigned int i=0;i<length;i++){
        c_val[i]=(char)(payload[i]);   
      }  

      if (atoi(c_val) == 1){

         String a = retIP.toString();
         Serial.print("IP String: "); Serial.println(a);

         snprintf (msg, MSG_BUFFER_SIZE, "%d", retIP[0]); // copy payload value into msg buffer
         publish_str = device[0].systemID + "/IP0"; // compose topic
         publish_str.toCharArray(publish_buffer,publish_str.length()+1);
         client.publish(publish_buffer,msg); 
         snprintf (msg, MSG_BUFFER_SIZE, "%d", retIP[1]); // copy payload value into msg buffer
         publish_str = device[0].systemID + "/IP1"; // compose topic
         publish_str.toCharArray(publish_buffer,publish_str.length()+1);
         client.publish(publish_buffer,msg); 
         snprintf (msg, MSG_BUFFER_SIZE, "%d", retIP[2]); // copy payload value into msg buffer
         publish_str = device[0].systemID + "/IP2"; // compose topic
         publish_str.toCharArray(publish_buffer,publish_str.length()+1);
         client.publish(publish_buffer,msg);                            
         snprintf (msg, MSG_BUFFER_SIZE, "%d", retIP[3]); // copy payload value into msg buffer
         publish_str = device[0].systemID + "/IP3"; // compose topic
         publish_str.toCharArray(publish_buffer,publish_str.length()+1);
         client.publish(publish_buffer,msg); 
      }
  }     
}

void setup_wifi() {
  delay(10);
  // connecting to a WiFi network

#ifdef DEBUG
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);
#endif

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if(WiFi.status() == WL_CONNECTED){ //If WiFi connected to hot spot then start mDNS
#ifdef DEBUG
      Serial.print(" ESP IP: "); Serial.println(WiFi.localIP());  
#endif      
    }
  retIP = WiFi.localIP();
}

void setup_Subscribe(){
    for (int i=0; i<device[0].numSubscribeMessages; i++){
      subscribeStr = device[0].subscribeMessages[i];
      subscribeStr.toCharArray(subscribe_buffer,subscribeStr.length()+1);
      client.subscribe(subscribe_buffer);
#ifdef DEBUG
  Serial.print("Subscribe No.: "); Serial.print(i); Serial.print(" Message: "); Serial.println(subscribe_buffer);
#endif
    }
} 

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {

#ifdef DEBUG
    Serial.println("Attempting MQTT connection...");
#endif

    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

#ifdef DEBUG
    Serial.print("MQTT Client ID: "); Serial.println(clientId);
#endif

    // Attempt to connect
    if (client.connect(clientId.c_str())) {
        setup_Subscribe(); 
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

void setupOTA(){

  ArduinoOTA.setPort(8266);
  
  ArduinoOTA.onStart([]() {
    digitalWrite(15, HIGH);
    otaRunning = 1;
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    digitalWrite(15,LOW);
    Serial.println("\nEnd");
    otaRunning = 0;
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();

}

void defineDeviceCapabilities(void){ 
  device[0].chipID = String(getESP32ChipId());
  device[0].numSwitchChannels = 2;
  device[0].numPWMChannels = 2;
  device[0].numButtons = 0;
  device[0].type = "DIM";
  device[0].systemID = device[0].type+device[0].chipID;
  device[0].publishInterval = 10000; // interval in milliseconds

  device[0].btnMsgperRelease[0] = 0;
  device[0].btnMsgperRelease[1] = 0;
  device[0].btnMsgperRelease[2] = 0;
  device[0].btnMsgperRelease[3] = 0;
  device[0].btnMsgperRelease[4] = 0;
  device[0].btnMsgperRelease[5] = 0;
  // --- define messages to be sent on button activation ----------------------
  // --- first array index is button number 
  // --- second array index is message number, acc. to BtnMsgperRelease[buttonNumber]
  // --- ,number of messages send per button release 
  // NO BUTTON MESSAGES FOR THIS DEVICE !!
  device[0].btnMessages[0][0] = ""; 
  device[0].btnMessages[1][0] = "";
  device[0].btnMessages[2][0] = "";
  device[0].btnMessages[3][0] = "";
  device[0].btnMessages[4][0] = "";  

  device[0].publishMessages[0]="DIM12967876/StateBrightness1";
  device[0].publishMessages[1]="DIM12967876/StateBrightness2";
  device[0].publishMessages[2]="DIM12967876/State1";
  device[0].publishMessages[3]="DIM12967876/State2";

  device[0].subscribeMessages[0]="DIM12967876/Brightness1";  // switch status of left LED strip
  device[0].subscribeMessages[1]="DIM12967876/Brightness2";  // switch status of right LED strip
  device[0].subscribeMessages[2]="DIM12967876/Switch1";    // switch status of aisle lamp 1
  device[0].subscribeMessages[3]="DIM12967876/Switch2";    // switch status of aisle lamp 2
  device[0].subscribeMessages[4]="DIM12967876/GetState";
  device[0].subscribeMessages[5]="DIM12967876/IP";
  device[0].numSubscribeMessages = 6;

}

void setup() {

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  EEPROM.begin(EEPROM_SIZE);
  bright1 = EEPROM.readInt(0);
  bright2 = EEPROM.readInt(sizeof(bright2));

  // setup ESP32 PWM control
  ledcAttachPin(LED1, PWMChannel);
  ledcAttachPin(LED2, PWMChannel+1);
  ledcSetup(PWMChannel, PWMFrequency, PWMResolution); // PWM channel 0, 1kHz, 10 bit resolution 
  ledcSetup(PWMChannel+1, PWMFrequency, PWMResolution); // PWM channel 1, 1kHz, 10 bit resolution 
  
  ledcWrite(PWMChannel, bright1);
  ledcWrite(PWMChannel+1, bright2);
    
  // put your setup code here, to run once:
  Serial.begin(9600);
  WiFiManager wifiManager;
  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  res = wifiManager.autoConnect("AutoConnectAP"); // anonymous ap
  
  defineDeviceCapabilities();
  
  //setup_wifi();

  device[0].deviceIP = WiFi.localIP();

#ifdef DEBUG
  Serial.print("ESP32 local IP:"); Serial.println(WiFi.localIP());
  Serial.print("System ID:"); Serial.println(device[0].systemID);
#endif

  setupOTA();
  
  // define SSID for WiFi Manager access point
 // device[0].systemID.toCharArray(publish_buffer,device[0].systemID.length()+1);
 // wifiManager.autoConnect(publish_buffer);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  // put your main code here, to run repeatedly:
  if (otaRunning == 0){
   if (!client.connected()) {
     reconnect();
   }
   client.loop();
  }

  ArduinoOTA.handle();
}
