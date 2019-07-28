#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiClient.h>
#include <LiquidCrystal.h>
#include <WiFiUdp.h>
#include "src/arduino_secrets.h"
#include <stdlib.h>
#include <math.h>

//와이파이 연결
WiFiUDP Udp;
IPAddress hostIp_temp;
IPAddress hostIp_dust;
WiFiClient client_temp;
WiFiClient client_dust;
WiFiClient client_time;
uint8_t ret;
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
WiFiServer server(80);
unsigned int localPort = 2390;

//시간
const char* ntpServer = "kr.pool.ntp.org";
const long gmtOffset_sec = 32400;
const int daylightOffset_sec = 0;
const int NTP_PACKET_SIZE = 48;
IPAddress timeServer(106, 247, 248, 106);
byte packetBuffer[ NTP_PACKET_SIZE];
unsigned long epoch = 0;
bool is_first = true;
int time_connection_attempt = 0;
char buf[40];
char *time_cut = 0;
char *time_val[8] = {0};
int year;
int month;
int date;
int day;
int hour;
int minute;
int second;
const char *day_arr[7] = {"Sun", "Mon", "Tue", "Wen", "Thu", "Fri", "Sat"};
const char *month_arr[13] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
char DaysOfMonth[]={31,28,31,30,31,30,31,31,30,31,30,31};


//온도
double temp = 0.0;
String wt_temp = "";
bool tagInside = false;  // 태그 안쪽인지 바깥쪽인지 구별하는 변수
bool flagStartTag = false; // 스타트 태그인지 구별하는 변수
String currentTag = ""; // 현재 태그를 저장하기 위한 변수, 공백으로 초기화함
String currentData = ""; // 태그 사이의 컨텐츠를 저장하는 변수
String startTag = ""; // 현재 elements의 start tag 저장
String endTag = "";   // 현재 elements의 end tag 저장
String tempValue = "";
int button_state;


//LCD
LiquidCrystal lcd(12, 11, 2, 3, 4, 5);

//timer
unsigned long timeVal;

