#ifndef hc_frame_h_included
  #define hc_frame_h_included

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

String publishStr;
String subscribeStr;
String compareStr;

WiFiClient espClient;
PubSubClient client(espClient);

char c_val[6];    // buffer for dealing with MQTT messages payload
#define MSG_BUFFER_SIZE  (60)  
char msg[MSG_BUFFER_SIZE];  // general MQTT message buffer

char publish_buffer[MSG_BUFFER_SIZE+5];
char subscribe_buffer[MSG_BUFFER_SIZE+5];
char compare_buffer[MSG_BUFFER_SIZE+5];
// -----------------------------------------------------------------------------------------
typedef struct {
  String subscribeMessage;
  String StatusMessage;
  int ChannelType; // for now: 0 = switch channel on or off, 1 = value channel, to transfer a float value
  int GPIO;
  int PWM;
  int StateVar;
  int ValueVar

} ch;
// -----------------------------------------------------------------------------------------
typedef struct {
  ch channels[4];
  
  String    chipID;
  String    systemID;
  String    type; //device type
  String    freeName;
  //int       numSwitchChannels;
  //int       numPWMChannels;
  int       GPIO[7];
  int       numGPIO;
  int       PWM[7];
  int       PWMHWChannel[7]; // HW timer channel for PWM generation, only valid for ESP32;
  int       PWMFrequency;    // HW PWM channel frequency, only valid for ESP32
  int       PWMResolution;   // HW PWM channel resolution, only vaild for ESP32
  int       numPWM;          // number of PWM controlled IOs
  int       stateVar[10]; // variable array for switch states
  float     valueVar[10]; // variable array for values, like brightness
  int       numButtons;
  int       numSubscribeMessages;
  unsigned int  publishInterval;
  int       btnMsgperRelease[6];
  int       btnState[6];
  int       btnPressCnt[6];
  String    btnMessages[4][6];     // messages to be sent on btn. activation 
  String    publishStatusMessages[10];   // device status messages to send
  int       numStatusMessages;
  String    publishValueMessages[10];   // device messages to send, for float values
  int       numValueMessages;
  String    subscribeMessages[10]; // messages the device needs to receive
  IPAddress deviceIP;
} iotdevice;


void HC_initallGPIOOut(iotdevice iot){
  int i;
  for (i = 0; i < iot.numGPIO-1; i++) pinMode(iot.GPIO[i], OUTPUT);  
}

void HC_initallGPIOIn(iotdevice iot){
  int i;
  for (i=0; i<iot.numGPIO-1; i++) pinMode(iot.GPIO[i], INPUT);
}

void HC_initGPIO(iotdevice iot, int index, int mode){
  pinMode(iot.GPIO[index], mode);
}

void HC_setGPIO(iotdevice iot, int index, int inv, int state){
  if (inv ==1){
    digitalWrite(iot.GPIO[index], !state);
  } else {
    digitalWrite(iot.GPIO[index], state);
  }
}

void HC_initPWMChannel(iotdevice iot){
  // for ESP8266 no special init for a PWM channel is needed, just define it as output
    for (int i = 0; i < iot.numPWM; i ++) pinMode(iot.PWM[i], OUTPUT);

  // for ESP32 we need a special init, select HW PWM channel and GPIO pin, frequency and duty
  #ifdef ESP32 // def from enviroment
    // setup ESP32 PWM control
    for (int i = 0; i < iot.numPWM; i ++){
      ledcAttachPin(iot.PWM[i] , PWMHWChannel[i]); // set HW PWM channel for each GPIO pin
      ledcSetup(iot.PWMHWChannel[i], iot.PWMFrequency, iot.PWMResolution);
    }
  #endif
}

void HC_setPWMChannel(iotdevice iot, int index){

  #ifdef ESP32
    ledcWrite(iot.PWMChannel[index], iot.valueVar[index]);
  #endif
  
  analogWrite(iot.PWM[index], iot.valueVar[index]); //this will work for ESP8266 only !
}

String HC_checkSubscribeMessage(iotdevice iot,int index, char* topic, unsigned int len, byte* payl){
  iot.subscribeMessages[index].toCharArray(compare_buffer,iot.subscribeMessages[index].length()+1);  
  
  if (strcmp(topic, compare_buffer) == 0){
      for (unsigned int i=0;i < len;i++){
        c_val[i]=(char)(payl[i]);   
      }

      return (String(c_val));
  }
  return("NAM"); // not a valid message
}


void HC_publishStateMessage(iotdevice iot, int index){
      publishStr = iot.publishStatusMessages[index];
      snprintf (msg, MSG_BUFFER_SIZE, "%d", iot.stateVar[index]); // copy payload value into msg buffe 
      publishStr.toCharArray(publish_buffer,publishStr.length()+1);
      client.publish(publish_buffer,msg);
}

void HC_publishValueMessage(iotdevice iot, int index){
      publishStr = iot.publishValueMessages[index];
      snprintf (msg, MSG_BUFFER_SIZE, "%f", iot.valueVar[index]); // copy payload value into msg buffer
      publishStr.toCharArray(publish_buffer,publishStr.length()+1);
      client.publish(publish_buffer,msg);
}

const int SWITCHCH = 0;
const int VALUECH = 1;

String HC_activateChannelbyMessage(iotdevice iot,int index, int chType, char* topic, unsigned int len, byte* payl){
  String retStr;

  iot.subscribeMessages[index].toCharArray(compare_buffer,iot.subscribeMessages[index].length()+1);  
  if (strcmp(topic, compare_buffer) == 0){
      for (unsigned int i=0;i < len;i++){
        c_val[i]=(char)(payl[i]);   
      }
      switch (chType){
        case 0: // switch channel, only 0 and 1, on and off
            iot.stateVar[index] = atoi(c_val);
            HC_setGPIO(iot, index, 1, iot.stateVar[index]);
            HC_publishStateMessage(iot,index);
        break;
        case 1: // value channel, float
            iot.valueVar[0] = (int)atof(c_val);
            HC_setPWMChannel(iot, 0);
            HC_publishValueMessage(iot,0);
            Serial.print("Value channel: "); Serial.print(iot.subscribeMessages[index]);
            Serial.print(" Value: "); Serial.println(iot.valueVar[index]);
        break;
        }
      return ("OK"); // vallid message 
  }
  return("NAM"); // not a valid message
}

void HC_subscribeAll(iotdevice iot){
  for (int i = 0; i < iot.numSubscribeMessages; i++){
    #ifdef DEBUG
      Serial.print("Subscribe to: "); Serial.println(iot.subscribeMessages[i]);
    #endif
      subscribeStr = iot.subscribeMessages[i];
      subscribeStr.toCharArray(subscribe_buffer,subscribeStr.length()+1);
      client.subscribe(subscribe_buffer);
  }
}

void HC_publishInitialState(iotdevice iot, PubSubClient pclient){

}

void HC_publishStatus(iotdevice iot){
  int i;
  for (i = 0; i < iot.numStatusMessages-1; i++){
    HC_publishStateMessage(iot, i);
  }
  for (i = 0; i < iot.numValueMessages-1; i++){
    HC_publishValueMessage(iot, i);
  }
}

#endif