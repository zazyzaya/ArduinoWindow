#include "Arduino.h"
#include "WiFiS3.h" 
#include <UnixTime.h>
#include "secrets.h"

#include "wifi_helpers.h"

WiFiClient client;
char user_agent[] = "User-Agent: ArduinoWifi/1.1";
char server[] = "new.earthtools.org";

String read_response() {
  Serial.println("Reading response..."); 
  String resp = "";

  uint32_t received_data_num = 0;
  while (client.available()) {
    String result = client.readString(); 
    resp += result; 
    Serial.println(result);
  }

  return resp;
}

void sunrise_timestamp(String tstr, int* times) {
    String timestr = tstr.substring(0, 2); 
    int time_int = timestr.toInt();
    times[SUN_HOUR] = time_int; 

    timestr = tstr.substring(3,5); 
    time_int = timestr.toInt();
    times[SUN_MIN] = time_int; 

    timestr = tstr.substring(6,8); 
    time_int = timestr.toInt();
    times[SUN_SEC] = time_int; 
}

void parse_sunrise_times(String resp, int[N_SUNTIMES] tstamps, int day, int month, int year) {
    String substr; 
    String timestr; 
    int time_int; 
    int times[3]; 
    char buff[80];

    int tz; 
    size_t st = resp.indexOf("<timezone>"); // len == 10  
    size_t en = resp.indexOf("</timezone>"); 
    substr = resp.substring(st+10, en); 
    tz = substr.toInt(); 
    UnixTime ts(tz); 

    // Sunrise 
    size_t idx = resp.indexOf("<sunrise>"); 
    substr = resp.substring(idx+9, idx+9+8); 
    sunrise_timestamp(substr, times); 
    ts.setDateTime(year,month,day,times[SUN_HOUR],times[SUN_MIN],times[SUN_SEC]); 
    tstamps[SUNRISE] = ts.getUnix(); 
    sprintf(
      buff, "Sunrise: %d,%d,%d,%d,%d,%d (%d)", 
      year,month,day,times[SUN_HOUR],times[SUN_MIN],times[SUN_SEC], tstamps[SUNRISE]
    );
    Serial.println(buff)

    // Nautical sunrise 
    idx = resp.indexOf("<nautical>"); 
    substr = resp.substring(idx+10, idx+10+8); 
    sunrise_timestamp(substr, times); 
    ts.setDateTime(year,month,day,times[SUN_HOUR],times[SUN_MIN],times[SUN_SEC]); 
    tstamps[NAUT_SUNRISE] = ts.getUnix(); 
    sprintf(
      buff, "Naut SR: %d,%d,%d,%d,%d,%d (%d)", 
      year,month,day,times[SUN_HOUR],times[SUN_MIN],times[SUN_SEC], tstamps[NAUT_SUNRISE]
    );
    Serial.println(buff)

    // Astro sunrise 
    idx = resp.indexOf("<astronomical>"); // len 14 
    substr = resp.substring(idx+14, idx+14+8); 
    sunrise_timestamp(substr, times); 
    ts.setDateTime(year,month,day,times[SUN_HOUR],times[SUN_MIN],times[SUN_SEC]); 
    tstamps[ASTRO_SUNRISE] = ts.getUnix(); 
    sprintf(
      buff, "Astro SR: %d,%d,%d,%d,%d,%d (%d)", 
      year,month,day,times[SUN_HOUR],times[SUN_MIN],times[SUN_SEC], tstamps[ASTRO_SUNRISE]
    );
    Serial.println(buff)

    // Chop out first half so string::find hits the sunset times 
    resp = resp.substring(idx+6); 
    
    // Sunset 
    idx = resp.indexOf("<sunset>"); 
    substr = resp.substring(idx+8, idx+8+8); 
    sunrise_timestamp(substr, times); 
    ts.setDateTime(year,month,day,times[SUN_HOUR],times[SUN_MIN],times[SUN_SEC]); 
    tstamps[SUNSET] = ts.getUnix(); 
    sprintf(
      buff, "Sunset: %d,%d,%d,%d,%d,%d (%d)", 
      year,month,day,times[SUN_HOUR],times[SUN_MIN],times[SUN_SEC], tstamps[SUNSET]
    );
    Serial.println(buff)

    // Nautical Sunset
    idx = resp.indexOf("<nautical>"); 
    substr = resp.substring(idx+10, idx+10+8); 
    sunrise_timestamp(substr, times[NAUT_SUNSET]); 
    ts.setDateTime(year,month,day,times[SUN_HOUR],times[SUN_MIN],times[SUN_SEC]); 
    tstamps[NAUT_SUNSET] = ts.getUnix(); 
    sprintf(
      buff, "Naut SS: %d,%d,%d,%d,%d,%d (%d)", 
      year,month,day,times[SUN_HOUR],times[SUN_MIN],times[SUN_SEC], tstamps[NAUT_SUNSET]
    );
    Serial.println(buff)

    // Astro sunset 
    idx = resp.indexOf("<astronomical>"); 
    substr = resp.substring(idx+14, idx+14+8); 
    sunrise_timestamp(substr, times); 
    ts.setDateTime(year,month,day,times[SUN_HOUR],times[SUN_MIN],times[SUN_SEC]); 
    tstamps[ASTRO_SUNSET] = ts.getUnix(); 
    sprintf(
      buff, "Astro SS: %d,%d,%d,%d,%d,%d (%d)", 
      year,month,day,times[SUN_HOUR],times[SUN_MIN],times[SUN_SEC], tstamps[ASTRO_SUNSET]
    );
    Serial.println(buff)
}