void setup() {
  Serial.begin(9600);
  delay(10);

  //스위치
  pinMode(8, INPUT);
  lcd.begin(16, 2);


  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  //LCD
  lcd.clear();
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print(ssid);

  WiFi.begin(ssid, password);

  Serial.println("");
  Serial.println("WiFi connected");

  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.print("Connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  delay(3000);
  //시간
  time_connectToServer();

  //온도
  temp_connectToServer();

  //미세먼지
  dust_connectToServer();
}

void loop() {
  timeVal = millis();
  int button_value = digitalRead(8);

  /****스위치****/

  if (button_value == HIGH) {
    Serial.println("input button");
    if (button_state == 0) button_state = 1;
    else if (button_state == 1) button_state = 2;
    else if (button_state == 2) button_state = 0;
  }
  else if (button_value == LOW) {}
  Serial.print("button_state = ");
  Serial.println(button_state);



  //시간
  if (button_state == 0) {

    time_connectToServer();
    lcd.clear();
    sprintf(buf, "%04d/%02d/%02d %s", year, month, date, day_arr[day]);
    lcd.print(buf);
    lcd.setCursor(0, 1);
    sprintf(buf, "%02d:%02d:%02d", hour, minute, second);
    lcd.print(buf);
    lcd.setCursor(0, 0);

  }


  //온도
  if (button_state == 1) {
    while (client_temp.connected()) {
      while (client_temp.available()) {
        char c = client_temp.read();
        if (c == '<') {
          tagInside = true;
        }
        if (tagInside) {
          currentTag += c;
        } else if (flagStartTag) {
          currentData += c;
        }
        if (c == '>') {
          //Serial.print("debug0");
          //Serial.println(currentTag);
          tagInside = false;
          //Serial.println(currentTag);
          if (currentTag.startsWith("<temperature")) {
            //Serial.print("debug1");
            int attribValue = currentTag.indexOf("value=");
            //Serial.println(attribValue);
            if (attribValue != -1) {
              //Serial.print("debug2");
              String tempValue = currentTag.substring(attribValue + 7);
              //Serial.println(tempValue);
              int quote = tempValue.indexOf("\"");
              //Serial.println(quote);
              tempValue = tempValue.substring(0, quote);

              Serial.print("Temperature : ");
              int len = tempValue.length();
              char TEMP_char[len + 1];
              double TEMP;
              strcpy(TEMP_char, tempValue.c_str());
              TEMP = atof(TEMP_char);
              TEMP -= 273.16;
              TEMP = roundf(TEMP * 10) / 10;
              Serial.println(TEMP, 1);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print(TEMP, 1);
              break;
            }
            //Serial.println(tempValue);
          }
          currentTag = "";
        }
      }
      break;
    }


  }

  //미세먼지
  if (button_state == 2) {
    lcd.begin(16, 2);
    lcd.clear();
    char c = client_dust.read();
    if (c == '<') {
      tagInside = true;
    }
    if (tagInside) {
      currentTag += c;
    } else if (flagStartTag) {
      currentData += c;
    }
    if (c == '>') {
      //Serial.print("debug0");
      //Serial.println(currentTag);
      tagInside = false;
      //Serial.println(currentTag);
      if (currentTag.startsWith("</pm10Grade1h")) {

        //        //Serial.println(tempValue);
        //        int quote = pm10Grade.indexOf("\"");
        //        //Serial.println(quote);
        //        pm10Grade = pm10Grade.substring(0, quote);

        Serial.print("pm10 : ");
        //        int len = pm10Grade.length();
        //        char pm10_char[len + 1];
        //        int pm10;
        //        strcpy(pm10_char, pm10Grade.c_str());
        //        pm10 = atoi(pm10_char);
        Serial.println(currentData);
        int len = currentData.length();
        char pm10_char[len + 1];
        strcpy(pm10_char, currentData.c_str());
        int pm10 = atoi(pm10_char);
        String Grade10;
        if (pm10 == 1) Grade10 = "Good";
        else if (pm10 == 2) Grade10 = "Normal";
        else if (pm10 == 3) Grade10 = "Bad";
        else if (pm10 == 4) Grade10 = "VeryBad";
        lcd.clear();
        lcd.print("pm10 : ");
        lcd.print(Grade10);

      }
      currentTag = "";
      currentData = "";

    }
  }
  char c = client_dust.read();
  if (c == '<') {
    tagInside = true;
    if (tagInside) {
      currentTag += c;
    } else if (flagStartTag) {
      currentData += c;
    }
    if (c == '>') {
      //Serial.print("debug0");
      //Serial.println(currentTag);
      tagInside = false;
      //Serial.println(currentTag);
      if (currentTag.startsWith("</pm25Grade1h")) {

        //        //Serial.print("debug2");
        //        String pm25Grade = currentTag.substring(attribValue + 7);
        //        //Serial.println(tempValue);
        //        int quote = pm25Grade.indexOf("\"");
        //        //Serial.println(quote);
        //        pm25Grade = pm25Grade.substring(0, quote);

        Serial.print("pm25 : ");
        //          int len = pm25Grade.length();
        //          char pm25_char[len + 1];
        //          int pm25;
        //          strcpy(pm25_char, pm25Grade.c_str());
        //          pm25 = atoi(pm25_char);
        Serial.println(currentData);
        int len = currentData.length();
        char pm25_char[len + 1];
        strcpy(pm25_char, currentData.c_str());
        int pm25 = atoi(pm25_char);
        String Grade25;
        if (pm25 == 1) Grade25 = "Good";
        else if (pm25 == 2) Grade25 = "Normal";
        else if (pm25 == 3) Grade25 = "Bad";
        else if (pm25 == 4) Grade25 = "VeryBad";
        lcd.setCursor(0, 1);
        lcd.print("pm25 : ");
        lcd.print(Grade25);

        //Serial.println(tempValue);
      }
      currentTag = "";
      currentData = "";
    }
  }

  //한 loop에 실행시간은 1초보다 적다고 가정하면
  while (true) {
    if (millis() - timeVal >= 1000) {
      if(second != 59) second++;
      else {
        second = 0;
        if(minute != 59) minute++;
        else {
          minute = 0;
          if(hour != 24) hour++;
          else {
            hour = 0;
            //숙제 : 년, 원, 일 추가하기
          }
        }
      }
      break;
    }

    delay(1);
  }


}


unsigned long sendNTPpacket(IPAddress & address) {
  //Serial.println("1");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  //Serial.println("2");
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  //Serial.println("3");

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  //Serial.println("4");
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  //Serial.println("5");
  Udp.endPacket();
  //Serial.println("6");
}

void dust_connectToServer() {
  ret = WiFi.hostByName("openapi.airkorea.or.kr", hostIp_dust);
  Serial.print("ret: ");
  Serial.println(ret);

  Serial.print("dust Host IP: ");
  Serial.println(hostIp_dust);
  Serial.println("");

  Serial.println("connecting to server...");
  String content = "";
  if (client_dust.connect(hostIp_dust, 80)) {
    //    Serial.println("Connected! Making HTTP request to www.kma.go.kr");
    //Serial.println("GET /data/2.5/weather?q="+location+"&mode=xml");
    //client_temp.println("GET /wid/queryDFSRSS.jsp?zone=2818583000 HTTP/1.1");

    client_dust.print("GET /openapi/services/rest/ArpltnInforInqireSvc/getMsrstnAcctoRltmMesureDnsty?stationName=동춘&dataTerm=DAILY&pageNo=1&numOfRows=10&ServiceKey=FVwLFQquorwlohyjpsSO19Ipral144vU9eqQMKN65T6Y%2FpSapVoGUa%2B%2BwNibqh78iMAJpzPGNJRPYmc0DWC3uw%3D%3D");
    //위에 지정된 주소와 연결한다.
    client_dust.println(" HTTP/1.1");
    client_dust.print("HOST: openapi.airkorea.or.kr\n");
    //client_temp.println("User-Agent: launchpad-wifi");
    client_dust.println("Connection: close");

    client_dust.println();
    Serial.println("client_dust Read Print ");
    Serial.println("");
  }

}


void temp_connectToServer() {
  ret = WiFi.hostByName("api.openweathermap.org", hostIp_temp);
  Serial.print("ret: ");
  Serial.println(ret);

  Serial.print("temp Host IP: ");
  Serial.println(hostIp_temp);
  Serial.println("");

  Serial.println("connecting to server...");
  String content = "";
  if (client_temp.connect(hostIp_temp, 80)) {
    //    Serial.println("Connected! Making HTTP request to www.kma.go.kr");
    //Serial.println("GET /data/2.5/weather?q="+location+"&mode=xml");
    //client_temp.println("GET /wid/queryDFSRSS.jsp?zone=2818583000 HTTP/1.1");

    client_temp.print("GET /data/2.5/weather?q=Incheon&mode=xml&appid=c8088903e94843954e3c64865bb51aeb");
    //위에 지정된 주소와 연결한다.
    //client.print("HOST: www.kma.go.kr\n");
    client_temp.println(" HTTP/1.1");
    client_temp.print("HOST: api.openweathermap.org\n");
    client_temp.println("User-Agent: launchpad-wifi");
    client_temp.println("Connection: close");

    client_temp.println();
    Serial.println("client_temp Read Print ");
    Serial.println("");
  }
}

void time_connectToServer() {
  while (!client_time.connect("google.com", 80)) {}
  client_time.print("HEAD / HTTP/1.1\r\n\r\n");
  while (!client_time.available()) {}

  while (client_time.available()) {
    if (client_time.read() == '\n') {
      if (client_time.read() == 'D') {
        if (client_time.read() == 'a') {
          if (client_time.read() == 't') {
            if (client_time.read() == 'e') {
              if (client_time.read() == ':') {
                client_time.read();
                String timeData = client_time.readStringUntil('\r');
                client_time.stop();
                timeData.toCharArray(buf, 30);

                time_cut = strtok(buf, ", :");
                for (int i = 0; time_cut; i++) {
                  time_val[i] = time_cut;
                  time_cut = strtok(0, ", :");
                }
                year   = atoi(time_val[3]);
                month  = month_to_digit((char *)time_val[2]);
                date   = atoi(time_val[1]);
                day    = day_to_digit((char *)time_val[0]);
                hour   = atoi(time_val[4]) + 9;
                minute = atoi(time_val[5]);
                second = atoi(time_val[6]);

                if (hour > 23) {
                  hour %= 24;
                  if (++day > 6) day = 0;
                  if     (!(year % 400)) DaysOfMonth[1] = 29; // 윤년/윤달 계산
                  else if (!(year % 100)) DaysOfMonth[1] = 28;
                  else if (!(year %  4)) DaysOfMonth[1] = 29;

                  if (date < DaysOfMonth[month - 1]) date++;
                  else {
                    date = 1;
                    if (++month > 12) {
                      month = 1;
                      year++;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

int month_to_digit(char* str) {
  for(int i=0; i<12; i++) {
      if(!strncmp(str, (char *)month_arr[i], 3)) return i+1;
  }
}
int day_to_digit(char* str) {
  for(int i=0; i<7; i++) {
      if(!strncmp(str, (char *)day_arr[i], 3)) return i;
  }
}
