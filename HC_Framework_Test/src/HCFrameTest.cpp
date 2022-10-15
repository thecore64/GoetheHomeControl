#define DEBUG

#include <Arduino.h>
#include <HC_Frame.h>

iotdevice myDevice;

IPAddress ip(192,168,0,10); // SWI is first in name, so we choose SWI address range
 // gateway and subnet stay usually the same for all modules !!
IPAddress gateway(192,168,0,1);  // Your router IP
IPAddress subnet(255,255,255,0); // Subnet mask
IPAddress retIP;

// WiFi credentials ------------------------------------------------
const char* ssid = "TheCore";
const char* password = "M1n0tauru5";
const char* mqtt_server = "192.168.0.5";

WiFiClient espClient;
PubSubClient client(espClient);

void defineDevice(){
  // define device type and name -----------------------------------
  myDevice.type = "SWIDIM";
  myDevice.freeName = "Aisle";
  // define switch and dim channels --------------------------------
  myDevice.numSwitchChannels = 2;
  myDevice.numPWMChannels = 2;
  // define the messages the device is subscibing to control it ----
  myDevice.numSubscribeMessages = 4;
  myDevice.subscribeMessages[0] = myDevice.type+myDevice.freeName+"/Switch1";
  myDevice.subscribeMessages[1] = myDevice.type+myDevice.freeName+"/Switch2";
  myDevice.subscribeMessages[2] = myDevice.type+myDevice.freeName+"/Brightness1";
  myDevice.subscribeMessages[3] = myDevice.type+myDevice.freeName+"/Brightness2";
  // define the device "state" messages to be published -----------------
  myDevice.publishStatusMessages[0] = myDevice.type+myDevice.freeName+"/stateSwitch1";
  myDevice.publishStatusMessages[1] = myDevice.type+myDevice.freeName+"/stateSwitch2";
  myDevice.publishValueMessages[0] = myDevice.type+myDevice.freeName+"/stateBrightness1";
  myDevice.publishValueMessages[1] = myDevice.type+myDevice.freeName+"/stateBrightness2";
}

void setupWiFi() {
  // connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  retIP = WiFi.localIP();

  #ifdef DEBUG
    Serial.print("\n\rconnected to:"); Serial.print(ssid); Serial.print("IP: "); Serial.println(retIP);
  #endif  
}

void setup() {
  setupWiFi();

  HC_subscribeAll(myDevice, client);

}

void loop() {
  // put your main code here, to run repeatedly:
}