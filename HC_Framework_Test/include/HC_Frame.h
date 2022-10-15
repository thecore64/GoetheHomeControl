#ifndef hc_frame_h_included
  #define hc_frame_h_included

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

String publishStr;
String subscribeStr;
String compareStr;

char c_val[6];    // buffer for dealing with MQTT messages payload
#define MSG_BUFFER_SIZE  (50)  
char msg[MSG_BUFFER_SIZE];  // general MQTT message buffer

char publish_buffer[MSG_BUFFER_SIZE+5];
char subscribe_buffer[MSG_BUFFER_SIZE+5];
char compare_buffer[MSG_BUFFER_SIZE+5];

// -----------------------------------------------------------------------------------------
typedef struct {
  String    chipID;
  String    systemID;
  String    type; //device type
  String    freeName;
  int       numSwitchChannels;
  int       stateVar[10]; // variable array for switch states
  float     valueVar[10]; // variable array for values, like brightness
  int       numPWMChannels;
  int       numButtons;
  int       numSubscribeMessages;
  unsigned int  publishInterval;
  int       btnMsgperRelease[6];
  int       btnState[6];
  int       btnPressCnt[6];
  String    btnMessages[4][6];     // messages to be sent on btn. activation 
  String    publishStatusMessages[10];   // device status messages to send
  String    publishValueMessages[10];   // device messages to send, for float values
  String    subscribeMessages[10]; // messages the device needs to receive
  IPAddress deviceIP;
} iotdevice;

void HC_subscribeAll(iotdevice iot, PubSubClient pclient){
  for (int i = 0; i < iot.numSubscribeMessages; i++){
    #ifdef DEBUG
      Serial.print("Subscribe to: "); Serial.println(iot.subscribeMessages[i]);
    #endif
      subscribeStr = iot.subscribeMessages[i];
      subscribeStr.toCharArray(subscribe_buffer,subscribeStr.length()+1);
      pclient.subscribe(subscribe_buffer);
  }
}

void publishStateMessage(iotdevice iot, PubSubClient pclient, int index){
      publishStr = iot.publishStatusMessages[index];
      snprintf (msg, MSG_BUFFER_SIZE, "%d", iot.stateVar[index]); // copy payload value into msg buffer
      publishStr.toCharArray(publish_buffer,publishStr.length()+1);
      publishStr = iot.publishStatusMessages[index];
      pclient.publish(publish_buffer,msg);
}

void publishValueMessage(iotdevice iot, PubSubClient pclient, int index){
      publishStr = iot.publishValueMessages[index];
      snprintf (msg, MSG_BUFFER_SIZE, "%d", iot.valueVar[index]); // copy payload value into msg buffer
      publishStr.toCharArray(publish_buffer,publishStr.length()+1);
      publishStr = iot.publishValueMessages[index];
      pclient.publish(publish_buffer,msg);
}


void HC_publishStatus(iotdevice iot, PubSubClient pclient){
   publishStateMessage(iot, pclient, 0);
   publishStateMessage(iot, pclient, 1);
}

#endif