int parse_cur_time(String response) {
  UnixTime tstamp(0);
  int idx = response.indexOf("<utctime>") + 9;
  String interesting = response.substring(idx, idx+20);

  int year,month,day,hour,min,sec; 
  year = response.substring(idx,idx+4).toInt(); 
  idx += 5; 

  month = response.substring(idx,idx+2).toInt();
  idx += 3;
  day = response.substring(idx,idx+2).toInt();
  idx += 3;
  hour = response.substring(idx,idx+2).toInt();
  idx += 3;  
  min = response.substring(idx,idx+2).toInt();
  idx += 3; 
  sec = response.substring(idx,idx+2).toInt();
  idx += 3; 

  tstamp.setDateTime(year,month,day,hour,min,sec);
  int unix = tstamp.getUnix();

  char buff[80];
  sprintf(buff, "%d,%d,%d,%d,%d,%d (%d)", year,month,day,hour,min,sec, unix);
  Serial.println(buff);

  return unix;
}

void get_sunrise_times(int times[N_SUNTIMES], int day, int month, int year) {
  char line[80];
  char suntimes_http[40];
  sprintf(suntimes_http, "/sun/%s/%s/%d/%d/99/0", LAT, LONG, day, month);
  
  String resp; 

  if (client.connect(server, 80)) {
    Serial.println("connected successfully");
    Serial.println("~~~~~~~~~~");

    sprintf(line, "GET %s HTTP/1.1", suntimes_http);
    client.println(line); 
    Serial.println(line);

    sprintf(line, "Host: %s", server);
    client.println(line);
    Serial.println(line);

    client.println(user_agent);
    Serial.println(user_agent);

    client.println("Connection: close");
    Serial.println("Connection: close");

    client.println();
    Serial.println();

    delay(1000);
    resp = read_response();
    parse_sunrise_times(resp, times, day, month, year);
  } else {
    Serial.println("Couldn't connect");
    delay(1000);
    Serial.println("Trying again"); 
    get_sunrise_times(times, day, month, year);
  }
}

int get_cur_time() {
  char line[80]; 
  char curtime_http[30]; 
  sprintf(curtime_http, "/timezone/%s/%s", LAT, LONG);
  
  String resp;
  if (client.connect(server, 80)) {
    Serial.println("connected successfully");
    Serial.println("~~~~~~~~~~");

    sprintf(line, "GET %s HTTP/1.1", curtime_http);
    client.println(line); 
    Serial.println(line);

    sprintf(line, "Host: %s", server);
    client.println(line);
    Serial.println(line);

    client.println(user_agent);
    Serial.println(user_agent);

    client.println("Connection: close");
    Serial.println("Connection: close");

    long sent_time = millis();
    
    client.println();
    Serial.println();

    delay(1000);
    resp = read_response();

    int unix_date = parse_cur_time(resp);
    int elapsed = (int)(millis() - sent_time); 
    
    return unix_date+elapsed;
  }
  else {
    Serial.println("Couldn't connect");
    delay(1000);
    Serial.println("Trying again"); 
    get_cur_time();
  }
}
