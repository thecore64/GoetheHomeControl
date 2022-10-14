#include <Arduino.h>
#include <lvgl.h>
#include <lcd_backlight.hpp>
#include <globals.h>
#include <TFT_eSPI.h>
#include <ezButton.h>
#include <PubSubClient.h>
#include "lv_demo_stress.h"
#include <rpcWiFi.h>

// WiFi credentials
const char *SSID = "TheCore";
const char *PASSWORD = "M1n0tauru5";
const char* mqtt_server = "192.168.0.5";

WiFiClient SAMClient;
PubSubClient client(SAMClient);

//************ define buttons ****************************************************
ezButton buttonUp(WIO_5S_UP);  
ezButton buttonDown(WIO_5S_DOWN); 
ezButton buttonLeft(WIO_5S_LEFT);
ezButton buttonRight(WIO_5S_RIGHT); 
ezButton buttonCenter(WIO_5S_PRESS);
ezButton buttonA(WIO_KEY_A);
ezButton buttonB(WIO_KEY_B);
ezButton buttonC(WIO_KEY_C);
//********************************************************************************

#define LVGL_TICK_PERIOD 5

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
static LCDBackLight backLight;
static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];

#if USE_LV_LOG != 0
/* Serial debugging */
void my_print(lv_log_level_t level, const char * file, uint32_t line, const char * dsc)
{

  Serial.printf("%s@%d->%s\r\n", file, line, dsc);
  delay(100);
}
#endif

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
  blCounter ++;
}

/* Reading input device (simulated encoder here) */
bool read_encoder(lv_indev_drv_t * indev, lv_indev_data_t * data)
{
  static int32_t last_diff = 0;
  int32_t diff = 0; /* Dummy - no movement */
  int btn_state = LV_INDEV_STATE_REL; /* Dummy - no press */

  data->enc_diff = diff - last_diff;;
  data->state = btn_state;

  last_diff = diff;

  return false;
}

