#define DEBUG

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <HC_Frame.h>

iotdevice myDevice;

IPAddress ip(192,168,0,10); // SWI is first in name, so we choose SWI address range
 // gateway and subnet stay usually the same for all modules !!
IPAddress gateway(192,168,0,1);  // Your router IP
IPAddress subnet(255,255,255,0); // Subnet mask
IPAddress retIP;

// WiFi credentials ------------------------------------------------
const char* ssid = "TheCore";
const char* password = "M1n0tauru5";
const char* mqtt_server = "192.168.0.5";


void defineDevice(){
  // define device type and name -----------------------------------
  myDevice.type = "SWIDIM";
  myDevice.freeName = "Aisle";
  // define switch and dim channels --------------------------------
  //myDevice.numSwitchChannels = 2;
  //myDevice.numPWMChannels = 2;
  myDevice.GPIO[0] = 2;
  myDevice.GPIO[1] = 5;
  myDevice.numGPIO = 2;
  myDevice.PWM[0] = 2;
  myDevice.PWM[1] = 5;
  myDevice.numPWM = 2;
  // define the messages the device is subscribing to to control it ----
  myDevice.numSubscribeMessages = 4;
  myDevice.subscribeMessages[0] = myDevice.type+myDevice.freeName+"/Switch1";
  myDevice.subscribeMessages[1] = myDevice.type+myDevice.freeName+"/Switch2";
  myDevice.subscribeMessages[2] = myDevice.type+myDevice.freeName+"/Brightness1";
  myDevice.subscribeMessages[3] = myDevice.type+myDevice.freeName+"/Brightness2";
  // define the device "state" and value messages to be published -----------------
  myDevice.publishStatusMessages[0] = myDevice.type+myDevice.freeName+"/stateSwitch1";
  myDevice.publishStatusMessages[1] = myDevice.type+myDevice.freeName+"/stateSwitch2";
  myDevice.numStatusMessages = 2;
  myDevice.publishValueMessages[0] = myDevice.type+myDevice.freeName+"/stateBrightness1";
  myDevice.publishValueMessages[1] = myDevice.type+myDevice.freeName+"/stateBrightness2";
  myDevice.numValueMessages = 2;


myDevice.channels[0].name = "LampLeft";
myDevice.channels[0].ChannelType = SWITCHCH;
myDevice.channels[0].Direction = OUTPUT;
myDevice.channels[0].GPIO = 2; // no. of HW pin
myDevice.channels[0].isInverted = 1;
myDevice.channels[0].PWM = 2;  // no. of HW PWM pin
myDevice.channels[0].isPWMChannel = 1; // it is a HW PWM channels
myDevice.channels[0].PWMFrequency = 5000;
myDevice.channels[0].PWMResolution = 10;
myDevice.channels[0].StatusMessage = myDevice.type+myDevice.freeName+"/stateSwitch1";
myDevice.channels[0].subscribeMessage = myDevice.type+myDevice.freeName+"/Switch1";

myDevice.numChannels = 4;
}

int otaRunning = 0;

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

void reconnect() {
  int mqtt_reconnectcnt = 0;

  // Loop until we're reconnected
  while (!client.connected()) {
    // Create a random client ID
    String clientId = myDevice.type + myDevice.freeName;
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      HC_subscribeAll(myDevice);
      #ifdef DEBUG
        Serial.println("Subsribed to all device messages!");
      #endif
    } else {  
      delay(2000); // Wait 2 seconds before retrying
      mqtt_reconnectcnt ++;
      #ifdef DEBUG
        Serial.print("Reconnect cnt: "); Serial.println(mqtt_reconnectcnt);
      #endif
      if (mqtt_reconnectcnt > 10){     
       ESP.restart();
      }
    }
  }
  #ifdef DEBUG
    Serial.println("MQTT connection established !");
  #endif
  delay(100);
}

void setupWiFi() {
  // connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  retIP = WiFi.localIP();

  #ifdef DEBUG
    Serial.print("\n\rconnected to: "); Serial.print(ssid); Serial.print(" IP: "); Serial.println(retIP);
  #endif  
}

void HC_callback(char* topic, byte* payload, unsigned int length) {
  String retStr;

  for (unsigned int i=0;i<6;i++) c_val[i]=0; // clear buffer

  retStr = HC_activateChannelbyMessage(myDevice, 0, SWITCHCH, topic, length, payload);
  #ifdef DEBUG
    Serial.print("Switch channel activation by message: "); Serial.println(retStr);
  #endif 

  retStr = HC_activateChannelbyMessage(myDevice, 2, VALUECH, topic, length, payload);
  #ifdef DEBUG
    Serial.print("Value channel activation by message: "); Serial.println(retStr);
  #endif 

/*
  retStr = HC_checkSubscribeMessage(myDevice, 0, topic, length, payload);
  if (retStr != "NAM"){ // it is a valid message content
   myDevice.stateVar[0] = retStr.toInt();
   HC_setGPIO(myDevice, 0, 1, myDevice.stateVar[0]);
   HC_publishStateMessage(myDevice,0);
  }
  #ifdef DEBUG
    Serial.print("Callback return Msg 1: "); Serial.println(retStr);
  #endif

  retStr = HC_checkSubscribeMessage(myDevice, 1, topic, length, payload);
  if (retStr != "NAM"){ // it is a valid message content
    myDevice.stateVar[1] = retStr.toInt();
    HC_publishStateMessage(myDevice,1);
  }  
  #ifdef DEBUG
    Serial.print("Callback return Msg 2: "); Serial.println(retStr);
  #endif

  retStr = HC_checkSubscribeMessage(myDevice, 2, topic, length, payload);
  if (retStr != "NAM"){ // it is a valid message content
    myDevice.valueVar[0] = retStr.toFloat();
    HC_setPWMChannel(myDevice, 0);
    HC_publishValueMessage(myDevice,0);
  }  
  #ifdef DEBUG
    Serial.print("Callback return Msg 3: "); Serial.println(retStr);
  #endif

  retStr = HC_checkSubscribeMessage(myDevice, 3, topic, length, payload);
  if (retStr != "NAM"){ // it is a valid message content
    // value conversion takes place here
    myDevice.valueVar[1] = retStr.toFloat();
    HC_publishValueMessage(myDevice,1);
  }  
  #ifdef DEBUG
    Serial.print("Callback return Msg 4: "); Serial.println(retStr);
  #endif  
  */
}  

void setup() {
  #ifdef DEBUG
    Serial.begin(9600);
  #endif  

  defineDevice();

  HC_initallGPIOOut(myDevice);
  
  setupWiFi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(HC_callback);
}

unsigned long previousMillis = 0;
const long interval = 2000;  

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  ArduinoOTA.handle();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
  
    //HC_publishStatus(myDevice);
  }

}