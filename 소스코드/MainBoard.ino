#include "WiFiEsp.h"
#include <SoftwareSerial.h> 
#include <Servo.h>

//인체 감지 핀
#define parcelLightPin 2
//WiFi 핀
#define txPin 4
#define rxPin 5 
//택배 핀
#define parcelTrig 6
#define parcelEcho 7
//음성인식핀
#define voiceTx 8
#define voiceRx 9
//모터 핀
#define motorPin 10
//자동문 핀
#define doorTrig 11
#define doorEcho 12
//현관등 핀
#define lightPin 13


//날씨 변수 
String ntemp;
String nsky;
String npty;
float temp=3.0;
int sky=0;
int pty=0;


//택배 변수
bool isParcel=false;

//인체 감지 센서 변수
int sensor;

//모터 변수 및 객체
Servo myservo1;
int pos = 100;
float duration;
float distance;




SoftwareSerial esp01(txPin, rxPin); // SoftwareSerial NAME(TX, RX);
char ssid[] = "DESKTOP-7F5CAU4 6017";    // your network SSID (name)
char pass[] = "43o61O7)";         // your network password
int status = WL_IDLE_STATUS;        // the Wifi radio's status
// https://www.kma.go.kr/wid/queryDFSRSS.jsp?zone=1165056000
const char* host = "www.kma.go.kr";
const String url = "/wid/queryDFSRSS.jsp?zone=1165056000";
WiFiEspClient client; // WiFiEspClient 객체 선언
// RSS 날씨 정보 저장 변수
String line0 = "";
uint8_t count = 0; // RSS 날씨 정보 분류 카운터
// RSS 날씨 정보 수신에 소요되는 시간 확인용 변수
unsigned long int count_time = 0;
bool count_start = false;
int count_val = 0;

SoftwareSerial voiceSerial (voiceTx, voiceRx );
int voice_recogn=0;

//WiFi setup
void get_weather();
void parsing();
void set_weather(){
  esp01.begin(9600);   //와이파이 시리얼
  WiFi.init(&esp01);   // initialize ESP module
  while ( status != WL_CONNECTED) {   // attempt to connect to WiFi network
    Serial.print(F("Attempting to connect to WPA SSID: "));
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);    // Connect to WPA/WPA2 network
  }
  Serial.println(F("You're connected to the network"));
  Serial.println();
  delay(1000);
  get_weather();
  while(client.available()) { // 날씨 정보가 들어오는 동안
    char c = client.read();
    if (c == '<') count++;
    if (count > 28 && count <= 78) line0 += c;
    else if (count > 78) break; // 중간에 빠져나가고 
  }
  if (count != 0) { // 수신 값 있으면 
    while(client.available()) {  // 나머지 날씨정보 버리기
      char c = client.read(); 
    }
    if (!client.connected()) {   // 날씨정보 수신 종료됐으면
      Serial.println();
      Serial.println(F("Disconnecting from server..."));
      client.stop();
    }
    parsing();
    count = 0;
  }
}
void get_weather() {
  Serial.println(F("Starting connection to server..."));
  if (client.connect(host, 80)) {
    Serial.println(F("Connected to server"));
    client.print("GET " + url + " HTTP/1.1\r\n" +  
                 "Host: " + host + "\r\n" +  
                 "Connection: close\r\n\r\n");
    count_start = true;
  }
}
void parsing(){
  int iStart= line0.indexOf(F("<temp>"));
  int iEnd= line0.indexOf(F("</temp>"));
  ntemp = line0.substring(iStart + 6, iEnd);
  //temp=ntemp.toFloat();
  
  iStart= line0.indexOf(F("<sky>"));
  iEnd= line0.indexOf(F("</sky>"));
  nsky = line0.substring(iStart + 5, iEnd);
  //sky=nsky.toInt();
  
  iStart= line0.indexOf(F("<pty>"));
  iEnd= line0.indexOf(F("</pty>"));
  npty = line0.substring(iStart + 5, iEnd);
  Serial.println(ntemp);
  Serial.println(nsky);
  Serial.println(npty);
  //pty=npty.toInt();
   
  line0 = ""; // 스트링 변수 line0 데이터 추출 완료 

  
}

