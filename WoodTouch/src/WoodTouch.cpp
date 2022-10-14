#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <PubSubClient.h>
#include <ezButton.h>

static int screenWidth = 128;
static int screenHeight = 160;

String IPstr;
char buffer[30];

//------- network credentials
const char* ssid = "TheCore";
const char* password = "M1n0tauru5";
const char* mqtt_server = "192.168.0.5";

WiFiClient esp32Client;
PubSubClient client(esp32Client);

#define LVGL_TICK_PERIOD 60

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */

static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];

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

float temp,hum;
char thBuffer[30];

lv_obj_t *screenLED[6];
lv_obj_t *btnlabel[6];
lv_obj_t *tempLabel, *humLabel;
String Labels[6] = {"AISLE LEFT", "AISLE RIGHT", "AISLE STRIP", "LIVING STRIP"};

//************ define buttons ****************************************************
ezButton button1(21);  // create ezButton object GPIO 22
ezButton button2(22);  // create ezButton object GPIO 21
ezButton button3(17);  // create ezButton object GPIO 17
//********************************************************************************
// with the touch buttons we use here, press and release events are exchanged, because of
// different logic levels
int btn1State = 0;
int btn2State = 0;
int btn3State = 0;

int redval = 0;
int greenval = 0;
int selfSend = 0;

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

/* Interrupt driven periodic handler */
static void lv_tick_handler(void)
{

  lv_tick_inc(LVGL_TICK_PERIOD);
  //blCounter ++;
}

bool my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data)
{
    uint16_t touchX, touchY;

    bool touched = false; //tft.getTouch(&touchX, &touchY, 600);

    if (touched){
    //  ledcWrite(0,0); // set full brightness backlight
    //  bl_switch_cnt = 0;
    //  bl_off = 0;
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

void InitLVGL(){
  lv_init();

  tft.begin(); /* TFT init */
  tft.setRotation(0);

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
}

void setupWiFi(){
 // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
      tft.setCursor(2,100);
      tft.setTextSize(1);
      tft.setTextColor(TFT_RED);
      tft.print("connect to: "); tft.print(ssid);
      Serial.println("Connecting to WiFi..");
      delay(500);
    }
    tft.setCursor(2,110);
    tft.print("connection successful");
    tft.setCursor(2,120);
    tft.print(WiFi.localIP());
    delay(2500);
}

void reconnect() {
  // Loop until we're reconnected
  tft.setCursor(2,130);
  tft.print("connecting MQTT server");
  tft.setCursor(2,140);
  while (!client.connected()) {

#ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
#endif

    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {

      subscribe_str = "SWI13651033/Switch1";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);

      subscribe_str = "SWI13651033/Switch2";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);

      //subscribe_str = "DIM12967876/Brightness1";
      //subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      //client.subscribe(subscribe_buffer);

      //subscribe_str = "DIM12967876/Brightness2";
      //subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      //client.subscribe(subscribe_buffer);

      //snprintf (msg, MSG_BUFFER_SIZE, "%d", 1); // copy payload value into msg buffer
      //client.publish("DIM12967876/GetState",msg); // request actual status from device

      subscribe_str = "SNSTEMHUMESP32-S2/Temperature";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);

      subscribe_str = "SNSTEMHUMESP32-S2/Humidity";
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
  tft.print("MQTT successful !");
  delay(2000);
}

void callback(char* topic, byte* payload, unsigned int length) {
  for (unsigned int i=0;i<6;i++) c_val[i]=0; 
  
  Serial.print("received message: "); Serial.print(topic); Serial.println();
  
  compare_str="SWI6233300/getState"; // Office desk lamp
  compare_str.toCharArray(compare_buffer,compare_str.length()+1);
  if (strcmp(topic, compare_buffer) == 0){
    Serial.print("SWI6233300/getState: "); Serial.print(topic); Serial.println();
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }
  }  
  
  compare_str="SNSTEMHUMESP32-S2/Temperature";
  compare_str.toCharArray(compare_buffer,compare_str.length()+1);
  if (strcmp(topic, compare_buffer) == 0){
    //Serial.print("received message: "); Serial.print(topic); Serial.println();
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }
    temp = atof(c_val);
    lv_label_set_recolor(tempLabel, true);
    snprintf(thBuffer, MSG_BUFFER_SIZE, "#FF0000 %.1f°C#", temp);
    lv_label_set_text(tempLabel, thBuffer);
    }  

  compare_str="SNSTEMHUMESP32-S2/Humidity";
  compare_str.toCharArray(compare_buffer,compare_str.length()+1);
  if (strcmp(topic, compare_buffer) == 0){
    for (unsigned int i=0;i<length;i++){
      c_val[i]=(char)(payload[i]);   
    }
    hum = atof(c_val);
    lv_label_set_recolor(humLabel, true);
    snprintf(thBuffer, MSG_BUFFER_SIZE, "#0000FF %.1f%%#", hum);
    lv_label_set_text(humLabel, thBuffer);
  }
