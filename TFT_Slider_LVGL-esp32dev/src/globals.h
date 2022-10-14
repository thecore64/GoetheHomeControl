#ifndef globals_h_included
  #define globals_h_included

#include <Arduino.h>
#include <PubSubClient.h>

lv_obj_t * btnLabel;

WiFiClient espClient;
PubSubClient client(espClient);

int screenWidth = 320;
int screenHeight = 240;

int wall_brightness = 0;
int wall_bright25   = 100;
int wall_bright50   = 300;
int wall_bright100  = 800;

#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

char subscribe_buffer[MSG_BUFFER_SIZE+5];
char publish_buffer[MSG_BUFFER_SIZE+5];
char compare_buffer[MSG_BUFFER_SIZE+5];
String subscribe_str;
String publish_str;
String compare_str;
int mqtt_reconnectcnt = 0;

#endif  