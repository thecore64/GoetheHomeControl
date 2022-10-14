#ifndef globals_h_included
  #define globals_h_included

#include <Arduino.h>
#include <PubSubClient.h>

lv_obj_t * btnLabel;
lv_obj_t * winWall;
lv_obj_t * sliderleft;
lv_obj_t * sliderright;
lv_obj_t * offButton, * hallwayoff;
lv_obj_t *hallway1, *hallway2;
lv_obj_t *hallLED1, *hallLED2;
lv_obj_t *lab1, *lab2, *lab3;
lv_obj_t *labLED1, *labLED2, *labLED3;

int hallway1State, hallway2State = 0;
int office1State, office2State, office3State = 0;

WiFiClient espClient;
PubSubClient client(espClient);

int screenWidth = 240;
int screenHeight = 320;

int activeWindow = 0;

int time_cnt = 0;
int bl_switch_cnt = 0;
int bl_off = 0;
int tft_dim_range[6] = {600,700,800,900,990,1024};
int tft_dim_index = 0;

int brightnessleft = 0;
int brightnessright = 0;

char c_val[6];    // buffer for MQTT messages payload
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