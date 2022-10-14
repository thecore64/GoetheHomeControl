#ifndef windowWall_h_included
  #define windowWall_h_included

// contains window and all sliders for bay bridge picture wall LED control

#include <Arduino.h>
#include <lvgl.h>
#include <globals.h>
#include <stdio.h>

void lv_winWall_close_event_cb(lv_obj_t * btn, lv_event_t event)
{
  if(event == LV_EVENT_RELEASED) {

    lv_obj_t * win = lv_win_get_from_btn(btn); 

    lv_obj_del(win);
  }
}

void slider_event_cb(lv_obj_t * slider, lv_event_t event)
{
  if(event == LV_EVENT_VALUE_CHANGED) {
      static char buf[4];                                 /* max 3 bytes  for number plus 1 null terminating byte */
      //snprintf(buf, 4, "%u", lv_slider_get_value(slider));
      //lv_label_set_text(slider_label, buf);               /*Refresh the text*/
      snprintf (msg, MSG_BUFFER_SIZE, "%d", lv_slider_get_value(slider)); // copy payload value into msg buffer
      client.publish("DIM12967876/Brightness1",msg);
  }
}

void slider2_event_cb(lv_obj_t * slider, lv_event_t event)
{
  if(event == LV_EVENT_VALUE_CHANGED) {
      static char buf[4];                                 /* max 3 bytes  for number plus 1 null terminating byte */
      //snprintf(buf, 4, "%u", lv_slider_get_value(slider));
      snprintf (msg, MSG_BUFFER_SIZE, "%d", lv_slider_get_value(slider)); // copy payload value into msg buffer
      //lv_label_set_text(slider_label2, buf);               /*Refresh the text*/
      client.publish("DIM12967876/Brightness2",msg);
   
      Serial.println();
      Serial.println(buf);
  }
}

void windowWall(void){
  lv_obj_t * win = lv_win_create(lv_scr_act(), NULL);
  lv_win_set_title(win, "LED Streifen Wand"); //Set the title

  lv_obj_t * close_btn = lv_win_add_btn(win, LV_SYMBOL_CLOSE);  /*Add close button and use built-in close action*/

  lv_obj_set_event_cb(close_btn, lv_winWall_close_event_cb);
  //lv_win_add_btn(win, LV_SYMBOL_SETTINGS);        /*Add a setup button*/ 

  /*Add labels*/
  lv_obj_t * txt = lv_label_create(win, NULL);
  lv_label_set_text(txt, "Links");
  lv_obj_align(txt, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 5);

  lv_obj_t * txt2 = lv_label_create(win, NULL);
  lv_label_set_text(txt2, "Rechts");
  lv_obj_align(txt2, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 90);

  /* Create a slider */
  lv_obj_t * slider = lv_slider_create(win, NULL);
  // first parameter of this function determines which screen or window the control belongs to,
  // if it is the wrong one, the object will not be destroyed by deleting the window
  lv_obj_set_event_cb(slider, slider_event_cb);         //Assign an event function
  lv_obj_set_width(slider, screenWidth-40);                        /*Set the width*/
  lv_obj_set_height(slider, 30);
  lv_slider_set_range(slider, 0, 1023);
  lv_slider_set_value(slider, wall_brightness, LV_ANIM_ON);
  lv_obj_align(slider, NULL, LV_ALIGN_IN_TOP_LEFT, 28, 40);    /*Align to the center of the parent (screen)*/
  // LV_SLIDER_PART_xx determines which part of the slider we want to change color
  // BG background color, KNOB of course, 
  // INDIC is the part which gets filled when moving
  lv_obj_set_style_local_bg_color(slider,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
  lv_obj_set_style_local_bg_color(slider,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
  lv_obj_set_style_local_bg_color(slider, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_CYAN);

  lv_obj_t * slider2 = lv_slider_create(win, NULL);
  lv_obj_set_event_cb(slider2, slider2_event_cb);         //Assign an event function
  // first parameter of this function determines which screen or window the control belongs to,
  // if it is the wrong one, the object will not be destroyed by deleting the window
  lv_obj_set_width(slider2, screenWidth-40);                        /*Set the width*/
  lv_obj_set_height(slider2, 30);
  lv_slider_set_range(slider2, 0, 1023);
  lv_slider_set_value(slider2, wall_brightness, LV_ANIM_ON);
  lv_obj_align(slider2, NULL, LV_ALIGN_IN_TOP_LEFT, 28, 120);    /*Align to the center of the parent (screen)*/
  // LV_SLIDER_PART_xx determines which part of the slider we want to change color
  // BG background color, KNOB of course, 
  // INDIC is the part which gets filled when moving
  lv_obj_set_style_local_bg_color(slider2,LV_SLIDER_PART_BG,LV_STATE_DEFAULT,LV_COLOR_ORANGE);
  lv_obj_set_style_local_bg_color(slider2,LV_SLIDER_PART_INDIC,LV_STATE_DEFAULT,LV_COLOR_NAVY);
  lv_obj_set_style_local_bg_color(slider2, LV_SLIDER_PART_KNOB, LV_STATE_DEFAULT,LV_COLOR_CYAN);
}

#endif  