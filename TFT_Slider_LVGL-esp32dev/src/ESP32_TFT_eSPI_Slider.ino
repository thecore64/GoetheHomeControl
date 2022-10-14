// The SPIFFS (FLASH filing system) is used to hold touch screen
// calibration data
#include "FS.h"

#include <lvgl.h>
#include <TFT_eSPI.h>

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <time.h>
#include "NPT_time.h"
#include <ArduinoOTA.h>
#include <ota.h>
//#include <globals.h>
#include <windowAisle.h>
#include <windowWall.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <stdio.h>

#define _TIMERINTERRUPT_LOGLEVEL_     4
#include "ESP32TimerInterrupt.h"
#define TIMER1_INTERVAL_MS        100
// Init ESP32 timer 1
ESP32Timer ITimer1(1);

int otaRunning;
int otaProgress;
int old_otaProgress;
String otaProgressStr;
String IPstr;
char buffer[20];

//------- network credentials
const char* ssid = "TheCore";
const char* password = "M1n0tauru5";
const char* mqtt_server = "192.168.0.5";

#define LVGL_TICK_PERIOD 60

//Ticker tick; /* timer for interrupt handler */
TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];

//lv_obj_t * slider;
//lv_obj_t * slider2;

lv_obj_t * slider_label;
lv_obj_t * slider_label2;

lv_obj_t * myButton;
lv_obj_t * myButtonLabel;
lv_obj_t * myLabel;
lv_obj_t * btn25;
lv_obj_t * btn50;
lv_obj_t * btn100;
lv_obj_t *wifilabel;
lv_obj_t *labeltime;

lv_style_t myButtonStyleREL; //released style
lv_style_t myButtonStylePR; //pressed style

int time_cnt = 0;
int bl_switch_cnt = 0;
int bl_off = 0;
int tft_dim_range[6] = {600,700,800,900,990,1024};
int tft_dim_index = 0;

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {

#ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
#endif

    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
#ifdef DEBUG      
      Serial.println("connected");
#endif
      subscribe_str = "Switch2267261/State";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);
#ifdef DEBUG
      Serial.print("subscribe:"); Serial.println(subscribe_buffer); 
#endif
      subscribe_str = "Switch8745923/State1";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);
#ifdef DEBUG
      Serial.print("subscribe:"); Serial.println(subscribe_buffer); 
#endif
      subscribe_str = "Switch8745923/State2";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);
#ifdef DEBUG
      Serial.print("subscribe:"); Serial.println(subscribe_buffer); 
#endif       
      subscribe_str = "DIM12967876/Brightness1";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);

      subscribe_str = "DIM12967876/Brightness2";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);

      snprintf (msg, MSG_BUFFER_SIZE, "%d", 1); // copy payload value into msg buffer
      client.publish("DIM12967876/GetState",msg); // request actual status from device

    } else {
#ifdef DEBUG      
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
#endif      
      // Wait 2 seconds before retrying
      delay(2000);
      mqtt_reconnectcnt ++;
      if (mqtt_reconnectcnt > 10){
#ifdef DEBUG      
       Serial.println();
       Serial.print("max. reconnect count reached, performing reset...");
#endif         
       ESP.restart();
      }
    }
  }
}

// -------------------------------------------------------------------------

bool IRAM_ATTR TimerHandler1(void * timerNo)
{ 
  bl_switch_cnt ++;
  time_cnt ++;

  return true;
}

// -------------------------------------------------------------------------

void lv_winPaintings_close_event_cb2(lv_obj_t * btn, lv_event_t event)
{
  if(event == LV_EVENT_RELEASED) {

    lv_obj_t * win = lv_win_get_from_btn(btn); 

    lv_obj_del(win);
  }
}

