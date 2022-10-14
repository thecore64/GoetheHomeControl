#include <Arduino.h>

#include <lvgl.h>
#include <TFT_eSPI.h>

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <time.h>
#include "NPT_time.h"
#include <ArduinoOTA.h>
#include <ota.h>
#include <SparkFun_MicroMod_Button.h>
#include <Wire.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <globals.h>
#include <stdio.h>
#include <FastLED.h>

#include <stdio.h>

#define _TIMERINTERRUPT_LOGLEVEL_     4
#include "ESP32TimerInterrupt.h"
#define TIMER1_INTERVAL_MS        1000
// Init ESP32 timer 1
ESP32Timer ITimer1(1);

#define LVGL_TICK_PERIOD 60

lv_obj_t * menuItem[6];
lv_obj_t * prev;
lv_obj_t * txt;
lv_obj_t * txt2;
lv_obj_t * offlabel;
lv_obj_t * sliderHallway;
lv_obj_t * mainTempLabel, * mainTempValue;
lv_obj_t * tempLabel, * tempValue;
lv_obj_t * tempminLabel, * tempminValue;
lv_obj_t * tempmaxLabel, * tempmaxValue;
lv_obj_t * humLabel, * humValue;
lv_obj_t * humminLabel, * humminValue;
lv_obj_t * hummaxLabel, * hummaxValue;


char thBuffer[30];

float temp,hum,mintemp,maxtemp,minhum,maxhum = 0;

int btn_cnt = 0;
int activeSlider = 0; // 0 = left Slider
int btnDownCnt = 0;
int hallwayBrightness = 0;
int numLEDhallway = 0;
int redval, greenval = 0;
int selfSend = 0; // this variable is set to 1 when the software is sending MQTT messages to prevent
                  // that values, and therefore sliders and controls, got updated immediately by the 
                  // MQTT receive callback routine. After sending is finished is will be set back to 0.
                  // Updating controls in the different screens will happen only when selfSend = 0 
                  // because the related values are only updated by the callback at this state !!
String IPstr;
char buffer[20];
// ------- FastLED definitions --------------------------------------------------------
#define NUM_LEDS 6

// APA102 LED's need Clock and Data pins !!
#define DATA_PIN 25
#define CLOCK_PIN 15

// array of leds, one item for each led in the strip.
CRGB leds[NUM_LEDS];

// ------- network credentials --------------------------------------------------------
const char* ssid = "TheCore";
const char* password = "M1n0tauru5";
const char* mqtt_server = "192.168.0.5";
// ------------------------------------------------------------------------------------

lv_obj_t * scr;
lv_obj_t * scr2;
lv_obj_t * hallway_scr;
lv_obj_t * weather_scr;
lv_obj_t * lab_scr;

MicroModButton button;

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];

// -------------------------------------------------------------------------
bool IRAM_ATTR TimerHandler1(void * timerNo)
{ 
  bl_switch_cnt ++;
  time_cnt ++;
  return true;
}
// -------------------------------------------------------------------------