void reconnect() {
  // Loop until we're reconnected
  tft.setCursor(12,85);
  tft.print("connecting MQTT server...");
  tft.setCursor(12,105);

  while (!client.connected()) {

#ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
#endif

    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {

      subscribe_str = "SWI6233300/getState"; //status message switcj
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);

      subscribe_str = "Switch2267261/State";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);

      subscribe_str = "Switch8745923/State1";
      subscribe_str.toCharArray(subscribe_buffer,subscribe_str.length()+1);
      client.subscribe(subscribe_buffer);

      subscribe_str = "Switch8745923/State2";
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
      Serial.println("Subscribed to MQTT messages !");
    } else {  
      // Wait 2 seconds before retrying
      delay(2000);
      mqtt_reconnectcnt ++;
      if (mqtt_reconnectcnt > 10){     
       //ESP.restart();
      }
    }
  }
  tft.print("MQTT connection successful !");
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
    if (activeWindow == 0){
      if (atof(c_val) == 0){
        lv_obj_set_style_local_bg_color(labLED[0],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_GREEN);
        labLEDstate[btn_cnt] = 0; 
      } else {
        lv_obj_set_style_local_bg_color(labLED[0],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_LIME);
        labLEDstate[btn_cnt] = 1;
      }
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
    if (activeWindow == 1){
      snprintf(thBuffer, MSG_BUFFER_SIZE, "#FF0000 %.1f°C#", temp);
      lv_label_set_text(tempValue, thBuffer);
    }  
    if (activeWindow == 0){
      //Serial.println("temp update win 0");
      lv_label_set_recolor(mainTempLabel, true);
      snprintf(thBuffer, MSG_BUFFER_SIZE, "#FF0000 %.1f°C#", temp);
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
    if (activeWindow == 1){
      snprintf(thBuffer, MSG_BUFFER_SIZE, "#FF0000 %.1f °C#", mintemp);
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
    if (activeWindow == 1){
      snprintf(thBuffer, MSG_BUFFER_SIZE, "#FF0000 %.1f °C#", maxtemp);
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
    if (activeWindow == 1){
      snprintf(thBuffer, MSG_BUFFER_SIZE, "#0000FF %.1f %%#", hum);
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
    if (activeWindow == 1){
      snprintf(thBuffer, MSG_BUFFER_SIZE, "#0000FF %.1f %%#", minhum);
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
    if (activeWindow == 1){
      snprintf(thBuffer, MSG_BUFFER_SIZE, "#0000FF %.1f %%#", maxhum);
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

void drawWeatherman(){
  txt = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(txt, true);
  lv_label_set_text(txt, "#FFFFFF Weatherman #");
  //lv_label_set_text(txt, "Weatherman");
  lv_obj_align(txt, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 3);
  lv_obj_set_style_local_text_font(txt, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_26);

// lv_label_set_text_fmt(label, "Value: %d", 15)
//---------- temperature ----------------------------------------------------------

  static lv_style_t lblstyle;
  lv_style_init(&lblstyle);
  lv_style_set_bg_color(&lblstyle,LV_STATE_DEFAULT,LV_COLOR_CYAN);
  lv_style_set_bg_opa(&lblstyle, LV_STATE_DEFAULT, LV_OPA_COVER);
  lv_style_set_bg_grad_color(&lblstyle, LV_STATE_DEFAULT, LV_COLOR_BLUE);
  lv_style_set_bg_grad_dir(&lblstyle, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  lv_style_set_text_font(&lblstyle, LV_STATE_DEFAULT, &lv_font_montserrat_20);
  
  tempLabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(tempLabel, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  lv_label_set_text(tempLabel, "#FF0000 Temperature: #");
  lv_obj_align(tempLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 40);
  lv_obj_set_style_local_text_font(tempLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);

  tempValue = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(tempValue, true);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#FF0000 %.1f#", temp);
  lv_label_set_text(tempValue, thBuffer);
  lv_obj_align(tempValue, NULL, LV_ALIGN_IN_TOP_LEFT, 155, 35);
  lv_obj_set_style_local_text_font(tempValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_26);

  tempminLabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(tempminLabel, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  lv_label_set_text(tempminLabel, "#FF0000 min. Temperature: #");
  lv_obj_align(tempminLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 70);
  lv_obj_set_style_local_text_font(tempminLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

  tempminValue = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(tempminValue, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#FF0000 %.1f #", mintemp);
  lv_label_set_text(tempminValue, thBuffer);
  lv_obj_align(tempminValue, NULL, LV_ALIGN_IN_TOP_LEFT, 175, 70);
  lv_obj_set_style_local_text_font(tempminValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

  tempmaxLabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(tempmaxLabel, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  lv_label_set_text(tempmaxLabel, "#FF0000 max. Temperature: #");
  lv_obj_align(tempmaxLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 100);
  lv_obj_set_style_local_text_font(tempmaxLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

  tempmaxValue = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(tempmaxValue, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#FF0000 %.1f #", maxtemp);
  lv_label_set_text(tempmaxValue, thBuffer);
  lv_obj_align(tempmaxValue, NULL, LV_ALIGN_IN_TOP_LEFT, 175, 100);
  lv_obj_set_style_local_text_font(tempmaxValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

//---------- humidity ----------------------------------------------------------
  humLabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(humLabel, true);
  lv_label_set_text(humLabel, "#0000FF rel.Humidity: #");
  lv_obj_align(humLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 150);
  lv_obj_set_style_local_text_font(humLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
  
  humValue = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(humValue, true);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#0000FF %.1f #", hum);
  lv_label_set_text(humValue, thBuffer);
  lv_obj_align(humValue, NULL, LV_ALIGN_IN_TOP_LEFT, 155, 145);
  lv_obj_set_style_local_text_font(humValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_26);

  humminLabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(humminLabel, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  lv_label_set_text(humminLabel, "#0000FF min. Humidity: #");
  lv_obj_align(humminLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 180);
  lv_obj_set_style_local_text_font(humminLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

  humminValue = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(humminValue, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#0000FF %.1f #", minhum);
  lv_label_set_text(humminValue, thBuffer);
  lv_obj_align(humminValue, NULL, LV_ALIGN_IN_TOP_LEFT, 175, 180);
  lv_obj_set_style_local_text_font(humminValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

  hummaxLabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(hummaxLabel, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  lv_label_set_text(hummaxLabel, "#0000FF max. Humidity: #");
  lv_obj_align(hummaxLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 210);
  lv_obj_set_style_local_text_font(hummaxLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

  hummaxValue = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_recolor(hummaxValue, true);
  //lv_obj_add_style(tempLabel, LV_OBJ_PART_MAIN, &lblstyle);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#0000FF %.1f #", maxhum);
  lv_label_set_text(hummaxValue, thBuffer);
  lv_obj_align(hummaxValue, NULL, LV_ALIGN_IN_TOP_LEFT, 175, 210);
  lv_obj_set_style_local_text_font(hummaxValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);

  activeWindow = 1;
}

void drawMain(){
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
  lv_obj_align(labelIP, NULL, LV_ALIGN_IN_TOP_LEFT, 28, 218);  
  
  // create label to show temperature on main screen
  mainTempLabel = lv_label_create(lv_scr_act(), NULL);
  lv_obj_set_style_local_text_font(mainTempLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);
  lv_obj_set_style_local_text_color(mainTempLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
  lv_label_set_recolor(mainTempLabel, true);
  snprintf(thBuffer, MSG_BUFFER_SIZE, "#00FFFF %.1f°C#", temp);
  lv_label_set_text(mainTempLabel, thBuffer);
  lv_obj_align(mainTempLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 138, 218);  

  // create label to hold WiFi icon (fontawesome) ------------------------------------------------
  lv_obj_t * wifilabel = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(wifilabel, LV_SYMBOL_WIFI);
  lv_obj_set_style_local_text_color(wifilabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
  lv_obj_align(wifilabel, NULL, LV_ALIGN_IN_TOP_LEFT, 5, 220);  
  //----------------------------------------------------------------------------------------------- 
  // Create label for the top buttons on WIO
  lv_obj_t * label = lv_label_create(lv_scr_act(), NULL);
  // set text color
  lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
  // set opacity for the background color
  lv_obj_set_style_local_bg_opa(label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT,LV_OPA_COVER);
  // set background color
  lv_obj_set_style_local_bg_color(label,LV_LABEL_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_CYAN);
  
  // create label for the top buttons on WIO
  toplabel1 = lv_label_create(lv_scr_act(), NULL);
  lv_obj_set_style_local_text_font(toplabel1, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
  lv_obj_align(toplabel1, NULL, LV_ALIGN_IN_TOP_LEFT, 3, 4);  
  lv_label_set_text(toplabel1, "LAB");

/*
  lv_obj_t * toplabel2 = lv_label_create(lv_scr_act(), NULL);
  lv_obj_set_style_local_text_font(toplabel2, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_18);
  lv_obj_align(toplabel2, NULL, LV_ALIGN_IN_TOP_LEFT, 70, 4);  
  lv_label_set_text(toplabel2, "all OFF");
*/  
  lv_obj_set_style_local_text_font(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_24);
  lv_label_set_text(label, "Crystal Cave Lab's");
  lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, 25,21);
  
  int y = 48; // Y for the LED and button columns
  for (int cnt = 0; cnt < 6; cnt ++){
    menuItem[cnt] = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_size(menuItem[cnt], 195,25);
    lv_obj_align(menuItem[cnt], NULL, LV_ALIGN_IN_TOP_LEFT, 20, y);
    lv_obj_t * btnlabel6 = lv_label_create(menuItem[cnt], NULL);
    lv_obj_set_style_local_text_font(btnlabel6, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20);
    int len = btnLabels[cnt].length()+1;
    btnLabels[cnt].toCharArray(publish_buffer, len);
    lv_label_set_text(btnlabel6, publish_buffer);     
    y = y + 26;
  }
  // highlight first menu entry
  lv_btn_toggle(menuItem[0]);
  
  y = 48;
  for (int i = 0; i<6; i++){
    labLED[i] = lv_btn_create(lv_scr_act(), NULL);
    lv_obj_set_size(labLED[i], 60,25);
    lv_obj_align(labLED[i], NULL, LV_ALIGN_IN_TOP_LEFT, 220, y);    
    lv_obj_set_style_local_bg_color(labLED[i],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_GREEN);
    y = y+26;
  }
  activeWindow = 0;
}

void setupWiFi(){
 // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    while (WiFi.status() != WL_CONNECTED)
    {
      tft.setCursor(12,25);
      tft.setTextSize(2);
      tft.setTextColor(TFT_BLACK);
      tft.print("connecting to: "); tft.print(SSID);
      Serial.println("Connecting to WiFi..");
      WiFi.begin(SSID, PASSWORD);
      delay(500);
    }
    tft.setCursor(12,45);
    tft.print("connection successful !");
    tft.setCursor(12,65);
    tft.print(WiFi.localIP());
    delay(5000);
}

void buzzer(int del){
  for (int i = 0; i<del; i++){
  digitalWrite(WIO_BUZZER, HIGH);
  delayMicroseconds(130);
  digitalWrite(WIO_BUZZER, LOW);
  delayMicroseconds(130);
  }
}
void setup() {

  backLight.initialize();
  // --- five way joystick -------------------------
  pinMode(WIO_5S_UP, INPUT_PULLUP);
  pinMode(WIO_5S_DOWN, INPUT_PULLUP);
  pinMode(WIO_5S_LEFT, INPUT_PULLUP);
  pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
  pinMode(WIO_5S_PRESS, INPUT_PULLUP);
  // --- three buttons on top of WIO ---------------
  pinMode(WIO_KEY_A, INPUT_PULLUP);
  pinMode(WIO_KEY_B, INPUT_PULLUP);
  pinMode(WIO_KEY_C, INPUT_PULLUP);
  // -----------------------------------------------
  pinMode(WIO_LIGHT, INPUT); // analog light sensor
  pinMode(WIO_BUZZER, OUTPUT);

  Serial.begin(115200); /* prepare for possible serial debug */


  lv_init();

#if USE_LV_LOG != 0
  lv_log_register_print(my_print); /* register print function for debugging */
#endif

  tft.begin(); /* TFT init */
  tft.setRotation(3); /* Landscape orientation */

  if (digitalRead(WIO_KEY_A) == 0){
    tft.fillScreen(TFT_RED);
    tft.setCursor(5,25);
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(5,40);
    tft.print("resetting WiFi data..");
    //wm.resetSettings();
    tft.setCursor(5,55);
    tft.print("Captive portal active");
    tft.setCursor(5,70);
    tft.print("AUTOCONNECT AP");
    delay(2000);
  }

  setupWiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);

  /*Initialize the display*/
  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 320;
  disp_drv.ver_res = 240;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the touch pad*/
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_ENCODER;
  indev_drv.read_cb = read_encoder;
  lv_indev_drv_register(&indev_drv);

  drawMain();

  backLight.setBrightness(254);
}


void loop() {

if (blCounter > 5000){
  blCounter = 0;
  backLight.setBrightness(0);
}  
if (!client.connected()) {
    reconnect();
  }
  client.loop();

  lv_tick_handler();
  lv_task_handler(); /* let the GUI do its work */
  //delay(5);

   int light = analogRead(WIO_LIGHT);
  
  // ************* handle the buttons **************************
  buttonA.setDebounceTime(50);

  buttonUp.loop();
  buttonDown.loop();
  buttonLeft.loop();
  buttonRight.loop();
  buttonCenter.loop();
  buttonA.loop();
  buttonB.loop();
  buttonC.loop();

  if (buttonC.isPressed()){
    blCounter = 0;
    backLight.setBrightness(200);
    buzzer(800);
    if (labAllState == 0){
      snprintf (msg, MSG_BUFFER_SIZE, "%d", 1); // copy payload value into msg buffer
      client.publish("SWI6233300/setstate",msg);
      lv_obj_set_style_local_bg_color(labLED[0],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_LIME);
      snprintf (msg, MSG_BUFFER_SIZE, "%d", 1); // copy payload value into msg buffer
      client.publish("Switch2267261/setstate",msg);
      lv_obj_set_style_local_bg_color(labLED[1],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_LIME);
      snprintf (msg, MSG_BUFFER_SIZE, "%d", 1); // copy payload value into msg buffer
      client.publish("Switch8745923/switch1",msg);
      lv_obj_set_style_local_bg_color(labLED[2],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_LIME);
      snprintf (msg, MSG_BUFFER_SIZE, "%d", 1); // copy payload value into msg buffer
      client.publish("Switch8745923/switch2",msg);
      lv_obj_set_style_local_bg_color(labLED[3],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_LIME);    
      for (int i=0; i<4; i++) labLEDstate[i]=1;
      labAllState = 1;
      lv_label_set_text(toplabel1, "LAB OFF");
    } else {
        snprintf (msg, MSG_BUFFER_SIZE, "%d", 0); // copy payload value into msg buffer
        client.publish("SWI6233300/setstate",msg);
        lv_obj_set_style_local_bg_color(labLED[0],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_GREEN); 
        snprintf (msg, MSG_BUFFER_SIZE, "%d", 0); // copy payload value into msg buffer
        client.publish("Switch2267261/setstate",msg);
        lv_obj_set_style_local_bg_color(labLED[1],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_GREEN);
        snprintf (msg, MSG_BUFFER_SIZE, "%d", 0); // copy payload value into msg buffer
        client.publish("Switch8745923/switch1",msg);
        lv_obj_set_style_local_bg_color(labLED[2],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_GREEN);
        snprintf (msg, MSG_BUFFER_SIZE, "%d", 0); // copy payload value into msg buffer
        client.publish("Switch8745923/switch2",msg);
        lv_obj_set_style_local_bg_color(labLED[3],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_GREEN);    
        for (int i=0; i<4; i++) labLEDstate[i]=0;
        labAllState = 0; 
        lv_label_set_text(toplabel1, "LAB ON");             
    }
  }
  if (buttonA.isPressed()){

  }
  if (buttonA.isReleased()){
      blCounter = 0;
      backLight.setBrightness(200);
      switch (activeWindow){
        case 0:
          Serial.println(" act. win 0, call weather");
          static lv_style_t style;
          prev = lv_scr_act(); // save previously active screen
          weather_scr = lv_cont_create(NULL, NULL);
          lv_obj_add_style(weather_scr, LV_STATE_DEFAULT, &style);
          lv_disp_load_scr(weather_scr);
          // Make a gradient
          lv_style_set_bg_opa(&style, LV_STATE_DEFAULT, LV_OPA_COVER);
          lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_BLUE);
          lv_style_set_bg_grad_color(&style, LV_STATE_DEFAULT, LV_COLOR_ORANGE);
          lv_style_set_bg_grad_dir(&style, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
          //*Shift the gradient to the bottom
          lv_style_set_bg_main_stop(&style, LV_STATE_DEFAULT, 43);
          lv_style_set_bg_grad_stop(&style, LV_STATE_DEFAULT, 212);
          //lv_scr_load(scr2);      
          //lv_obj_del(prev);
          //windowWall();
          drawWeatherman();
          lv_disp_load_scr(weather_scr);
        break;
        case 1:
          Serial.println(" act. win 1, call main");
          lv_obj_del(weather_scr);
          lv_disp_load_scr(prev);
          activeWindow = 0;
        break;
        default:
        break;
      }  
  }

  if (buttonB.isPressed()){
    blCounter = 0;
    backLight.setBrightness(200); 
    buzzer(300);
  }

  if (buttonCenter.isPressed()){
    blCounter = 0;
    backLight.setBrightness(200);
    buzzer(800);
  }
  if (buttonCenter.isReleased()){

    labLEDstate[btn_cnt] = !labLEDstate[btn_cnt];
    if (labLEDstate[btn_cnt] == 1){
      lv_obj_set_style_local_bg_color(labLED[btn_cnt],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_LIME);
    } else {
      lv_obj_set_style_local_bg_color(labLED[btn_cnt],LV_BTN_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_GREEN);
    }
    switch (btn_cnt){
      case 0:
          snprintf (msg, MSG_BUFFER_SIZE, "%d", labLEDstate[btn_cnt]); // copy payload value into msg buffer
          client.publish("SWI6233300/setstate",msg);  
      break;        
      case 1:
          snprintf (msg, MSG_BUFFER_SIZE, "%d", labLEDstate[btn_cnt]); // copy payload value into msg buffer
          client.publish("Switch2267261/setstate",msg);
      break;
      case 2:
          snprintf (msg, MSG_BUFFER_SIZE, "%d", labLEDstate[btn_cnt]); // copy payload value into msg buffer
          client.publish("Switch8745923/switch1",msg);
      break;
      case 3:
          snprintf (msg, MSG_BUFFER_SIZE, "%d", labLEDstate[btn_cnt]); // copy payload value into msg buffer
          client.publish("Switch8745923/switch2",msg);
      break;
      case 4:
          snprintf (msg, MSG_BUFFER_SIZE, "%d", labLEDstate[btn_cnt]); // copy payload value into msg buffer
          client.publish("SWI14343632/setstate",msg);
      break;
    }
  }

  if (buttonUp.isPressed()){
    blCounter = 0;
    backLight.setBrightness(200);
    buzzer(300);
  }
  if (buttonUp.isReleased()){
    lv_btn_toggle(menuItem[btn_cnt]);
    btn_cnt--;
    if (btn_cnt < 0) btn_cnt = 5;
    lv_btn_toggle(menuItem[btn_cnt]);
  }
  if (buttonDown.isPressed()){
    blCounter = 0;
    backLight.setBrightness(200);  
    buzzer(300);  
  }
  if (buttonDown.isReleased()){
    lv_btn_toggle(menuItem[btn_cnt]);
    btn_cnt++;
    if (btn_cnt > 5) btn_cnt = 0;
    lv_btn_toggle(menuItem[btn_cnt]);
  }  
  
}