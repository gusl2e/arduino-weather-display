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

    epoch += 1;
    //Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    lcd.begin(16, 2);
    lcd.setCursor(0,0);
    lcd.clear();
    lcd.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    lcd.print(':');
    if (((epoch % 3600) / 60) < 10) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      lcd.print('0');
    }
    lcd.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    lcd.print(':');
    if ((epoch % 60) < 10) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      lcd.print('0');
    }
    lcd.print(epoch % 60); // print the second


    
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
              lcd.setCursor(0,0);
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
  while(true){
    if(millis()-timeVal>=1000){
      break; 
    }
    
    loop(1);
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
  if (client_temp.connect(hostIp_dust, 80)) {
    //    Serial.println("Connected! Making HTTP request to www.kma.go.kr");
    //Serial.println("GET /data/2.5/weather?q="+location+"&mode=xml");
    //client_temp.println("GET /wid/queryDFSRSS.jsp?zone=2818583000 HTTP/1.1");

    client_temp.print("GET /openapi/services/rest/ArpltnInforInqireSvc/getMsrstnAcctoRltmMesureDnsty?stationName=동춘&dataTerm=DAILY&pageNo=1&numOfRows=10&ServiceKey=FVwLFQquorwlohyjpsSO19Ipral144vU9eqQMKN65T6Y%2FpSapVoGUa%2B%2BwNibqh78iMAJpzPGNJRPYmc0DWC3uw%3D%3D");
    //위에 지정된 주소와 연결한다.
    client_temp.println(" HTTP/1.1");
    client_temp.print("HOST: openapi.airkorea.or.kr\n");
    //client_temp.println("User-Agent: launchpad-wifi");
    client_temp.println("Connection: close");

    client_temp.println();
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
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  unsigned long secsSince1900 = highWord << 16 | lowWord;
  const unsigned long seventyYears = 2208988800UL;
  unsigned long epoch_first = secsSince1900 - seventyYears;

  while (true)
  {
    lcd.clear();
    delay(1);

    Udp.begin(localPort);
    sendNTPpacket(timeServer);
    delay(1000);
    int parse_packet = 0;
    parse_packet = Udp.parsePacket();
    Serial.println(parse_packet);
    Serial.println("");

    Udp.read(packetBuffer, NTP_PACKET_SIZE);
    Serial.print("time_connection_attempt = ");
    Serial.println(time_connection_attempt);
    if (parse_packet == 48) {
      Serial.println("ParsePacket == 48");
      Serial.println(epoch_first);

      time_connection_attempt ++;
      lcd.print("count = ");
      lcd.print(time_connection_attempt);
      lcd.clear();
      if (time_connection_attempt == 5) {
        is_first = false;

        //대한민국 보정
        epoch_first = epoch_first + 9 * 60 * 60;
        epoch = epoch_first;
        break;
      }

    }
    else {
      time_connection_attempt = 0;
    }
  }
}