/*
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
  } */
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
      } */
  //------------------------------------------------------------------------------------------------    
  }     

void setup() {
  Serial.begin(9600);

  InitLVGL();

  pinMode(26, OUTPUT);
  digitalWrite(26, HIGH); // switch on TFT backlight

  setupWiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  static lv_style_t style;
  lv_style_init(&style);
  lv_style_set_radius(&style, LV_STATE_DEFAULT, 5);

  // Make a gradient
  lv_style_set_bg_opa(&style, LV_STATE_DEFAULT, LV_OPA_COVER);
  lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_MAROON);
  lv_style_set_bg_grad_color(&style, LV_STATE_DEFAULT, LV_COLOR_NAVY);
  lv_style_set_bg_grad_dir(&style, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);

  // Shift the gradient to the bottom
  lv_style_set_bg_main_stop(&style, LV_STATE_DEFAULT, 98);
  lv_style_set_bg_grad_stop(&style, LV_STATE_DEFAULT, 320);

  // create and display screen ------------------------------------------------------------------
  lv_obj_t * scr = lv_cont_create(NULL, NULL);
  lv_obj_add_style(scr, LV_STATE_DEFAULT, &style);
  lv_disp_load_scr(scr);

  // create label to show IP address ------------------------------------------------------------
  lv_obj_t *labelIP = lv_label_create(lv_scr_act(), NULL);
  IPstr = WiFi.localIP().toString();
  //IPstr="192.168.0.88";
  IPstr.toCharArray(buffer,IPstr.length()+1);
  lv_obj_set_style_local_text_font(labelIP, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_unscii_8);//&lv_font_montserrat_12);
  // set text color
  lv_obj_set_style_local_text_color(labelIP, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_label_set_text(labelIP, buffer);
  lv_obj_align(labelIP, NULL, LV_ALIGN_IN_TOP_LEFT, 25, 145);  
 
  // create label to hold WiFi icon (fontawesome) ------------------------------------------------
  lv_obj_t * wifilabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(wifilabel, LV_SYMBOL_WIFI);
  lv_obj_set_style_local_text_color(wifilabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
  lv_obj_align(wifilabel, NULL, LV_ALIGN_IN_TOP_LEFT, 1, 140);  

  // create labels for temperature ----------------------------------------------------------------
  tempLabel = lv_label_create(lv_scr_act(), NULL);
  lv_obj_set_style_local_text_color(tempLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
  lv_obj_align(tempLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 1, 110);  
  lv_label_set_text(tempLabel,"00.0°C");
  lv_obj_set_style_local_text_font(tempLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20); 
  lv_obj_set_style_local_text_color(tempLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAGENTA);

// create labels for humidity ----------------------------------------------------------------------
  humLabel = lv_label_create(lv_scr_act(), NULL);
  lv_obj_set_style_local_text_color(humLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
  lv_obj_align(humLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 70, 110);  
  lv_label_set_text(humLabel,"00.0%");
  lv_obj_set_style_local_text_font(humLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20); 
  lv_obj_set_style_local_text_color(humLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_CYAN);

  // create four LED's to show light status -------------------------------------------------------
  int y = 4;
  for (int i = 0; i<4; i++){
    screenLED[i] = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_size(screenLED[i], 115,20);
    lv_obj_align(screenLED[i], NULL, LV_ALIGN_IN_TOP_LEFT, 8, y);  
    lv_obj_set_style_local_bg_color(screenLED[i],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_GREEN);
    lv_obj_set_style_local_border_color(screenLED[i], LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
    btnlabel[i] = lv_label_create(screenLED[i], NULL);
    lv_obj_set_style_local_text_font(btnlabel[i], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_12); //&lv_font_unscii_8); 
    lv_obj_set_style_local_text_color(btnlabel[i], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    int len = Labels[i].length()+1;
    Labels[i].toCharArray(buffer, len);
    lv_label_set_text(btnlabel[i], buffer);   
    y = y+25;
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  lv_tick_handler();
  lv_task_handler(); /* let the GUI do its work! */
  delay(5);

  button1.loop();  // handle button functions
  button2.loop();
  button3.loop();

  // ------------- Button 1 -------------------------------------------------  
   if(button1.isPressed()){
     // we do nothing on button press !
      Serial.println("Button 1");
      btn1State = !btn1State;
      selfSend = 1;
      if (btn1State == 1){
        lv_obj_set_style_local_bg_color(screenLED[2],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_LIME);
        lv_obj_set_style_local_border_color(screenLED[2], LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
        lv_obj_set_style_local_text_color(btnlabel[2], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
        redval = 300;
        snprintf (msg, MSG_BUFFER_SIZE, "%d", redval); // red 
        client.publish("SWI13651033/Brightness1",msg);
        delay(200);
        greenval = 100;
        snprintf (msg, MSG_BUFFER_SIZE, "%d", greenval); // green 
        client.publish("SWI13651033/Brightness2",msg);
      } else {
        lv_obj_set_style_local_bg_color(screenLED[2],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_GREEN);
        lv_obj_set_style_local_border_color(screenLED[2], LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
        lv_obj_set_style_local_text_color(btnlabel[2], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
        redval = 0;
        snprintf (msg, MSG_BUFFER_SIZE, "%d", redval); // red 
        client.publish("SWI13651033/Brightness1",msg);
        delay(200);
        greenval = 0;
        snprintf (msg, MSG_BUFFER_SIZE, "%d", greenval); // green 
        client.publish("SWI13651033/Brightness2",msg);
      }
      selfSend = 0;
   }
  if(button1.isReleased()){
    // Nope, nothing here !!
  }
  // ------------- Button 2 -------------------------------------------------  
   if(button2.isPressed()){
      Serial.println("Button 2");
      btn2State = !btn2State;
      snprintf (msg, MSG_BUFFER_SIZE, "%d", btn2State); // copy payload value into msg buffer
      client.publish("SWI13651033/Switch2",msg);
      selfSend = 1;
      if (btn2State ==1){
        lv_obj_set_style_local_bg_color(screenLED[1],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_LIME);
        lv_obj_set_style_local_border_color(screenLED[1], LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
        lv_obj_set_style_local_text_color(btnlabel[1], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
      } else {
        lv_obj_set_style_local_bg_color(screenLED[1],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_GREEN);
        lv_obj_set_style_local_border_color(screenLED[1], LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
        lv_obj_set_style_local_text_color(btnlabel[1], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);        
      }
      selfSend = 0;
   }
/*
        // MQTT messages for the LED strips in living room
        snprintf (msg, MSG_BUFFER_SIZE, "%d", 256); // copy payload value into msg buffer
        client.publish("DIM12967876/Brightness1",msg);
        snprintf (msg, MSG_BUFFER_SIZE, "%d", 256); // copy payload value into msg buffer
        client.publish("DIM12967876/Brightness2",msg);

        snprintf (msg, MSG_BUFFER_SIZE, "%d", 0); // copy payload value into msg buffer
        client.publish("DIM12967876/Brightness1",msg);
        snprintf (msg, MSG_BUFFER_SIZE, "%d", 0); // copy payload value into msg buffer
        client.publish("DIM12967876/Brightness2",msg);
*/
   if(button2.isReleased()){
    // Nope, nothing here !!
   }
  // ------------- Button 3 -------------------------------------------------  
   if(button3.isPressed()){
      Serial.println("Button 3");
      selfSend = 1;
      btn3State = !btn3State;
      snprintf (msg, MSG_BUFFER_SIZE, "%d", btn3State); // copy payload value into msg buffer
      client.publish("SWI13651033/Switch1",msg);
      selfSend = 1;
      if (btn3State ==1){
        lv_obj_set_style_local_bg_color(screenLED[0],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_LIME);
        lv_obj_set_style_local_border_color(screenLED[0], LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
        lv_obj_set_style_local_text_color(btnlabel[0], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
      } else {
        lv_obj_set_style_local_bg_color(screenLED[0],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_GREEN);
        lv_obj_set_style_local_border_color(screenLED[0], LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
        lv_obj_set_style_local_text_color(btnlabel[0], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);        
      }
      selfSend = 0;
   }
   if(button3.isReleased()){
    // Nope, nothing here !!
   }     
}