void windowPaintings(void){
  lv_obj_t * win = lv_win_create(lv_scr_act(), NULL);
  lv_win_set_title(win, "LED Bilder"); //Set the title

  lv_obj_t * close_btn = lv_win_add_btn(win, LV_SYMBOL_CLOSE);  /*Add close button and use built-in close action*/

  lv_obj_set_event_cb(close_btn, lv_winPaintings_close_event_cb2);
  //lv_win_add_btn(win, LV_SYMBOL_SETTINGS);        /*Add a setup button*/ 

  /*Add labels*/
  lv_obj_t * txt = lv_label_create(win, NULL);
  lv_label_set_text(txt, "Links");
  lv_obj_align(txt, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 10);


  /* Create a slider */
  lv_obj_t * slider = lv_slider_create(win, NULL);
  // first parameter of this function determines which screen or window the control belongs to,
  // if it is the wrong one, the object will not be destroyed by deleting the window
  lv_obj_set_event_cb(slider, slider_event_cb);         //Assign an event function
  lv_obj_set_width(slider, screenWidth-70);                        /*Set the width*/
  lv_obj_set_height(slider, 40);
  lv_obj_align(slider, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 40);    /*Align to the center of the parent (screen)*/
  // LV_SLIDER_PART_xx determines which part of the slider we want to change color
  // BG background color, KNOB of course, 
  // INDIC is the part which gets filled when moving
  lv_obj_set_style_local_bg_color(slider,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
  lv_obj_set_style_local_bg_color(slider,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
  lv_obj_set_style_local_bg_color(slider, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_CYAN);
}

static void btn_paintings_event_handler(lv_obj_t * obj, lv_event_t event){
    if(event == LV_EVENT_CLICKED) {
        windowPaintings();
    }
}

static void btn4_event_handler(lv_obj_t * obj, lv_event_t event){
    if(event == LV_EVENT_CLICKED) {
      // set both LED lines, red and green, to 50% brightness
        snprintf (msg, MSG_BUFFER_SIZE, "%d", redval); // red 
        client.publish("SWI13651033/Brightness1",msg);
        snprintf (msg, MSG_BUFFER_SIZE, "%d", greenval); // green 
        client.publish("SWI13651033/Brightness2",msg);
        windowAisleLED();
    }
}


#if USE_LV_LOG != 0
/* Serial debugging */
void my_print(lv_log_level_t level, const char * file, uint32_t line, const char * dsc)
{

  Serial.printf("%s@%d->%s\r\n", file, line, dsc);
  delay(100);
}
#endif

void printEvent(String Event, lv_event_t event)
{
  
  Serial.print(Event);
  printf(" ");

  switch(event) {
      case LV_EVENT_PRESSED:
          printf("Pressed\n");
          break;

      case LV_EVENT_SHORT_CLICKED:
          printf("Short clicked\n");
          break;

      case LV_EVENT_CLICKED:
          printf("Clicked\n");
          break;

      case LV_EVENT_LONG_PRESSED:
          printf("Long press\n");
          break;

      case LV_EVENT_LONG_PRESSED_REPEAT:
          printf("Long press repeat\n");
          break;

      case LV_EVENT_RELEASED:
          printf("Released\n");
          break;
  }
}


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

bool my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data)
{
    uint16_t touchX, touchY;

    bool touched = tft.getTouch(&touchX, &touchY, 600);

    if (touched){
      ledcWrite(0,0); // set full brightness backlight
      bl_switch_cnt = 0;
      bl_off = 0;
    }

    if(!touched)
    {
      return false;
    }

    if(touchX>screenWidth || touchY > screenHeight)
    {
      Serial.println("Y or y outside of expected parameters..");
      Serial.print("y:");
      Serial.print(touchX);
      Serial.print(" x:");
      Serial.print(touchY);
    }
    else
    {

      data->state = touched ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL; 
  
      /*Save the state and save the pressed coordinate*/
      //if(data->state == LV_INDEV_STATE_PR) touchpad_get_xy(&last_x, &last_y);
     
      /*Set the coordinates (if released use the last pressed coordinates)*/
      data->point.x = 320-touchX;
      data->point.y = 240-touchY;
  
      Serial.print("Data x");
      Serial.println(touchX);
      
      Serial.print("Data y");
      Serial.println(touchY);

    }

    return false; /*Return `false` because we are not buffering and no more data to read*/
}

static void btn2_event_handler(lv_obj_t * obj, lv_event_t event){
    if(event == LV_EVENT_CLICKED) {
        printf("Clicked\n");
    }
    if (lv_btn_get_state(obj) == LV_STATE_CHECKED){
      snprintf (msg, MSG_BUFFER_SIZE, "%d", 1); // copy payload value into msg buffer
      client.publish("SWI13651033/Switch1",msg);
    }

    if (lv_btn_get_state(obj) == LV_STATE_DEFAULT){
      snprintf (msg, MSG_BUFFER_SIZE, "%d", 0); // copy payload value into msg buffer
      client.publish("SWI13651033/Switch1",msg);
    }
}

static void btn3_event_handler(lv_obj_t * obj, lv_event_t event){
    if(event == LV_EVENT_CLICKED) {
        printf("Clicked 3\n");
    }
    if (lv_btn_get_state(obj) == LV_STATE_CHECKED){
      snprintf (msg, MSG_BUFFER_SIZE, "%d", 1); // copy payload value into msg buffer
      client.publish("SWI13651033/Switch2",msg);
    }

    if (lv_btn_get_state(obj) == LV_STATE_DEFAULT){
      snprintf (msg, MSG_BUFFER_SIZE, "%d", 0); // copy payload value into msg buffer
      client.publish("SWI13651033/Switch2",msg);
    }
}

static void btn_event_handler(lv_obj_t * obj, lv_event_t event){
    if(event == LV_EVENT_CLICKED) {
        printf("Clicked\n");
        windowWall();
    }
}

static void btn25_event_handler(lv_obj_t * obj, lv_event_t event){
//    if(event == LV_EVENT_CLICKED) {
//        printf("Clicked\n");
//    }
    if (lv_btn_get_state(obj) == LV_STATE_CHECKED){
      lv_obj_set_state(btn50,LV_STATE_DEFAULT);
      lv_obj_set_state(btn100,LV_STATE_DEFAULT);
      snprintf (msg, MSG_BUFFER_SIZE, "%d", wall_bright25); // copy payload value into msg buffer
      //client.publish("Switch8745923/switch1",msg);
      client.publish("DIM12967876/Brightness1",msg);
      client.publish("DIM12967876/Brightness2",msg);
      wall_brightness = 100;
    }
    if (lv_btn_get_state(obj) == LV_STATE_DEFAULT){
      snprintf (msg, MSG_BUFFER_SIZE, "%d", 0); // copy payload value into msg buffer
      //client.publish("Switch8745923/switch1",msg);
      client.publish("DIM12967876/Brightness1",msg);
      client.publish("DIM12967876/Brightness2",msg);
      wall_brightness = 0;
    }
}

static void btn50_event_handler(lv_obj_t * obj, lv_event_t event){
//    if(event == LV_EVENT_CLICKED) {
//        printf("Clicked\n");
//    }
    if (lv_btn_get_state(obj) == LV_STATE_CHECKED){
      lv_obj_set_state(btn100,LV_STATE_DEFAULT);
      lv_obj_set_state(btn25,LV_STATE_DEFAULT);
      snprintf (msg, MSG_BUFFER_SIZE, "%d", wall_bright50); // copy payload value into msg buffer
      //client.publish("Switch8745923/switch1",msg);
      client.publish("DIM12967876/Brightness1",msg);
      client.publish("DIM12967876/Brightness2",msg);
      wall_brightness = 200;
    }
    if (lv_btn_get_state(obj) == LV_STATE_DEFAULT){
      snprintf (msg, MSG_BUFFER_SIZE, "%d", 0); // copy payload value into msg buffer
      //client.publish("Switch8745923/switch1",msg);
      client.publish("DIM12967876/Brightness1",msg);
      client.publish("DIM12967876/Brightness2",msg);
      wall_brightness = 0;
    }
}

static void btn100_event_handler(lv_obj_t * obj, lv_event_t event){
//    if(event == LV_EVENT_CLICKED) {
//        printf("Clicked\n");
//    }

    if (lv_btn_get_state(obj) == LV_STATE_CHECKED){
      lv_obj_set_state(btn50,LV_STATE_DEFAULT);
      lv_obj_set_state(btn25,LV_STATE_DEFAULT);
      snprintf (msg, MSG_BUFFER_SIZE, "%d", wall_bright100); // copy payload value into msg buffer
      //client.publish("Switch8745923/switch1",msg);
      //client.publish("Switch8745923/switch2",msg);
      client.publish("DIM12967876/Brightness1",msg);
      client.publish("DIM12967876/Brightness2",msg);
      wall_brightness = 800;
    }
    if (lv_btn_get_state(obj) == LV_STATE_DEFAULT){
      snprintf (msg, MSG_BUFFER_SIZE, "%d", 0); // copy payload value into msg buffer
      //client.publish("Switch8745923/switch1",msg);
      //client.publish("Switch8745923/switch2",msg);
      client.publish("DIM12967876/Brightness1",msg);
      client.publish("DIM12967876/Brightness2",msg);
      wall_brightness = 0;
    }
}

static void event_handler(lv_obj_t * obj, lv_event_t event)
{
  /*
    if(event == LV_EVENT_CLICKED) {
        printf("Clicked\n");
    }
    else if(event == LV_EVENT_VALUE_CHANGED) {
        printf("Toggled\n");
    } */
}

void setupWiFi() {

  delay(10);
  // We start by connecting to a WiFi network
#ifdef DEBUG  
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
#endif
  tft.setCursor(12,25);
  tft.print("connecting to: "); tft.print(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef DEBUG
    Serial.print(".");
#endif
  }
  randomSeed(micros());
#ifdef DEBUG
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: "); Serial.println(WiFi.localIP());
#endif
  tft.setCursor(12,35);
  tft.print("WiFi connection successful !");
  tft.setCursor(12,45);
  tft.print(WiFi.localIP());
}


void setup() {

  pinMode(TFT_BL, OUTPUT);
  
  digitalWrite(TFT_BL, LOW);

  if (ITimer1.attachInterruptInterval(TIMER1_INTERVAL_MS*10, (void (*)())TimerHandler1))
    {
      Serial.print(F("Starting  ITimer1 OK, millis() = ")); Serial.println(millis());
      //tft.print("starting timer ... OK");
    } else {
      Serial.println(F("Can't set ITimer1 !"));
      //tft.print("starting timer ... failed");
    }

  Serial.begin(115200); /* prepare for possible serial debug */
/*
  WiFiManager wifiManager;
  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();
 
  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  res = wifiManager.autoConnect("HOMECONTROLAP"); // anonymous ap
  Serial.println("enabling AutoConnect Access Point...");
  //res = wifiManager.autoConnect("AutoConnectAP","password"); // password protected ap
*/
  setupWiFi();

  setupOTA();
  
  client.setServer(mqtt_server, 1883);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getTime();

  lv_init();

  ledcSetup(0, 30000, 10);
  ledcAttachPin(TFT_BL, 0); 
  ledcWrite(0,0); // set full brightness backlight

  #if USE_LV_LOG != 0
    lv_log_register_print_cb(my_print); /* register print function for debugging */
  #endif

  tft.begin(); /* TFT init */
  tft.setRotation(3);

  uint16_t calData[5] = { 275, 3620, 264, 3532, 1 };
  tft.setTouch(calData);

  lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);

  /*Initialize the display*/
  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the input device driver*/
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);             /*Descriptor of a input device driver*/
  indev_drv.type = LV_INDEV_TYPE_POINTER;    /*Touch pad is a pointer-like device*/
  indev_drv.read_cb = my_touchpad_read;      /*Set your driver function*/
  lv_indev_drv_register(&indev_drv);         /*Finally register the driver*/

  //Set the theme..
  lv_theme_t * th = lv_theme_material_init(LV_THEME_DEFAULT_COLOR_PRIMARY , LV_THEME_DEFAULT_COLOR_SECONDARY, LV_THEME_DEFAULT_FLAG, LV_THEME_DEFAULT_FONT_SMALL , LV_THEME_DEFAULT_FONT_NORMAL, LV_THEME_DEFAULT_FONT_SUBTITLE, LV_THEME_DEFAULT_FONT_TITLE);     
  //LV_THEME_DEFAULT_COLOR_PRIMARY
  lv_theme_set_act(th);

    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_radius(&style, LV_STATE_DEFAULT, 5);

    /*Make a gradient*/
    lv_style_set_bg_opa(&style, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_CYAN);
    lv_style_set_bg_grad_color(&style, LV_STATE_DEFAULT, LV_COLOR_BLUE);
    lv_style_set_bg_grad_dir(&style, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);

    /*Shift the gradient to the bottom*/
    lv_style_set_bg_main_stop(&style, LV_STATE_DEFAULT, 48);
    lv_style_set_bg_grad_stop(&style, LV_STATE_DEFAULT, 222);

    lv_obj_t * scr = lv_cont_create(NULL, NULL);
  
     lv_obj_add_style(scr, LV_STATE_DEFAULT, &style);

  lv_disp_load_scr(scr);
  
