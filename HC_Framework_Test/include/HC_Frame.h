#ifndef hc_frame_h_included
  #define hc_frame_h_included

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define SWITCHCH 0
#define VALUECH 1

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
  String name;
  String subscribeMessage;
  String StatusMessage;
  int ChannelType;    // for now: 0 = switch channel on or off, 1 = value channel, to transfer a float value
  int Direction;      // INPUT or OUTPUT
  int GPIO;           // HW pin number
  int isInverted;     // 1 means always channel on, so for low swiching channels
  int PWM;            // HW GPIO pin or PWM channel (ESP32) 
  int isPWMChannel;   // 1 if it is PWM, 0 if it is another value channel, e.d. DAC
  int PWMChannel;     // HW PMW generator channel, valid for ESP32 only
  int PWMFrequency;   // PWM frequency for PWM generator, valid for ESP32 only
  int PWMResolution;  // resolution for PWM generator, valid for ESP32 only
  int State;          // switch state 
  float Value;          // value of the channel, e.g. PWM value
} ch;
// -----------------------------------------------------------------------------------------
typedef struct {

  ch channels[4];
  int numChannels;

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


void initChannels(iotdevice iot){
/*
typedef struct {
  String name;
  String subscribeMessage;
  String StatusMessage;
  int ChannelType; // for now: 0 = switch channel on or off, 1 = value channel, to transfer a float value
  int Direction;   // INPUT or OUTPUT
  int GPIO;        // HW pin number
  int PWM;         // HW PWM pin or channel (ESP32) number
  int State;       // switch state 
  int Value;       // value of the channel, e.g. PWM value
} ch;
*/

  for (int i = 0; i < iot.numChannels; i++){
    // for ESP8266 no special init for a PWM channel is needed, just define it as output
    pinMode(iot.channels[i].GPIO, iot.channels[i].Direction);
    // for ESP32 we need a special init, select HW PWM channel and GPIO pin, frequency and duty
    #ifdef ESP32 // def from arduino enviroment
      if (iot.channels[i].isPWMChannel){
        // setup ESP32 PWM control
        ledcAttachPin(iot.channels[i].PWM , iot.channels[i].PWMHWChannel; // set HW PWM channel for each GPIO pin
        ledcSetup(iot.channels[i].PWMHWChannel, iot.channels[i].PWMFrequency, iot.channels[i].PWMResolution);
      }
    #endif

  }
}

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

//--------------------------------------------------------------------------------------------------------
void ChannelPublishMessage(iotdevice iot, int index){
    publishStr = iot.channels[index].StatusMessage;
    switch (iot.channels[index].ChannelType){
      case 0:
        snprintf (msg, MSG_BUFFER_SIZE, "%d", iot.channels[index].State); // copy payload value into msg buffer
      break;
      case 1:
        snprintf (msg, MSG_BUFFER_SIZE, "%f", iot.channels[index].Value); // copy payload value into msg buffer
      break;
    }
    publishStr.toCharArray(publish_buffer,publishStr.length()+1);
    client.publish(publish_buffer,msg);
  }
//--------------------------------------------------------------------------------------------------------
void ChannelSet(iotdevice iot, int index){
switch (iot.channels[index].ChannelType){
      case 0:
        if (iot.channels[index].isInverted){
          digitalWrite(iot.GPIO[index], !iot.channels[index].State);
        } else {
          digitalWrite(iot.GPIO[index], iot.channels[index].State);
        }  
      break;
      case 1:
        if (iot.channels[index].isPWMChannel == 1){
          #ifdef ESP32
            ledcWrite(iot.PWMChannel[index], iot.valueVar[index]);
          #endif
          analogWrite(iot.PWM[index], iot.valueVar[index]); //this will work for ESP8266 only !
        }
      break;
    }
}  
//--------------------------------------------------------------------------------------------------------
String new_HC_activateChannelbyMessage(iotdevice iot,int index, char* topic, unsigned int len, byte* payl){
  String retStr;

  iot.channels[index].subscribeMessage.toCharArray(compare_buffer,iot.channels[index].subscribeMessage.length()+1);  
  if (strcmp(topic, compare_buffer) == 0){
      for (unsigned int i=0;i < len;i++){
        c_val[i]=(char)(payl[i]);   
      }
      switch (iot.channels[index].ChannelType){
        case 0: // switch channel, only 0 and 1, on and off
            iot.channels[index].State= atoi(c_val);
            ChannelSet(iot, index);
            ChannelPublishMessage(iot, index);
        break;
        case 1: // value channel, float
            iot.channels[index].Value = (int)atof(c_val);
            ChannelSet(iot, index);
            ChannelPublishMessage(iot, index);
            Serial.print("Value channel: "); Serial.print(iot.subscribeMessages[index]);
            Serial.print(" Value: "); Serial.println(iot.valueVar[index]);
        break;
        }
      return ("OK"); // vallid message 
  }
  return("NAM"); // not a valid message
}
//------------------------------------------------------------------------------------------------------------------

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