//음성인식 setup
void voice_roc_setup(){
  voiceSerial.begin(9600); // 통신 속도 9600bps로 모듈과 시리얼 통신 시작
  Serial.println("wait voice settings are in progress");
  delay(1000);
  voiceSerial.write(0xAA); // compact mode 사용
  voiceSerial.write(0x37);
  delay(1000);
  voiceSerial.write(0xAA); // 그룹2 음성 명령어 imported
  voiceSerial.write(0x22);

  Serial.println("The voice settings are complete");
  Serial.println();
}

void setup() {
  Serial.begin(9600);

  //인체 감지 setup  2
  pinMode(parcelLightPin, INPUT);

  pinMode(3,INPUT);
  // WiFi setup 4 5
  set_weather();

  //택배 초음파 setup 6 7
  pinMode(parcelTrig, OUTPUT);
  pinMode(parcelEcho, INPUT);

  //음성인식 setup  8 9
  voice_roc_setup();

  //모터 setup
  myservo1.attach(motorPin);

  //자동문 초음파 setup 11 12
  pinMode(doorTrig, OUTPUT);
  pinMode(doorEcho, INPUT);

  //현관등 setup 13
  pinMode(lightPin, OUTPUT);
}



void loop(){
  //택배
  detectParcel();
  //현관등
  turnOnLightIfParcelTrue();

  //자동문
  openDoor();

  
  //음성인식
  voiceRecog();

}
void openDoor() {
  digitalWrite(doorTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(doorTrig, LOW);

  duration = pulseIn(doorEcho, HIGH);
  distance = ((float)(340 * duration) / 10000) / 2;

  if(distance < 10) {
    digitalWrite(lightPin, HIGH);
    for (pos = 90; pos >= 20; pos -= 2) {
      myservo1.write(pos);
      delay(50);
    }
    delay(5000);
    for(pos = 20; pos <= 90; pos += 2) {
      myservo1.write(pos);
      delay(50);
    }
    digitalWrite(lightPin, LOW);
    delay(1000);
  }

}



void turnOnLightIfParcelTrue(){
  //적외선 인체감지 센서에서 값을 읽음
  if (isParcel) {
    digitalWrite(parcelLightPin, HIGH);
  }
  else {
    digitalWrite(parcelLightPin, LOW);
  }
}

//택배 시간 측정
unsigned long start_time;
//측정된 거리가 10cm 이하라면, 모터 100도 회전
void detectParcel() {
  start_time=millis();
  
  while(millis()-start_time<=5000){
    digitalWrite(parcelTrig, LOW);
    digitalWrite(parcelEcho, LOW);
    delayMicroseconds(2);
    digitalWrite(parcelTrig, HIGH);
    delayMicroseconds(10);
    digitalWrite(parcelTrig, LOW);
    unsigned long duration = pulseIn(parcelEcho, HIGH); 
    float distance = ((float)(340 * duration) / 10000) / 2;
    
    if(distance>5){
      isParcel=false;
    }
    else isParcel=true;
  }

}

void voiceRecog(){
  while(voiceSerial.available()){
    voice_recogn= voiceSerial.read();
    digitalWrite(3, HIGH);
    switch(voice_recogn){
      //날씨
      case 0x11:
      Serial.println("날씨");
      if(temp<10)Serial.println('i');
      if(temp>25)Serial.println('j');
      switch(sky){
        case 1:
        Serial.print('f');
        break;
        case 3:
        Serial.println('g');
        break;
        case 4:
        Serial.println('h');
        break;
      }
      break;

      case 0x12:
      Serial.println("우산");
      
      switch(pty){
        case 0:
        Serial.println('a');
        break;
        case 1:
        Serial.println('b');
        break;
        case 2:
        Serial.println('c');
        break;
        case 3:
        Serial.println('d');
        break;
        case 4:
        Serial.println('e');
        break;
      }
      break;
      case 0x13:
      Serial.println("택배");
      if(isParcel) Serial.println('9');
      else Serial.println('8');
      break;

      case 0x14:
      Serial.println("불켜");
      digitalWrite(lightPin, HIGH);
      Serial.println('4');
      delay(7000);
      digitalWrite(lightPin, LOW);
      Serial.println('3');
      break;

      case 0x15:
      Serial.println("안녕");
      Serial.println('1');
      break;
  }
      digitalWrite(3, LOW);

 }
}
