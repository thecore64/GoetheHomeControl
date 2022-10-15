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
  String    chipID;
  String    systemID;
  String    type; //device type
  String    freeName;
  int       numSwitchChannels;
  int       numPWMChannels;
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

String checkSubscribeMessage(iotdevice iot,int index, char* topic, unsigned int len, byte* payl){
  iot.subscribeMessages[index].toCharArray(compare_buffer,iot.subscribeMessages[index].length()+1);  
  
  if (strcmp(topic, compare_buffer) == 0){
      for (unsigned int i=0;i < len;i++){
        c_val[i]=(char)(payl[i]);   
      }

      return (String(c_val));
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

void publishStateMessage(iotdevice iot, int index){
      publishStr = iot.publishStatusMessages[index];
      snprintf (msg, MSG_BUFFER_SIZE, "%d", iot.stateVar[index]); // copy payload value into msg buffe 
      publishStr.toCharArray(publish_buffer,publishStr.length()+1);
      client.publish(publish_buffer,msg);
}

void publishValueMessage(iotdevice iot, int index){
      publishStr = iot.publishValueMessages[index];
      snprintf (msg, MSG_BUFFER_SIZE, "%f", iot.valueVar[index]); // copy payload value into msg buffer
      publishStr.toCharArray(publish_buffer,publishStr.length()+1);
      client.publish(publish_buffer,msg);
}

void publishInitialState(iotdevice iot, PubSubClient pclient){

}

void HC_publishStatus(iotdevice iot){
  int i;
  for (i = 0; i < iot.numStatusMessages-1; i++){
    publishStateMessage(iot, i);
  }
  for (i = 0; i < iot.numValueMessages-1; i++){
    publishValueMessage(iot, i);
  }
}

#endif