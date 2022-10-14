#include <ArduinoOTA.h>
#include <TFT_eSPI.h>

extern TFT_eSPI tft;
extern int otaRunning;
extern int otaProgress;
extern int old_otaProgress;
extern String otaProgressStr;

void setupOTA(){
    
  //ArduinoOTA.setPort(8266);

  ArduinoOTA.onStart([]() {
  //  otaRunning = 1;
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

  ledcWrite(0,128); // switch on TFT backlight  
  
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE); // Background is not defined so it is transparent

  tft.setCursor (12, 15);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(3);
  tft.print("OTA Flash...");

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    otaRunning = 0;
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    otaProgress = progress/(total/100);

    tft.setTextColor(TFT_BLACK); // Background is not defined so it is transparent
    tft.setCursor(32,55);
    otaProgressStr = String(old_otaProgress)+" % finished";
    tft.print(otaProgressStr);

    tft.setCursor (32, 55);
    tft.setTextColor(TFT_YELLOW);
    otaProgressStr = String(otaProgress)+" % finished";
    tft.print(otaProgressStr);
    old_otaProgress = otaProgress;

  });
  ArduinoOTA.onError([](ota_error_t error) {
    tft.setTextColor(TFT_RED); // Background is not defined so it is transparent
    tft.setCursor(32,85);
    tft.print("OTA failed !");
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}