void drawLab(){
  txt = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(txt, "OFFICELAB");
  lv_obj_align(txt, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 270);
  lv_obj_set_style_local_text_font(txt, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_26);
  
  static lv_style_t lblstyle;
  lv_style_init(&lblstyle);
  lv_style_set_bg_color(&lblstyle,LV_STATE_DEFAULT,LV_COLOR_GREEN);
  lv_style_set_bg_opa(&lblstyle, LV_STATE_DEFAULT, LV_OPA_COVER);
  lv_style_set_bg_grad_color(&lblstyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_bg_grad_dir(&lblstyle, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  lv_style_set_text_font(&lblstyle, LV_STATE_DEFAULT, &lv_font_montserrat_20);

  static lv_style_t style;
  // Make a gradient
  lv_style_set_bg_opa(&style, LV_STATE_DEFAULT, LV_OPA_COVER);
  lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_GREEN);
  lv_style_set_bg_grad_color(&style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_bg_grad_dir(&style, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  //*Shift the gradient to the bottom
  lv_style_set_bg_main_stop(&style, LV_STATE_DEFAULT, 43);
  lv_style_set_bg_grad_stop(&style, LV_STATE_DEFAULT, 212);

  lv_obj_add_style(lab_scr, LV_STATE_DEFAULT, &style);

  // create buttons
  lab1 = lv_btn_create(lv_scr_act(), NULL);
  //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
  lv_obj_set_size(lab1, 155,40);
  //lv_obj_set_width(btn,50);
  lv_obj_align(lab1, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 25);
  //lv_btn_set_checkable(btn, true);
  //lv_btn_toggle(btn2);
  //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
  lv_obj_set_style_local_bg_color(lab1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
  offlabel = lv_label_create(lab1, NULL);
  lv_obj_set_style_local_text_font(offlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
  lv_label_set_text(offlabel, "Office Desk"); 
  
  labLED1 = lv_btn_create(lv_scr_act(), NULL);
  //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
  lv_obj_set_size(labLED1, 40,40);
  //lv_obj_set_width(btn,50);
  lv_obj_align(labLED1, NULL, LV_ALIGN_IN_TOP_LEFT, 160, 25);
  //lv_btn_set_checkable(btn, true);
  //lv_btn_toggle(btn2);
  //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
  if (office1State == 0){
    lv_obj_set_style_local_bg_color(labLED1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
  } else {
    lv_obj_set_style_local_bg_color(labLED1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_MAGENTA);
  }
// create buttons
  lab2 = lv_btn_create(lv_scr_act(), NULL);
  //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
  lv_obj_set_size(lab2, 155,40);
  //lv_obj_set_width(btn,50);
  lv_obj_align(lab2, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 85);
  //lv_btn_set_checkable(btn, true);
  //lv_btn_toggle(btn2);
  //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
  offlabel = lv_label_create(lab2, NULL);
  lv_obj_set_style_local_text_font(offlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
  lv_label_set_text(offlabel, "Lab Shelve"); 
  lv_obj_set_style_local_bg_color(lab2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_BLUE);

  labLED2 = lv_btn_create(lv_scr_act(), NULL);
  //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
  lv_obj_set_size(labLED2, 40,40);
  //lv_obj_set_width(btn,50);
  lv_obj_align(labLED2, NULL, LV_ALIGN_IN_TOP_LEFT, 160, 85);
  //lv_btn_set_checkable(btn, true);
  //lv_btn_toggle(btn2);
  //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
  if (office2State == 0){
    lv_obj_set_style_local_bg_color(labLED2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
  } else {
    lv_obj_set_style_local_bg_color(labLED2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_MAGENTA);
  }
  
// create buttons
  lab3 = lv_btn_create(lv_scr_act(), NULL);
  //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
  lv_obj_set_size(lab3, 155,40);
  //lv_obj_set_width(btn,50);
  lv_obj_align(lab3, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 145);
  //lv_btn_set_checkable(btn, true);
  //lv_btn_toggle(btn2);
  //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
  offlabel = lv_label_create(lab3, NULL);
  lv_obj_set_style_local_text_font(offlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
  lv_label_set_text(offlabel, "Shelve Desk"); 
  lv_obj_set_style_local_bg_color(lab3,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_BLUE);

  labLED3 = lv_btn_create(lv_scr_act(), NULL);
  //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
  lv_obj_set_size(labLED3, 40,40);
  //lv_obj_set_width(btn,50);
  lv_obj_align(labLED3, NULL, LV_ALIGN_IN_TOP_LEFT, 160, 145);
  //lv_btn_set_checkable(btn, true);
  //lv_btn_toggle(btn2);
  //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
  if (office3State == 0){
    lv_obj_set_style_local_bg_color(labLED3,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
  } else {
    lv_obj_set_style_local_bg_color(labLED3,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_MAGENTA);
  }

  

  btnDownCnt = 0;
  activeWindow = 5;
}

void drawWeatherman(){
  txt = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(txt, "Weatherman");
  lv_obj_align(txt, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 270);
  lv_obj_set_style_local_text_font(txt, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_26);

// lv_label_set_text_fmt(label, "Value: %d", 15)
//---------- temperature ----------------------------------------------------------

  static lv_style_t lblstyle;
  lv_style_init(&lblstyle);
  lv_style_set_bg_color(&lblstyle,LV_STATE_DEFAULT,LV_COLOR_CYAN);
  lv_style_set_bg_opa(&lblstyle, LV_STATE_DEFAULT, LV_OPA_COVER);
  lv_style_set_bg_grad_color(&lblstyle, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_bg_grad_dir(&lblstyle, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  lv_style_set_text_font(&lblstyle, LV_STATE_DEFAULT, &lv_font_montserrat_20);
  
  tempLabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(tempLabel, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  lv_label_set_text(tempLabel, "#00FFFF Temperature: #");
  lv_obj_align(tempLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 30);
  lv_obj_set_style_local_text_font(tempLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);

  tempValue = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(tempValue, true);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#00FFFF %.1f#", temp);
  lv_label_set_text(tempValue, thBuffer);
  lv_obj_align(tempValue, NULL, LV_ALIGN_IN_TOP_LEFT, 155, 25);
  lv_obj_set_style_local_text_font(tempValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_26);

  tempminLabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(tempminLabel, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  lv_label_set_text(tempminLabel, "#00FFFF min. Temperature: #");
  lv_obj_align(tempminLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 60);
  lv_obj_set_style_local_text_font(tempminLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

  tempminValue = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(tempminValue, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#00FFFF %.1f #", mintemp);
  lv_label_set_text(tempminValue, thBuffer);
  lv_obj_align(tempminValue, NULL, LV_ALIGN_IN_TOP_LEFT, 175, 60);
  lv_obj_set_style_local_text_font(tempminValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

  tempmaxLabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(tempmaxLabel, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  lv_label_set_text(tempmaxLabel, "#00FFFF max. Temperature: #");
  lv_obj_align(tempmaxLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 90);
  lv_obj_set_style_local_text_font(tempmaxLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

  tempmaxValue = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(tempmaxValue, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#00FFFF %.1f #", maxtemp);
  lv_label_set_text(tempmaxValue, thBuffer);
  lv_obj_align(tempmaxValue, NULL, LV_ALIGN_IN_TOP_LEFT, 175, 90);
  lv_obj_set_style_local_text_font(tempmaxValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

//---------- humidity ----------------------------------------------------------
  humLabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(humLabel, true);
  lv_label_set_text(humLabel, "#FFFF00 rel.Humidity: #");
  lv_obj_align(humLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 120);
  lv_obj_set_style_local_text_font(humLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
  
  humValue = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(humValue, true);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#FFFF00 %.1f #", hum);
  lv_label_set_text(humValue, thBuffer);
  lv_obj_align(humValue, NULL, LV_ALIGN_IN_TOP_LEFT, 155, 115);
  lv_obj_set_style_local_text_font(humValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_26);

  humminLabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(humminLabel, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  lv_label_set_text(humminLabel, "#FFFF00 min. Humidity: #");
  lv_obj_align(humminLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 150);
  lv_obj_set_style_local_text_font(humminLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

  humminValue = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(humminValue, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#FFFF00 %.1f #", minhum);
  lv_label_set_text(humminValue, thBuffer);
  lv_obj_align(humminValue, NULL, LV_ALIGN_IN_TOP_LEFT, 175, 150);
  lv_obj_set_style_local_text_font(humminValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

  hummaxLabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(hummaxLabel, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  lv_label_set_text(hummaxLabel, "#FFFF00 max. Humidity: #");
  lv_obj_align(hummaxLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 180);
  lv_obj_set_style_local_text_font(hummaxLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

  hummaxValue = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(hummaxValue, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#FFFF00 %.1f #", maxhum);
  lv_label_set_text(hummaxValue, thBuffer);
  lv_obj_align(hummaxValue, NULL, LV_ALIGN_IN_TOP_LEFT, 175, 180);
  lv_obj_set_style_local_text_font(hummaxValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

  activeWindow = 6;
}

void drawHallway(){
  txt = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(txt, "Hallway");
  lv_obj_align(txt, NULL, LV_ALIGN_IN_TOP_LEFT, 60, 270);
  lv_obj_set_style_local_text_font(txt, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_26);

  // create buttons
  hallway1 = lv_btn_create(lv_scr_act(), NULL);
  //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
  lv_obj_set_size(hallway1, 155,40);
  //lv_obj_set_width(btn,50);
  lv_obj_align(hallway1, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 25);
  //lv_btn_set_checkable(btn, true);
  //lv_btn_toggle(btn2);
  //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
  offlabel = lv_label_create(hallway1, NULL);
  lv_obj_set_style_local_text_font(offlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
  lv_label_set_text(offlabel, "Hallway L");
  lv_obj_set_style_local_bg_color(hallway1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);

  hallLED1 = lv_btn_create(lv_scr_act(), NULL);
  //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
  lv_obj_set_size(hallLED1, 40,40);
  //lv_obj_set_width(btn,50);
  lv_obj_align(hallLED1, NULL, LV_ALIGN_IN_TOP_LEFT, 160, 25);
  //lv_btn_set_checkable(btn, true);
  //lv_btn_toggle(btn2);
  //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
  lv_obj_set_style_local_bg_color(hallLED1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);

  hallway2 = lv_btn_create(lv_scr_act(), NULL);
  //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
  lv_obj_set_size(hallway2, 155,40);
  //lv_obj_set_width(btn,50);
  lv_obj_align(hallway2, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 85);
  //lv_btn_set_checkable(btn, true);
  //lv_btn_toggle(btn2);
  //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
  offlabel = lv_label_create(hallway2, NULL);
  lv_obj_set_style_local_text_font(offlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
  lv_label_set_text(offlabel, "Hallway R"); 
  lv_obj_set_style_local_bg_color(hallway2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_BLUE);

  hallLED2 = lv_btn_create(lv_scr_act(), NULL);
  //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
  lv_obj_set_size(hallLED2, 40,40);
  //lv_obj_set_width(btn,50);
  lv_obj_align(hallLED2, NULL, LV_ALIGN_IN_TOP_LEFT, 160, 85);
  //lv_btn_set_checkable(btn, true);
  //lv_btn_toggle(btn2);
  //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
  lv_obj_set_style_local_bg_color(hallLED2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);

  sliderHallway = lv_slider_create(lv_scr_act(), NULL);
  // first parameter of this function determines which screen or window the control belongs to,
  // if it is the wrong one, the object will not be destroyed by deleting the window
  //lv_obj_set_event_cb(sliderleft, slider_event_cb);         //Assign an event function
  lv_obj_set_width(sliderHallway, screenWidth-30);                        /*Set the width*/
  lv_obj_set_height(sliderHallway, 30);
  lv_slider_set_range(sliderHallway, 0, 700);
  lv_slider_set_value(sliderHallway, hallwayBrightness, LV_ANIM_ON);
  lv_obj_align(sliderHallway, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 150);    /*Align to the center of the parent (screen)*/
  // LV_SLIDER_PART_xx determines which part of the slider we want to change color
  // BG background color, KNOB of course, 
  // INDIC is the part which gets filled when moving
  lv_obj_set_style_local_bg_color(sliderHallway,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
  lv_obj_set_style_local_bg_color(sliderHallway,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
  lv_obj_set_style_local_bg_color(sliderHallway, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_CYAN);
  

  hallwayoff = lv_btn_create(lv_scr_act(), NULL);
  //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
  lv_obj_set_size(hallwayoff, 155,40);
  //lv_obj_set_width(btn,50);
  lv_obj_align(hallwayoff, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 205);
  //lv_btn_set_checkable(btn, true);
  //lv_btn_toggle(btn2);
  //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
  offlabel = lv_label_create(hallwayoff, NULL);
  lv_obj_set_style_local_text_font(offlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
  lv_label_set_text(offlabel, "all off"); 
  lv_obj_set_style_local_bg_color(hallwayoff,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_BLUE);

  activeWindow = 3;
}
// -------------------------------------------------------------------------

void drawOaklandBay(){
  txt = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(txt, "Oakland Bay");
  lv_obj_align(txt, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 270);
  lv_obj_set_style_local_text_font(txt, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_26);
  
  txt = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(txt, true);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#FFFFFF LED STRIP LINKS #");
  lv_label_set_text(txt, thBuffer);
  //lv_label_set_text(txt, "Links");
  lv_obj_align(txt, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 7);

  txt2 = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(txt2, true);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#FFFFFF LED STRIP RECHTS #");
  lv_label_set_text(txt2, thBuffer);
  //lv_label_set_text(txt2, "Rechts");
  lv_obj_align(txt2, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 90);

  sliderleft = lv_slider_create(lv_scr_act(), NULL);
  // first parameter of this function determines which screen or window the control belongs to,
  // if it is the wrong one, the object will not be destroyed by deleting the window
  //lv_obj_set_event_cb(sliderleft, slider_event_cb);         //Assign an event function
  lv_obj_set_width(sliderleft, screenWidth-30);                        /*Set the width*/
  lv_obj_set_height(sliderleft, 30);
  lv_slider_set_range(sliderleft, 0, 1023);
  lv_slider_set_value(sliderleft, brightnessleft, LV_ANIM_ON);
  lv_obj_align(sliderleft, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 40);    /*Align to the center of the parent (screen)*/
  // LV_SLIDER_PART_xx determines which part of the slider we want to change color
  // BG background color, KNOB of course, 
  // INDIC is the part which gets filled when moving
  lv_obj_set_style_local_bg_color(sliderleft,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
  lv_obj_set_style_local_bg_color(sliderleft,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
  lv_obj_set_style_local_bg_color(sliderleft, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_CYAN);

  sliderright = lv_slider_create(lv_scr_act(), NULL);
  //lv_obj_set_event_cb(sliderright, slider2_event_cb);         //Assign an event function
  // first parameter of this function determines which screen or window the control belongs to,
  // if it is the wrong one, the object will not be destroyed by deleting the window
  lv_obj_set_width(sliderright, screenWidth-30);                        //*Set the width
  lv_obj_set_height(sliderright, 30);
  lv_slider_set_range(sliderright, 0, 1023);
  lv_slider_set_value(sliderright, brightnessright, LV_ANIM_ON);
  lv_obj_align(sliderright, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 120);    //*Align to the center of the parent (screen)
  // LV_SLIDER_PART_xx determines which part of the slider we want to change color
  // BG background color, KNOB of course, 
  // INDIC is the part which gets filled when moving
  lv_obj_set_style_local_bg_color(sliderright,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
  lv_obj_set_style_local_bg_color(sliderright,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
  lv_obj_set_style_local_bg_color(sliderright, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_CYAN);

  // create the off button
  offButton = lv_btn_create(lv_scr_act(), NULL);
  lv_obj_set_size(offButton, 195,40);
  lv_obj_align(offButton, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 215);
  offlabel = lv_label_create(offButton, NULL);
  lv_obj_set_style_local_text_font(offlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
  //lv_label_set_text(offlabel, "all Off"); 
  lv_label_set_recolor(offlabel, true);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#FFFFFF all off #");
  lv_label_set_text(offlabel, thBuffer);
  lv_obj_set_style_local_bg_color(offButton,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_MAGENTA);
     
  activeSlider = 0;
  lv_obj_set_style_local_bg_color(sliderleft,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
  lv_obj_set_style_local_bg_color(sliderleft,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
  lv_obj_set_style_local_bg_color(sliderleft, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_RED);
}
// -------------------------------------------------------------------------

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint16_t c;

  tft.startWrite(); /* Start new TFT transaction */
  tft.setAddrWindow(area->x1, area->y1, (area->x2 - area->x1 + 1), (area->y2 - area->y1 + 1)); /* set the working window */
  for (int y = area->y1; y <= area->y2; y++) {
    for (int x = area->x1; x <= area->x2; x++) {
      c = color_p->full;
      tft.writeColor(c, 1);
      color_p++;
    }
  }
  tft.endWrite(); /* terminate TFT transaction */
  lv_disp_flush_ready(disp); /* tell lvgl that flushing is done */
}

void callback(char* topic, byte* payload, unsigned int length) {
  for (unsigned int i=0;i<6;i++) c_val[i]=0; 

  compare_str="SNSTEMHUMESP32-S2/Temperature";
  compare_str.toCharArray(compare_buffer,compare_str.length()+1);
  if (strcmp(topic, compare_buffer) == 0){
    //Serial.print("received message: "); Serial.print(topic); Serial.println();
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }
    temp = atof(c_val);
    if (activeWindow == 6){
      snprintf(thBuffer, MSG_BUFFER_SIZE, "#00FFFF %.1f°C#", temp);
      lv_label_set_text(tempValue, thBuffer);
    }  
    if (activeWindow == 0){
      snprintf(thBuffer, MSG_BUFFER_SIZE, "#00FFFF %.1f°C#", temp);
      lv_label_set_text(mainTempLabel, thBuffer);
    }
  }  
  compare_str="SNSTEMHUMESP32-S2/minTemperature";
  compare_str.toCharArray(compare_buffer,compare_str.length()+1);
  if (strcmp(topic, compare_buffer) == 0){
    //Serial.print("received message: "); Serial.print(topic); Serial.println();
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }
    mintemp = atof(c_val);
    if (activeWindow == 6){
      snprintf(thBuffer, MSG_BUFFER_SIZE, "#00FFFF %.1f °C#", mintemp);
      lv_label_set_text(tempminValue, thBuffer);
    }  
  }
  compare_str="SNSTEMHUMESP32-S2/maxTemperature";
  compare_str.toCharArray(compare_buffer,compare_str.length()+1);
  if (strcmp(topic, compare_buffer) == 0){
    //Serial.print("received message: "); Serial.print(topic); Serial.println();
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }
    maxtemp = atof(c_val);
    if (activeWindow == 6){
      snprintf(thBuffer, MSG_BUFFER_SIZE, "#00FFFF %.1f °C#", maxtemp);
      lv_label_set_text(tempmaxValue, thBuffer);
    }  
  }

  compare_str="SNSTEMHUMESP32-S2/Humidity";
  compare_str.toCharArray(compare_buffer,compare_str.length()+1);
  if (strcmp(topic, compare_buffer) == 0){
    //Serial.print("received message: "); Serial.print(topic); Serial.println();
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }
    //Serial.print("Payload: "); Serial.print(c_val);
    hum = atof(c_val);
    //Serial.print(" Humidity: "); Serial.print(hum);
    if (activeWindow == 6){
      snprintf(thBuffer, MSG_BUFFER_SIZE, "#FFFF00 %.1f %%#", hum);
      lv_label_set_text(humValue, thBuffer);
    }  
  }
  compare_str="SNSTEMHUMESP32-S2/minHumidity";
  compare_str.toCharArray(compare_buffer,compare_str.length()+1);
  if (strcmp(topic, compare_buffer) == 0){
    //Serial.print("received message: "); Serial.print(topic); Serial.println();
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }
    minhum = atof(c_val);
    if (activeWindow == 6){
      snprintf(thBuffer, MSG_BUFFER_SIZE, "#FFFF00 %.1f %%#", minhum);
      lv_label_set_text(humminValue, thBuffer);
    }  
  }
  compare_str="SNSTEMHUMESP32-S2/maxHumidity";
  compare_str.toCharArray(compare_buffer,compare_str.length()+1);
  if (strcmp(topic, compare_buffer) == 0){
    //Serial.print("received message: "); Serial.print(topic); Serial.println();
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }
    maxhum = atof(c_val);
    if (activeWindow == 6){
      snprintf(thBuffer, MSG_BUFFER_SIZE, "#FFFF00 %.1f %%#", maxhum);
      lv_label_set_text(hummaxValue, thBuffer);
    }  
  }

  if (selfSend == 0){
  // receive all Brightness messages and change controlls accordingly
  // ,we need to know the value, in case it is changed from another controller
  compare_str="DIM12967876/Brightness1";
  compare_str.toCharArray(compare_buffer,compare_str.length()+1);
  if (strcmp(topic, compare_buffer) == 0){
    Serial.print("received message: "); Serial.print(topic); Serial.println();
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }
    brightnessleft = atoi(c_val);
    if (activeWindow == 1){
      lv_slider_set_value(sliderleft, brightnessleft, LV_ANIM_ON);
    }
  }  
  compare_str="DIM12967876/Brightness2";
  compare_str.toCharArray(compare_buffer,compare_str.length()+1);
  if (strcmp(topic, compare_buffer) == 0){
    Serial.print("received message: "); Serial.print(topic); Serial.println();
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }
    brightnessright = atoi(c_val);
    if (activeWindow == 1){
      lv_slider_set_value(sliderright, brightnessright, LV_ANIM_ON);
    }
  }  
  }
  // ------ send IP request message -----------------------------------------------------------
  /*
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
  }     */
}


void reconnect() {
  // Loop until we're reconnected
  tft.setCursor(12,55);
  tft.print("connecting MQTT server...");
  tft.setCursor(12,65);

  while (!client.connected()) {

#ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
#endif

    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      subscribe_str = "Switch2267261/State";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);

      subscribe_str = "Switch8745923/State1";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);

      subscribe_str = "Switch8745923/State2";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);
    
      subscribe_str = "DIM12967876/Brightness1";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);

      subscribe_str = "DIM12967876/Brightness2";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);

      snprintf (msg, MSG_BUFFER_SIZE, "%d", 1); // copy payload value into msg buffer
      client.publish("DIM12967876/GetState",msg); // request actual status from device

      subscribe_str = "SNSTEMHUMESP32-S2/Temperature";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);
      subscribe_str = "SNSTEMHUMESP32-S2/Humidity";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);
      subscribe_str = "SNSTEMHUMESP32-S2/minTemperature";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);
      subscribe_str = "SNSTEMHUMESP32-S2/maxTemperature";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);
      subscribe_str = "SNSTEMHUMESP32-S2/minHumidity";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);
      subscribe_str = "SNSTEMHUMESP32-S2/maxHumidity";
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
  tft.setCursor(12,65);
  tft.print("MQTT connection successful !");
  delay(2000);
}

void setupWiFi() {

  delay(10);
 
  tft.fillScreen(TFT_GOLD);
  tft.setCursor(12,25);
  tft.print("connecting to: "); tft.print(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  randomSeed(micros());
  //tft.fillScreen(TFT_GOLD);
  tft.setTextColor(TFT_BLACK);
  tft.setCursor(12,35);
  tft.print("WiFi connection successful !");
  tft.setCursor(12,45);
  tft.print(WiFi.localIP());
  delay(1000);
}

void setup() {

  FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);  // BGR ordering is typical
  FastLED.setBrightness(5);
  for(int Led = 0; Led < NUM_LEDS; Led = Led + 1) {
      // Turn our current led on to green, then show the leds
      leds[Led] = CRGB::Green;

      // Show the leds (only one of which is set to white, from above)
      FastLED.show();

      // Wait a little bit
      delay(100);

      // Turn our current led back to black for the next loop around
      leds[Led] = CRGB::Black;
  }
  leds[0] = CRGB::Green;

  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT);

  ledcSetup(0, 5000, 10);
  ledcAttachPin(TFT_BL, 0); 
  ledcWrite(0,500); // set brightness backlight

  //digitalWrite(TFT_BL, LOW);
  
  //pinMode(17, OUTPUT);

  if (ITimer1.attachInterruptInterval(TIMER1_INTERVAL_MS*10, (void (*)())TimerHandler1))
    {
      Serial.print(F("Starting  ITimer1 OK, millis() = ")); Serial.println(millis());
      //tft.print("starting timer ... OK");
    } else {
      Serial.println(F("Can't set ITimer1 !"));
      //tft.print("starting timer ... failed");
    }

  Wire.begin();
  button.begin();

  lv_init(); // initialize LVGL graphics library

  tft.begin(); /* TFT init */
  tft.setRotation(2);
  
  setupWiFi();
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //uint16_t calData[5] = { 275, 3620, 264, 3532, 1 };
  //tft.setTouch(calData);

  lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);

  /*Initialize the display*/
  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);

  //Set the theme..
  lv_theme_t * th = lv_theme_material_init(LV_THEME_DEFAULT_COLOR_PRIMARY , LV_THEME_DEFAULT_COLOR_SECONDARY, LV_THEME_DEFAULT_FLAG, LV_THEME_DEFAULT_FONT_SMALL , LV_THEME_DEFAULT_FONT_NORMAL, LV_THEME_DEFAULT_FONT_SUBTITLE, LV_THEME_DEFAULT_FONT_TITLE);     
  lv_theme_set_act(th);

    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_radius(&style, LV_STATE_DEFAULT, 5);

    // Make a gradient
    lv_style_set_bg_opa(&style, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_CYAN);
    lv_style_set_bg_grad_color(&style, LV_STATE_DEFAULT, LV_COLOR_GREEN);
    lv_style_set_bg_grad_dir(&style, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);

    //*Shift the gradient to the bottom
    lv_style_set_bg_main_stop(&style, LV_STATE_DEFAULT, 43);
    lv_style_set_bg_grad_stop(&style, LV_STATE_DEFAULT, 212);

    scr = lv_cont_create(NULL, NULL);
    
// *************************************************
// *************************************************

     lv_obj_add_style(scr, LV_STATE_DEFAULT, &style);

    lv_disp_load_scr(scr);

// create label to show IP adress ---------------------------------------------------------------
  lv_obj_t *labelIP = lv_label_create(lv_scr_act(), NULL);
  IPstr = WiFi.localIP().toString();
  IPstr.toCharArray(buffer,IPstr.length()+1);
  lv_obj_set_style_local_text_font(labelIP, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_14);
  lv_obj_set_style_local_text_color(labelIP, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
  lv_label_set_text(labelIP, buffer);
  lv_obj_align(labelIP, NULL, LV_ALIGN_IN_TOP_LEFT, 28, 304);  
  
  // create label to show temperature on main screen
  mainTempLabel = lv_label_create(lv_scr_act(), NULL);
  lv_obj_set_style_local_text_font(mainTempLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);
  lv_obj_set_style_local_text_color(mainTempLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
  lv_label_set_recolor(mainTempLabel, true);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#00FFFF %.1f°C#", temp);
  lv_label_set_text(mainTempLabel, thBuffer);
  lv_obj_align(mainTempLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 118, 302);  

  // create label to hold WiFi icon (fontawesome) ------------------------------------------------
  lv_obj_t * wifilabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(wifilabel, LV_SYMBOL_WIFI);
  lv_obj_set_style_local_text_color(wifilabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
  lv_obj_align(wifilabel, NULL, LV_ALIGN_IN_TOP_LEFT, 5, 299);  
  //----------------------------------------------------------------------------------------------- 
  // Create top label
  lv_obj_t * label = lv_label_create(lv_scr_act(), NULL);
  // set text color
  lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
  // set opacity for the background color
  lv_obj_set_style_local_bg_opa(label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,LV_OPA_COVER);
  // set background color
  lv_obj_set_style_local_bg_color(label,LV_LABEL_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_CYAN);
  
  lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_26);
  lv_label_set_text(label, "    Crystal Cave    ");
  lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 0);


    menuItem[0] = lv_btn_create(lv_scr_act(), NULL);
    //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
    lv_obj_set_size(menuItem[0], 195,40);
    //lv_obj_set_width(btn,50);
    lv_obj_align(menuItem[0], NULL, LV_ALIGN_IN_TOP_LEFT, 20, 33);
    //lv_btn_set_checkable(btn, true);
    lv_btn_toggle(menuItem[0]);
    //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
    lv_obj_t * btnlabel = lv_label_create(menuItem[0], NULL);
    lv_obj_set_style_local_text_font(btnlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
    lv_label_set_text(btnlabel, "OAKLAND BAY");

    menuItem[1]= lv_btn_create(lv_scr_act(), NULL);
    //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
    lv_obj_set_size(menuItem[1], 195,40);
    //lv_obj_set_width(btn,50);
    lv_obj_align(menuItem[1], NULL, LV_ALIGN_IN_TOP_LEFT, 20, 75);
    //lv_btn_set_checkable(btn, true);
    //lv_btn_toggle(btn2);
    //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
    lv_obj_t * btnlabel2 = lv_label_create(menuItem[1], NULL);
    lv_obj_set_style_local_text_font(btnlabel2, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
    lv_label_set_text(btnlabel2, "TOKIO STREET");

    menuItem[2] = lv_btn_create(lv_scr_act(), NULL);
    //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
    lv_obj_set_size(menuItem[2], 195,40);
    //lv_obj_set_width(btn,50);
    lv_obj_align(menuItem[2], NULL, LV_ALIGN_IN_TOP_LEFT, 20, 117);
    //lv_btn_set_checkable(btn, true);
    //lv_btn_toggle(btn2);
    //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
    lv_obj_t * btnlabel3 = lv_label_create(menuItem[2], NULL);
    lv_obj_set_style_local_text_font(btnlabel3, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
    lv_label_set_text(btnlabel3, "HALLWAY");    

    menuItem[3] = lv_btn_create(lv_scr_act(), NULL);
    //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
    lv_obj_set_size(menuItem[3], 195,40);
    //lv_obj_set_width(btn,50);
    lv_obj_align(menuItem[3], NULL, LV_ALIGN_IN_TOP_LEFT, 20, 159);
    //lv_btn_set_checkable(btn, true);
    //lv_btn_toggle(btn2);
    //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
    lv_obj_t * btnlabel4 = lv_label_create(menuItem[3], NULL);
    lv_obj_set_style_local_text_font(btnlabel4, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
    lv_label_set_text(btnlabel4, "-------");    

    menuItem[4] = lv_btn_create(lv_scr_act(), NULL);
    //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
    lv_obj_set_size(menuItem[4], 195,40);
    //lv_obj_set_width(btn,50);
    lv_obj_align(menuItem[4], NULL, LV_ALIGN_IN_TOP_LEFT, 20, 202);
    //lv_btn_set_checkable(btn, true);
    //lv_btn_toggle(btn2);
    //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
    lv_obj_t * btnlabel5 = lv_label_create(menuItem[4], NULL);
    lv_obj_set_style_local_text_font(btnlabel5, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
    lv_label_set_text(btnlabel5, "OFFICELAB"); 

    menuItem[5] = lv_btn_create(lv_scr_act(), NULL);
    //lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
    lv_obj_set_size(menuItem[5], 195,40);
    //lv_obj_set_width(btn,50);
    lv_obj_align(menuItem[5], NULL, LV_ALIGN_IN_TOP_LEFT, 20, 245);
    //lv_btn_set_checkable(btn, true);
    //lv_btn_toggle(btn2);
    //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
    lv_obj_t * btnlabel6 = lv_label_create(menuItem[5], NULL);
    lv_obj_set_style_local_text_font(btnlabel6, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
    lv_label_set_text(btnlabel6, "WEATHERMAN");     
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  lv_task_handler(); /* let the GUI do its work */

  //Serial.print("Switch:"); Serial.println(bl_switch_cnt);
  
  delay(5);

  if (bl_switch_cnt >= 4000){
    for(int Led = 0; Led < NUM_LEDS; Led = Led + 1) {
        leds[Led] = CRGB::SaddleBrown;
        delay(100);
        FastLED.show();
    }  
    bl_switch_cnt = 0;
    bl_off = 1;
    tft.fillScreen(TFT_WHITE);
    digitalWrite(TFT_BL, HIGH);
    for(int Led = 0; Led < NUM_LEDS; Led = Led + 1) {
          // Turn our current led on to white, then show the leds
        leds[Led] = CRGB::Black;
        delay(100);
        FastLED.show();
    }  
  }

  if(button.getPressedInterrupt())  //Check to see if a button has been pressed
  {
    uint8_t pressed = button.getPressed(); //Read which button has been pressed
    
    if (pressed){
      bl_switch_cnt = 0;
      if (bl_off == 1){
        if (activeWindow == 0) lv_disp_load_scr(scr);
        if (activeWindow == 1) lv_disp_load_scr(scr2);
        if (activeWindow == 3) lv_disp_load_scr(hallway_scr);
        digitalWrite(TFT_BL, LOW);
       // windowWall();
       // lv_obj_del(winWall);
        bl_off = 0;
      }
    }

    if(pressed & 0x01)
    {
      Serial.println("Button A pressed!");
    }
    if(pressed & 0x02)
    {
      Serial.println("Button B pressed!");
    }
    if(pressed & 0x04)
    {
      Serial.println("Button UP pressed!");
    }
    if(pressed & 0x08)
    {
      Serial.println("Button DOWN pressed!");
    }
    if(pressed & 0x10)
    {
      Serial.println("Button LEFT pressed!");
      selfSend = 1;
      if (activeWindow == 1){
        if (activeSlider == 0){
          brightnessleft-=16;
          if (brightnessleft < 0) brightnessleft = 0;
          lv_slider_set_value(sliderleft, brightnessleft, LV_ANIM_ON);
          snprintf (msg, MSG_BUFFER_SIZE, "%d", lv_slider_get_value(sliderleft)); // copy payload value into msg buffer
          client.publish("DIM12967876/Brightness1",msg);
          Serial.println(brightnessleft);
        } else {
          brightnessright-=16;
          if (brightnessright < 0) brightnessright = 0;
          lv_slider_set_value(sliderright, brightnessright, LV_ANIM_ON);
          snprintf (msg, MSG_BUFFER_SIZE, "%d", lv_slider_get_value(sliderright)); // copy payload value into msg buffer
          client.publish("DIM12967876/Brightness2",msg);
        }
      }
      if (activeWindow == 3){
        hallwayBrightness -= 8;
        if (hallwayBrightness < 0) hallwayBrightness = 0;
        lv_slider_set_value(sliderHallway, hallwayBrightness, LV_ANIM_ON);
        numLEDhallway = hallwayBrightness/116;
        leds[numLEDhallway] = CRGB::Black;  
        FastLED.show();
      }     
    }
    if(pressed & 0x20)
    {
      Serial.println("Button RIGHT pressed!");
      selfSend = 1;
      if (activeWindow == 1){
        if (activeSlider == 0){
          brightnessleft+=32;
          if (brightnessleft > 1024) brightnessleft = 1024;
          lv_slider_set_value(sliderleft, brightnessleft, LV_ANIM_ON);
          snprintf (msg, MSG_BUFFER_SIZE, "%d", lv_slider_get_value(sliderleft)); // copy payload value into msg buffer
          client.publish("DIM12967876/Brightness1",msg);
          Serial.println(brightnessleft);
          selfSend = 0;
        } else {
          brightnessright+=32;
          if (brightnessright > 1024) brightnessright = 1024;
          lv_slider_set_value(sliderright, brightnessright, LV_ANIM_ON);
          snprintf (msg, MSG_BUFFER_SIZE, "%d", lv_slider_get_value(sliderright)); // copy payload value into msg buffer
          client.publish("DIM12967876/Brightness2",msg);
          selfSend = 0;
        }
      }
      if (activeWindow == 3){
        hallwayBrightness += 8;
        if (hallwayBrightness > 716) hallwayBrightness = 716;
        lv_slider_set_value(sliderHallway, hallwayBrightness, LV_ANIM_ON);
        numLEDhallway = hallwayBrightness/116;
        leds[numLEDhallway] = CRGB::DarkTurquoise;  
        FastLED.show();
      }     
    }
    if(pressed & 0x40)
    {
      Serial.println("Button CENTER pressed!");
    }
    delay(30);
  }

  if(button.getClickedInterrupt())  //Check to see if a button has been released
  {
    uint8_t clicked = button.getClicked(); //Read which button has been released
    
    if(clicked & 0x01)
    {
      Serial.println("Button A released!");
      //scr2 = lv_cont_create(NULL, NULL);
      
      lv_disp_load_scr(scr);
      activeWindow = 0;
      for (int i=0; i<NUM_LEDS; i++){
        leds[i] = CRGB::Black;  
        FastLED.show();
      }
      leds[5-btn_cnt] = CRGB::Green;
      FastLED.show();

      if (activeWindow > 0){
        switch (btn_cnt){
          case 0:
         //   lv_obj_del(winWall);
         //   activeWindow = 0;
          break;
          case 1:
        //    lv_obj_del(winWall);
        //    activeWindow = 0;        
          break;
          default:
          break;
        }
      }  
    }
    if(clicked & 0x02)
    {
      Serial.println("Button B released!");
      // *************************************
      // add switching on aisle lamps here !!
      // *************************************
    }
    if(clicked & 0x04) // Button UP released
    {
      Serial.println("Button UP released!");

      if (activeWindow == 1){
        btnDownCnt--;
        if (btnDownCnt < 0) btnDownCnt = 0;
      }
      if (activeWindow == 0){
        lv_btn_toggle(menuItem[btn_cnt]);
        btn_cnt--;
        if (btn_cnt < 0) btn_cnt = 5;
        lv_btn_toggle(menuItem[btn_cnt]);
        for(int Led = 0; Led < NUM_LEDS; Led = Led + 1) {
          // Turn our current led on to white, then show the leds
          leds[Led] = CRGB::Black;
        }  
        leds[5-btn_cnt] = CRGB::Green;  
        FastLED.show();
      }  
      if (activeWindow == 1){
        Serial.print ("Btn Down Cnt:"); Serial.println(btnDownCnt);
        switch (btnDownCnt){
          case 0:
            activeSlider = 0;
            lv_obj_set_style_local_bg_color(sliderleft,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderleft,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(sliderleft, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_RED);
            lv_obj_set_style_local_bg_color(sliderright,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderright,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(sliderright, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_CYAN);
          break;
          case 1:
            activeSlider = 1;
            lv_obj_set_style_local_bg_color(sliderleft,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderleft,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(sliderleft, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_CYAN);
            lv_obj_set_style_local_bg_color(sliderright,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderright,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(sliderright, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_RED);
            lv_obj_set_style_local_bg_color(offButton,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_MAGENTA);
          break;
          case 2: // off button
            activeSlider = 3;
            lv_obj_set_style_local_bg_color(sliderright,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderright,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(sliderright, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_RED);
            lv_obj_set_style_local_bg_color(offButton,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
          break;  
          default:
          break;
        }
    
      }
      if (activeWindow == 3){
        Serial.print("act. Win = 3 -"); Serial.println(btnDownCnt);

        btnDownCnt--;
        if (btnDownCnt < 0) btnDownCnt = 0;

        switch (btnDownCnt){
          case 0:
            lv_obj_set_style_local_bg_color(hallway1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(hallway2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_BLUE);
            //if (hallway1State == 1) lv_obj_set_style_local_bg_color(hallway1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_CYAN);
          break;
          case 1:
            lv_obj_set_style_local_bg_color(hallway1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_BLUE);
            //if (hallway1State == 1) lv_obj_set_style_local_bg_color(hallway1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_CYAN);
            lv_obj_set_style_local_bg_color(hallway2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderHallway,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderHallway,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(sliderHallway, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_CYAN);     
          break;
          case 2:
            lv_obj_set_style_local_bg_color(sliderHallway,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderHallway,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(sliderHallway, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_RED);         
          break;
          default:
          break;
        }
      }
      if (activeWindow == 5){
        Serial.print("btn dwn office: "); Serial.println(btnDownCnt);

        btnDownCnt--;
        if (btnDownCnt < 0) btnDownCnt = 2;
        switch (btnDownCnt){
          case 0:
            lv_obj_set_style_local_bg_color(lab1,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(lab2,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(lab3,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            //if (hallway1State == 1) lv_obj_set_style_local_bg_color(hallway1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_CYAN);
          break;
          case 1:
            lv_obj_set_style_local_bg_color(lab1,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(lab2,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(lab3,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            //if (hallway1State == 1) lv_obj_set_style_local_bg_color(hallway1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_CYAN);
          break;
          case 2:
            lv_obj_set_style_local_bg_color(lab1,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(lab2,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(lab3,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
          break;
          default:
          break;
        }        
      }
    }
    if(clicked & 0x08) // Button DOWN released
    {
      Serial.println("Button DOWN released!");
      
      if (activeWindow == 0){
        lv_btn_toggle(menuItem[btn_cnt]);
        btn_cnt++;
        if (btn_cnt > 5) btn_cnt = 0;
        lv_btn_toggle(menuItem[btn_cnt]);
        for(int Led = 0; Led < NUM_LEDS; Led = Led + 1) {
          // Turn our current led on to white, then show the leds
          leds[Led] = CRGB::Black;
        }  
        leds[5-btn_cnt] = CRGB::Green;  
        FastLED.show();
        Serial.println(btn_cnt);
      }      
      if (activeWindow == 1){
        btnDownCnt++;
        if (btnDownCnt > 2) btnDownCnt = 2;
        Serial.print ("Btn Down Cnt:"); Serial.println(btnDownCnt);
        switch (btnDownCnt){
          case 0:
            activeSlider = 0;
            lv_obj_set_style_local_bg_color(sliderleft,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderleft,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(sliderleft, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_RED);
            lv_obj_set_style_local_bg_color(sliderright,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderright,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(sliderright, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_CYAN);
          break;
          case 1:
            activeSlider = 1;
            lv_obj_set_style_local_bg_color(sliderleft,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderleft,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(sliderleft, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_CYAN);
            lv_obj_set_style_local_bg_color(sliderright,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderright,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(sliderright, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_RED);
            
            lv_obj_set_style_local_bg_color(offButton,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_MAGENTA);
          break;
          case 2: // off button
            activeSlider = 2; // means active off button
            lv_obj_set_style_local_bg_color(sliderright,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderright,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(sliderright, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_CYAN);
            
            lv_obj_set_style_local_bg_color(offButton,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
          break;  
          default:
          break;
        }
      
      }   
      if (activeWindow == 3){
        Serial.print("act. Win = 3 -"); Serial.println(btnDownCnt);

        btnDownCnt++;
        if (btnDownCnt > 3) btnDownCnt = 0;
        switch (btnDownCnt){
          case 0:
            lv_obj_set_style_local_bg_color(hallway1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(hallway2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_BLUE);
            lv_obj_set_style_local_bg_color(hallwayoff,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            //if (hallway1State == 1) lv_obj_set_style_local_bg_color(hallway1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_CYAN);
          break;
          case 1:
            lv_obj_set_style_local_bg_color(hallway1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_BLUE);
            lv_obj_set_style_local_bg_color(hallway2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            //if (hallway1State == 1) lv_obj_set_style_local_bg_color(hallway1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_CYAN);
          break;
          case 2:
            lv_obj_set_style_local_bg_color(hallway2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_BLUE);
            lv_obj_set_style_local_bg_color(sliderHallway,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderHallway,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(sliderHallway, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_RED);         
          break;
          case 3:
            lv_obj_set_style_local_bg_color(sliderHallway,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(sliderHallway,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(sliderHallway, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_CYAN);         
            lv_obj_set_style_local_bg_color(hallwayoff,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
          break;
          default:
          break;
        }
      }
      if (activeWindow == 5){
        Serial.print("btn dwn office: "); Serial.println(btnDownCnt);
        btnDownCnt++;
        if (btnDownCnt > 2) btnDownCnt = 0;
        switch (btnDownCnt){
          case 0: 
            lv_obj_set_style_local_bg_color(lab1,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            lv_obj_set_style_local_bg_color(lab2,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_BLUE);
            lv_obj_set_style_local_bg_color(lab3,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_BLUE);
            //if (hallway1State == 1) lv_obj_set_style_local_bg_color(hallway1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_CYAN);
          break;
          case 1:
            lv_obj_set_style_local_bg_color(lab1,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_BLUE);
            lv_obj_set_style_local_bg_color(lab2,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
            //if (hallway1State == 1) lv_obj_set_style_local_bg_color(hallway1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_CYAN);
          break;
          case 2:
            lv_obj_set_style_local_bg_color(lab1,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(lab2,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_NAVY);
            lv_obj_set_style_local_bg_color(lab3,LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
          break;
          default:
          break;
        }        
      }
    }
    if(clicked & 0x10) // Button LEFT released
    {
      Serial.println("Button LEFT released!");

      if (activeWindow == 1){
        if (activeSlider == 0){
          lv_slider_set_value(sliderleft, brightnessleft, LV_ANIM_ON);
          snprintf (msg, MSG_BUFFER_SIZE, "%d", lv_slider_get_value(sliderleft)); // copy payload value into msg buffer
          client.publish("DIM12967876/Brightness1",msg);
          selfSend = 0;
        } else {
          lv_slider_set_value(sliderright, brightnessright, LV_ANIM_ON);
          snprintf (msg, MSG_BUFFER_SIZE, "%d", lv_slider_get_value(sliderright)); // copy payload value into msg buffer
          client.publish("DIM12967876/Brightness2",msg);
          selfSend = 0;
        }
      }
      if (activeWindow == 3){        
        if (hallwayBrightness == 0){
          redval = 0;
        } else {
          redval = hallwayBrightness + 250;
        }
        snprintf (msg, MSG_BUFFER_SIZE, "%d", redval); // red 
        client.publish("SWI13651033/Brightness1",msg);

        greenval = hallwayBrightness;
        snprintf (msg, MSG_BUFFER_SIZE, "%d", greenval); // green 
        client.publish("SWI13651033/Brightness2",msg);
        //lv_slider_set_value(sliderHallway, hallwayBrightness, LV_ANIM_ON);
      }

    }
    if(clicked & 0x20) // Button right released
    {
      Serial.print("Button RIGHT released! act. window: "); Serial.println(activeWindow);
      if (activeWindow == 1){
          Serial.print("RIGHT act. Window: "); Serial.println(activeWindow);
          Serial.print("RIGHT Btn Cnt: "); Serial.println(btn_cnt);
          Serial.print("RIGHT Btn Down cnt: "); Serial.println(btnDownCnt);    
        if (activeSlider == 0){
          lv_slider_set_value(sliderleft, brightnessleft, LV_ANIM_ON);
          snprintf (msg, MSG_BUFFER_SIZE, "%d", lv_slider_get_value(sliderleft)); // copy payload value into msg buffer
          client.publish("DIM12967876/Brightness1",msg);
          selfSend = 0;
        } else {
          lv_slider_set_value(sliderright, brightnessright, LV_ANIM_ON);
          snprintf (msg, MSG_BUFFER_SIZE, "%d", lv_slider_get_value(sliderright)); // copy payload value into msg buffer
          client.publish("DIM12967876/Brightness2",msg);
          selfSend = 0;
        }      
      }
      if (activeWindow == 3){
        redval = hallwayBrightness+300;
        snprintf (msg, MSG_BUFFER_SIZE, "%d", redval); // red 
        client.publish("SWI13651033/Brightness1",msg);

        greenval = hallwayBrightness;
        snprintf (msg, MSG_BUFFER_SIZE, "%d", greenval); // green 
        client.publish("SWI13651033/Brightness2",msg);

        Serial.print("hallway Brightn.: "); Serial.print(hallwayBrightness);
      }
    }

    if(clicked & 0x40) // Button center released
    {    
      Serial.println("Button CENTER released!");
      Serial.print("act. Window: "); Serial.println(activeWindow);
      Serial.print("Btn Cnt: "); Serial.println(btn_cnt);
      Serial.print("Btn Down cnt: "); Serial.println(btnDownCnt);

      switch (activeWindow){
       case 0:
        switch (btn_cnt){
          case 0: // Oakland bay
            btnDownCnt = 0;           
            static lv_style_t style;
            lv_style_init(&style);

            prev = lv_scr_act(); // save previously active screen

            scr2 = lv_cont_create(NULL, NULL);
            lv_obj_add_style(scr2, LV_STATE_DEFAULT, &style);
            lv_disp_load_scr(scr2);
            
            // Make a gradient
            lv_style_set_bg_opa(&style, LV_STATE_DEFAULT, LV_OPA_COVER);
            lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_MAGENTA);
            lv_style_set_bg_grad_color(&style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
            lv_style_set_bg_grad_dir(&style, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);

            //*Shift the gradient to the bottom
            lv_style_set_bg_main_stop(&style, LV_STATE_DEFAULT, 43);
            lv_style_set_bg_grad_stop(&style, LV_STATE_DEFAULT, 212);

            //lv_scr_load(scr2);      
            //lv_obj_del(prev);
            //windowWall();
            drawOaklandBay();

            lv_disp_load_scr(scr2);
            delay(200);
            activeWindow  = 1;
          break;
          case 1: // TOKYO STREETS
          break;
          case 2: // hallway
            Serial.print("Hall way led: "); Serial.println(numLEDhallway);
            prev = lv_scr_act(); // save previously active screen
            hallway_scr = lv_cont_create(NULL, NULL);
            lv_obj_add_style(hallway_scr, LV_STATE_DEFAULT, &style);
            lv_disp_load_scr(hallway_scr);
            // Make a gradient
            lv_style_set_bg_opa(&style, LV_STATE_DEFAULT, LV_OPA_COVER);
            lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_CYAN);
            lv_style_set_bg_grad_color(&style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
            lv_style_set_bg_grad_dir(&style, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
            // Shift the gradient to the bottom
            lv_style_set_bg_main_stop(&style, LV_STATE_DEFAULT, 3);
            lv_style_set_bg_grad_stop(&style, LV_STATE_DEFAULT, 152);
            drawHallway();
            lv_disp_load_scr(hallway_scr);
            //delay(200);
            btnDownCnt = 0;
            for (int i=0; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
            for (int i=0; i < numLEDhallway+1; i++) leds[i] = CRGB::DarkTurquoise;
            FastLED.show();
            activeWindow  = 3;
          break;
          case 4: // Officelab
            prev = lv_scr_act(); // save previously active screen
            // make gradient moved to drawLab() !
            lab_scr = lv_cont_create(NULL, NULL);
            
            //lv_obj_add_style(lab_scr, LV_STATE_DEFAULT, &style);
            lv_disp_load_scr(lab_scr);
            drawLab();
            lv_disp_load_scr(lab_scr);
            btnDownCnt = 0;
          break;
          case 5: // Weatherman
            prev = lv_scr_act(); // save previously active screen
            weather_scr = lv_cont_create(NULL, NULL);
            lv_obj_add_style(weather_scr, LV_STATE_DEFAULT, &style);
            lv_disp_load_scr(weather_scr);
            // Make a gradient
            lv_style_set_bg_opa(&style, LV_STATE_DEFAULT, LV_OPA_COVER);
            lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
            lv_style_set_bg_grad_color(&style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
            lv_style_set_bg_grad_dir(&style, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
            //*Shift the gradient to the bottom
            lv_style_set_bg_main_stop(&style, LV_STATE_DEFAULT, 43);
            lv_style_set_bg_grad_stop(&style, LV_STATE_DEFAULT, 212);
            //lv_scr_load(scr2);      
            //lv_obj_del(prev);
            //windowWall();
            drawWeatherman();
            lv_disp_load_scr(weather_scr);
            Serial.print("draw Weatherman !"); Serial.println();
          break;
          default:
          break;
        }
       break;
       case 1:
          if (activeSlider == 2){
            msg[0] = 0;
            client.publish("DIM12967876/Brightness1",msg);
            client.publish("DIM12967876/Brightness2",msg);
            brightnessleft = 0;
            brightnessright = 0;
            lv_slider_set_value(sliderleft, brightnessleft, LV_ANIM_ON);
            lv_slider_set_value(sliderright, brightnessright, LV_ANIM_ON);
            delay(500);
            //lv_obj_del(winWall);
            lv_disp_load_scr(scr);
            activeWindow = 0;
            Serial.print("1 center release act. Window: "); Serial.println(activeWindow);
          } else {
            lv_disp_load_scr(scr);
            activeWindow = 0;
          }
       break;  
       case 3:
          switch (btnDownCnt){
            case 0:
              hallway1State = !hallway1State;
              Serial.print("Hallway 1 state: "); Serial.println(hallway1State);
              switch (hallway1State){
                case 0:                  
                  snprintf (msg, MSG_BUFFER_SIZE, "%d", 0);
                  client.publish("SWI13651033/Switch1",msg);
                  // SWI13651033/Switch1
                  lv_obj_set_style_local_bg_color(hallLED1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);                  
                break;
                case 1:                  
                  snprintf (msg, MSG_BUFFER_SIZE, "%d", 1);
                  client.publish("SWI13651033/Switch1",msg);
                  lv_obj_set_style_local_bg_color(hallLED1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_MAGENTA);                  
                break;
              }
            break;
            case 1:
              hallway2State = !hallway2State;
              Serial.print("Hallway 2 state: "); Serial.println(hallway1State);
              switch (hallway2State){
                case 0:
                  snprintf (msg, MSG_BUFFER_SIZE, "%d", 0);
                  client.publish("SWI13651033/Switch2",msg);
                  
                  lv_obj_set_style_local_bg_color(hallLED2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);                  
                break;
                case 1:
                  snprintf (msg, MSG_BUFFER_SIZE, "%d", 1);
                  client.publish("SWI13651033/Switch2",msg);
                  lv_obj_set_style_local_bg_color(hallLED2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_MAGENTA);                  
                break;
              }            
            break;
            case 2:
            break;
          }
       break;
       case 5:
          switch(btnDownCnt){
            case 0:
              office1State = !office1State;
              if (office1State == 1){
                lv_obj_set_style_local_bg_color(labLED1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_MAGENTA);
              } else {
                lv_obj_set_style_local_bg_color(labLED1,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
              }
            break;
            case 1:
              office2State = !office2State;
              if (office2State == 1){
                  snprintf (msg, MSG_BUFFER_SIZE, "%d", 1);
                  client.publish("Switch2267261/setstate",msg);
                  lv_obj_set_style_local_bg_color(labLED2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_MAGENTA);
              } else {
                  snprintf (msg, MSG_BUFFER_SIZE, "%d", 0);
                  client.publish("Switch2267261/setstate",msg);
                  lv_obj_set_style_local_bg_color(labLED2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
              }
            break;
            case 2:
              office3State = !office3State;
              if (office3State == 1){
                lv_obj_set_style_local_bg_color(labLED3,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_MAGENTA);
              } else {
                lv_obj_set_style_local_bg_color(labLED3,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
              }            
            break;
          }
          //lv_disp_load_scr(scr);
          //activeWindow = 0;
       break;  

       default:
       break; 
      }
     
    }
  }
}