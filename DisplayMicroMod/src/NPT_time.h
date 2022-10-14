#ifndef NPT_time_h
  #define NPT_time_h
#endif  

//------ credentials  for NTP server
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

char timeHour[3];
char timeMinute[3];
char timeSecond[3];
char timeinfo_buffer[25];

void getTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  //snprintf (timeinfo_buffer, MSG_BUFFER_SIZE, "%B %d %Y %H:%M:%S", &timeinfo);

  strftime(timeHour,3, "%H", &timeinfo);
  strftime(timeMinute,3, "%M", &timeinfo);
  strftime(timeSecond,3, "%S", &timeinfo);
  snprintf (timeinfo_buffer, 25, "%s:%s:%s", timeHour, timeMinute, timeSecond);
  Serial.println(&timeinfo,  "%B %d %Y %H:%M:%S");
  Serial.println(timeinfo_buffer);
}