//**********************************************************
/*
  LV_IMG_DECLARE(Sun_320x240_4bit);
  // lv_img_set_src(img, &my_image_name);

  lv_obj_t * wp = lv_img_create(lv_scr_act(), NULL);
  lv_img_set_src(wp, &Sun_320x240_4bit);
  lv_obj_set_pos(wp,0,0);
  lv_obj_set_width(wp, LV_HOR_RES);
*/  
//**********************************************************

  //lv_obj_t * tv = lv_tabview_create(scr, NULL);
  //lv_obj_set_size(tv, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));

  // create label to show IP adress ---------------------------------------
  lv_obj_t *labelIP = lv_label_create(lv_scr_act(), NULL);
  IPstr = WiFi.localIP().toString();
  IPstr.toCharArray(buffer,IPstr.length()+1);
  lv_obj_set_style_local_text_font(labelIP, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_14);
  // set text color
  lv_obj_set_style_local_text_color(labelIP, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
  lv_label_set_text(labelIP, buffer);
  lv_obj_align(labelIP, NULL, LV_ALIGN_IN_TOP_LEFT, 35, 223);  
  //-----------------------------------------------------------------------
  // create label to hold WiFi icon (fontawesome)
  wifilabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(wifilabel, LV_SYMBOL_WIFI);
  lv_obj_set_style_local_text_color(wifilabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
  lv_obj_align(wifilabel, NULL, LV_ALIGN_IN_TOP_LEFT, 5, 223);  
  //-----------------------------------------------------------------------
  // create label to show actual time -------------------------------------
  labeltime = lv_label_create(lv_scr_act(), NULL);
  lv_obj_set_style_local_text_font(labeltime, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
  // set text color
  lv_obj_set_style_local_text_color(labeltime, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_label_set_text(labeltime, timeinfo_buffer);
  lv_obj_align(labeltime, NULL, LV_ALIGN_IN_TOP_LEFT, 220, 223);  
  //-----------------------------------------------------------------------

  // Create simple label
  lv_obj_t *label = lv_label_create(lv_scr_act(), NULL);
  // set text color
  lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
  // set opacity for the background color
  lv_obj_set_style_local_bg_opa(label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,LV_OPA_COVER);
  // set background color
  lv_obj_set_style_local_bg_color(label,LV_LABEL_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_CYAN);
  
  lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_26);
  lv_label_set_text(label, "    CBE Home Control    ");
  lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 0);

   lv_obj_t * btnlabel;

    lv_obj_t * btn = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(btn, btn_event_handler); // cb = callback, set the buttons callback function
    lv_obj_set_size(btn, 70,50);
    //lv_obj_set_width(btn,50);
    lv_obj_align(btn, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 40);
    //lv_btn_set_checkable(btn, true);
    //lv_btn_toggle(btn2);
    //lv_btn_set_fit2(btn, LV_FIT_TIGHT, LV_FIT_TIGHT);
    btnlabel = lv_label_create(btn, NULL);
    lv_obj_set_style_local_text_font(btnlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
    lv_label_set_text(btnlabel, "Wand");

    btn25 = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(btn25, btn25_event_handler); // cb = callback, set the buttons callback function
    lv_obj_set_size(btn25, 65,50);
    lv_obj_align(btn25, NULL, LV_ALIGN_IN_TOP_LEFT, 100, 40);
    lv_btn_set_checkable(btn25, true);
    //lv_btn_toggle(btn2);
    //lv_btn_set_fit2(btn50, LV_FIT_TIGHT, LV_FIT_TIGHT);
    btnlabel = lv_label_create(btn25, NULL);
    lv_obj_set_style_local_text_font(btnlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
    lv_label_set_text(btnlabel, "25%");

    btn50 = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(btn50, btn50_event_handler); // cb = callback, set the buttons callback function
    lv_obj_set_size(btn50, 65,50);
    lv_obj_align(btn50, NULL, LV_ALIGN_IN_TOP_LEFT, 170, 40);
    lv_btn_set_checkable(btn50, true);
    //lv_btn_toggle(btn2);
    //lv_btn_set_fit2(btn50, LV_FIT_TIGHT, LV_FIT_TIGHT);
    btnlabel = lv_label_create(btn50, NULL);
    lv_obj_set_style_local_text_font(btnlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
    lv_label_set_text(btnlabel, "50%");

    btn100 = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(btn100, btn100_event_handler); // cb = callback, set the buttons callback function
    lv_obj_set_size(btn100, 65,50);
    lv_obj_align(btn100, NULL, LV_ALIGN_IN_TOP_LEFT, 240, 40);
    lv_btn_set_checkable(btn100, true);
    //lv_btn_toggle(btn2);
    //lv_btn_set_fit2(btn50, LV_FIT_TIGHT, LV_FIT_TIGHT);
    btnlabel = lv_label_create(btn100, NULL);
    lv_obj_set_style_local_text_font(btnlabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
    lv_label_set_text(btnlabel, "100%");

    lv_obj_t * btn2 = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(btn2, btn2_event_handler); // cb = callback, set the buttons callback function
    lv_obj_align(btn2, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 115);
    lv_btn_set_checkable(btn2, true);
    //lv_btn_toggle(btn2);
    lv_btn_set_fit2(btn2, LV_FIT_NONE, LV_FIT_TIGHT);
    btnlabel = lv_label_create(btn2, NULL);
    lv_label_set_text(btnlabel, "Gang links");

    lv_obj_t * btn3 = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(btn3, btn3_event_handler); // cb = callback, set the buttons callback function
    lv_obj_align(btn3, NULL, LV_ALIGN_IN_TOP_LEFT, 160, 115);
    lv_btn_set_checkable(btn3, true);
    //lv_btn_toggle(btn3);
    lv_btn_set_fit2(btn3, LV_FIT_NONE, LV_FIT_TIGHT);
    btnlabel = lv_label_create(btn3, NULL);
    lv_label_set_text(btnlabel, "Gang rechts");

    lv_obj_t * btn4 = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(btn4, btn4_event_handler); // cb = callback, set the buttons callback function
    lv_obj_align(btn4, NULL, LV_ALIGN_IN_TOP_LEFT, 20, 175);
    //lv_btn_set_checkable(btn4, true);
    //lv_btn_toggle(btn4);
    lv_btn_set_fit2(btn4, LV_FIT_NONE, LV_FIT_TIGHT);
    btnlabel = lv_label_create(btn4, NULL);
    lv_label_set_text(btnlabel, "LED Gang");

    lv_obj_t * btn5 = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_event_cb(btn5, btn_paintings_event_handler); // cb = callback, set the buttons callback function
    lv_obj_align(btn5, NULL, LV_ALIGN_IN_TOP_LEFT, 160, 175);
    //lv_btn_set_checkable(btn5, true);
    //lv_btn_toggle(btn4);
    lv_btn_set_fit2(btn5, LV_FIT_NONE, LV_FIT_TIGHT);
    btnlabel = lv_label_create(btn5, NULL);
    lv_label_set_text(btnlabel, "Bild");    
}

void loop() {
  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (time_cnt >=1000){
    time_cnt = 0;
    getTime();
    lv_label_set_text(labeltime, timeinfo_buffer);
  }
  if ((bl_switch_cnt >= 20000) && (bl_off == 0)){
    ledcWrite(0,tft_dim_range[4]); // dim backlight
    digitalWrite(TFT_BL, HIGH);
  }
  if (bl_switch_cnt >= 40000){
    bl_switch_cnt = 0;
    bl_off = 1;
    ledcWrite(0,tft_dim_range[5]); // switch off backlight
  }
  lv_task_handler(); /* let the GUI do its work */
  delay(5);
}
