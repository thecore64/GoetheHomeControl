#ifndef hc_frame_h_included
  #define hc_frame_h_included

#include <Arduino.h>

// -----------------------------------------------------------------------------------------
typedef struct {
  String    chipID;
  String    systemID;
  String    type;
  int       numSwitchChannels;
  int       numPWMChannels;
  int       numButtons;
  int       numSubscribeMessages;
  unsigned int  publishInterval;
  int       btnMsgperRelease[6];
  int       btnState[6];
  int       btnPressCnt[6];
  String    btnMessages[4][6];     // messages to be sent on btn. activation 
  String    publishMessages[10];   // general device messages to send, like RSSI, alive, etc.
  String    subscribeMessages[10]; // messages the device needs to receive
  IPAddress deviceIP;
} iotdevice;


#endif hc_frame_h_included  