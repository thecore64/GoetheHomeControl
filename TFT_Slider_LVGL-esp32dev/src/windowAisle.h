#ifndef windowAisle_h_included
  #define windowAisle_h_included

// contains window and all sliders for aisle LED control

#include <lvgl.h>
#include <globals.h>

// start values for R and G when calling the window
int redval = 570;
int greenval = 256;

lv_obj_t * sliderred;
lv_obj_t * slidergreen;
lv_obj_t * winAisle;

void lv_winAisle_close_event_cb(lv_obj_t * btn, lv_event_t event)
{
  if(event == LV_EVENT_RELEASED) {

    lv_obj_t * win = lv_win_get_from_btn(btn); 

    lv_obj_del(win);
  }
}

void sliderred_event_cb(lv_obj_t * slider, lv_event_t event)
{  
  if(event == LV_EVENT_VALUE_CHANGED) {
      static char buf[5];     /* max 4 bytes  for number plus 1 null terminating byte */
      //lv_label_set_text(slider_label, buf);               /*Refresh the text*/
      snprintf (msg, MSG_BUFFER_SIZE, "%d", lv_slider_get_value(sliderred)); // copy payload value into msg buffer
      client.publish("SWI13651033/Brightness1",msg);
      Serial.println();
      Serial.print("Slider Red: "); Serial.print(buf); Serial.print(" "); 
      Serial.print(msg); Serial.println();
  }
}

void slidergreen_event_cb(lv_obj_t * slider, lv_event_t event)
{
  if(event == LV_EVENT_VALUE_CHANGED) {
      static char buf[5];        /* max 4 bytes  for number plus 1 null terminating byte */
      snprintf (msg, MSG_BUFFER_SIZE, "%d", lv_slider_get_value(slider)); // copy payload value into msg buffer
      client.publish("SWI13651033/Brightness2",msg);
      Serial.println();
      Serial.print("Slider Green: "); Serial.print(buf); Serial.print(" "); 
      Serial.print(msg); Serial.println();
  }
}

static void btnLED_event_handler_cb(lv_obj_t * obj, lv_event_t event){
    if(event == LV_EVENT_RELEASED) {
      // set both LED lines, red and green, to 50% brightness
        snprintf (msg, MSG_BUFFER_SIZE, "%d", 0); // copy payload value into msg buffer
        client.publish("SWI13651033/Brightness1",msg);
        snprintf (msg, MSG_BUFFER_SIZE, "%d", 0); // copy payload value into msg buffer
        client.publish("SWI13651033/Brightness2",msg);
    //lv_obj_t * win = lv_win_get_from_btn(winAisle); 
    lv_obj_del(winAisle);
    } 
}

void windowAisleLED(void){
  winAisle = lv_win_create(lv_scr_act(), NULL);
  lv_win_set_title(winAisle, "LED Gang"); //Set the title

  lv_obj_t * Aisleclose_btn = lv_win_add_btn(winAisle, LV_SYMBOL_CLOSE);  /*Add close button and use built-in close action*/

  lv_obj_set_event_cb(Aisleclose_btn, lv_winAisle_close_event_cb);
  //lv_win_add_btn(win, LV_SYMBOL_SETTINGS);        /*Add a setup button*/ 

  /* Create a slider */
  sliderred = lv_slider_create(winAisle, NULL);
  // first parameter of this function determines which screen or window the control belongs to,
  // if it is the wrong one, the object will not be destroyed by deleting the window
  lv_obj_set_event_cb(sliderred, sliderred_event_cb);         //Assign an event function
  lv_obj_set_width(sliderred, screenWidth-70);                        /*Set the width*/
  lv_obj_set_height(sliderred, 40);
  lv_obj_align(sliderred, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 20);    /*Align to the center of the parent (screen)*/
  lv_slider_set_range(sliderred, 0, 1023);
  lv_slider_set_value(sliderred, redval, LV_ANIM_OFF);
  // LV_SLIDER_PART_xx determines which part of the slider we want to change color
  // BG background color, KNOB of course, 
  // INDIC is the part which gets filled when moving
  lv_obj_set_style_local_bg_color(sliderred,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
  lv_obj_set_style_local_bg_color(sliderred,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_RED);
  lv_obj_set_style_local_bg_color(sliderred, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_RED);

  slidergreen = lv_slider_create(winAisle, NULL);
  lv_obj_set_event_cb(slidergreen, slidergreen_event_cb);         //Assign an event function
  // first parameter of this function determines which screen or window the control belongs to,
  // if it is the wrong one, the object will not be destroyed by deleting the window
  lv_obj_set_width(slidergreen, screenWidth-70);                        //Set the width
  lv_obj_set_height(slidergreen, 40);
  lv_obj_align(slidergreen, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 150);    //Align to the center of the parent (screen)
  lv_slider_set_range(slidergreen, 0, 1023);
    lv_slider_set_value(slidergreen, greenval, LV_ANIM_OFF);
  // LV_SLIDER_PART_xx determines which part of the slider we want to change color
  // BG background color, KNOB of course, 
  // INDIC is the part which gets filled when moving
  lv_obj_set_style_local_bg_color(slidergreen,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
  lv_obj_set_style_local_bg_color(slidergreen,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_LIME);
  lv_obj_set_style_local_bg_color(slidergreen, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_LIME);

  lv_obj_t * btnLED = lv_btn_create(winAisle, NULL); 
  lv_obj_set_event_cb(btnLED, btnLED_event_handler_cb); // cb = callback, set the buttons callback function
  lv_obj_align(btnLED, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 80);
  lv_btn_set_fit2(btnLED, LV_FIT_NONE, LV_FIT_TIGHT);

  lv_obj_t * btnLEDlabel = lv_label_create(btnLED, NULL);
  lv_label_set_text(btnLEDlabel, "Off");
  
}

#endif  