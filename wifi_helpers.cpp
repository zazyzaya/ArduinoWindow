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

void parse_sunrise_timestamp(string tstr, int* times) {
    string timestr = tstr.substr(0, 2); 
    cout << timestr << ":"; 
    int time_int = stoi(timestr); 
    times[SUN_HOUR] = time_int; 

    timestr = tstr.substr(3,2); 
    cout << timestr << ":"; 
    time_int = stoi(timestr); 
    times[SUN_MIN] = time_int; 

    timestr = tstr.substr(6,2); 
    cout << timestr << "\n"; 
    time_int = stoi(timestr);
    times[SUN_SEC] = time_int; 
}

void parse_sunrise_time(string resp, int** times) {
    string substr; 
    string timestr; 
    int time_int; 

    // Sunrise 
    size_t idx = resp.find("<sunrise>"); 
    substr = resp.substr(idx+9, 8); 
    parse_timestamp(substr, times[SUNRISE]); 

    // Civil sunrise 
    idx = resp.find("<civil>"); 
    substr = resp.substr(idx+7, 8); 
    parse_timestamp(substr, times[CIVIL_SUNRISE]); 

    // Chop out first half so string::find hits the sunset times 
    resp = resp.substr(idx+6); 
    
    // Sunset 
    idx = resp.find("<sunset>"); 
    substr = resp.substr(idx+8, 8); 
    parse_timestamp(substr, times[SUNSET]); 

    // Civil Sunset
    idx = resp.find("<civil>"); 
    substr = resp.substr(idx+7, 8); 
    parse_timestamp(substr, times[CIVIL_SUNSET]); 
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

void get_sunrise_times(int** times) {
  char line[80];
  char suntimes_http[30];
  sprintf(suntimes_http, "/sun/%s/%s/", LAT, LONG);
  
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

    resp = read_response();
    return parse_sunrise_times(resp, times);

  }
  else {
    Serial.println("Couldn't connect");
    delay(1000);
    Serial.println("Trying again"); 
    get_sunrise_time(times);
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
