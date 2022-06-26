#include <ESP8266WiFi.h>
#include "time.h"
#include<NTPClient.h>
#include<WiFiUdp.h>
#include <FirebaseArduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include "printf.h"
#include "RF24.h"
#define coi 0
RF24 radio(2, 15); // pin 2 - CE ,  pin 15 - CSN 
uint8_t address[1][10] = {"gateway"};
byte msg[5];
byte canhBao = 0;
byte khanCap = 0;
unsigned long timer1=0;
unsigned long connectTime=0;
unsigned long timeTN = 0;
unsigned long timeCheckWifi=0;
unsigned long timeCheckWarning=0;
byte countWarning=0;

#define FIREBASE_HOST "projectfinal-9e8e9-default-rtdb.firebaseio.com" 
#define FIREBASE_AUTH "jQhQ0HMHy8VwDSdmuzzzEqdjN0iVji6dWkU0UEgR"   //Không dùng xác thực nên không đổi
#define WIFI_SSID "VongTay"   
#define WIFI_PASSWORD "79KhanhHoa"
WiFiUDP u;
NTPClient n(u, "1.asia.pool.ntp.org", 7*3600);       // lay thoi gian thuc

const String myphone = "0328417742";     // 
void Gsm_Init();                                    // Cau hinh Module Sim 800L
void Gsm_MakeCall(String phone);                   // Ham goi dien
void Gsm_MakeSMS(String phone,String content);     // Ham nhan tin


void setup() {
  pinMode(coi,OUTPUT);
  digitalWrite(coi,LOW);
  Serial.begin(9600); 
  delay(10000);
  Gsm_Init();                                // Cau hinh module Sim 800L
  Gsm_MakeCall(myphone);                    // Test cuoc goi 
  delay(2000);
  Gsm_MakeSMS(myphone,"I'm a test");       // Test tin nhan

  radio.begin();
  radio.setAutoAck(1);
  radio.setDataRate(RF24_1MBPS);          // thiet lap toc do truyen nhan
  radio.setPALevel(RF24_PA_MAX);         // Dung luong toi da
  radio.setChannel(10); // Dat kenh
  radio.openReadingPipe(1, address[0]);     
  radio.startListening(); // RX mode
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) 
  {
  connectTime++;
    delay(500);
    Serial.print("...");
  if(connectTime == 10)
  {
    connectTime = 0;
    Gsm_MakeSMS(myphone, "khong ket noi duoc wifi");
    digitalWrite(coi,HIGH);
    delay(500);
    digitalWrite(coi,LOW);
    delay(500);
    digitalWrite(coi,HIGH);
    delay(500);
    digitalWrite(coi,LOW);
    break;  
  }
  }
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);// ket noi firebase
}

void loop() {
    if(radio.available()){
        while(radio.available()){
        radio.read(&msg, sizeof(msg));
        }
        if(msg[2]==1){
          countWarning++;
          timeCheckWarning = millis();
        }
        if(msg[3]==1){
          canhBao=0;
          Gsm_MakeSMS(myphone, "Toi da an toan!");
        }
        if(msg[4]==1)
        {
          khanCap = 1;
          timeTN = millis();
        }
        String data = convertToJson(String(msg[0]), String(msg[1]), String(canhBao), String(khanCap));
        Serial.println(data);
        Firebase.pushString("data", data);
  }
  if((unsigned long)(millis()-timeCheckWarning)>1000)
  {
    countWarning =0;
  }
  if(countWarning > 5)// han che truong hop hoat dong manh
  {
    canhBao = 1;
  }
  if((unsigned long)(millis()-timeTN)>1000)
  { // goi 1 s thi tat
    khanCap=0;
  }
  if(canhBao == 1)
  {
    digitalWrite(coi,HIGH);
    Gsm_MakeSMS(myphone, "canh bao nga");
    Gsm_MakeCall(myphone);
    delay(4000);
  }
  else
  {
    digitalWrite(coi,LOW);
  }
  if(khanCap == 1)
  {// cuoc goi
    Gsm_MakeSMS(myphone, "Toi dang gap nguy hiem");
    Gsm_MakeCall(myphone);
    delay(5000);
  }
}
String convertToJson(String data1, String data2, String data3, String data4)
{
  char JsonConvertChar[512];
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["time"] = getTimeFromInternet();
  root["heartbeat"] = data1;
  root["SpO2"] = data2;
  root["horn"] = data3;
  root["message"] = data4;
  root["date"] = getDateFromInternet();
  root.printTo((char*)JsonConvertChar, root.measureLength()+1);
  String data = JsonConvertChar;
  jsonBuffer.clear();
  return data;
}
String getTimeFromInternet(){
  n.update();
  String gio, phut, giay;
  if(n.getHours()<10)
  {
    gio = "0"+String(n.getHours());
  }else
  {
    gio = String(n.getHours());
  }
  if(n.getMinutes()<10)
  {
    phut = "0"+String(n.getMinutes());
  }
  else
  {
    phut = String(n.getMinutes());
  }
  if(n.getSeconds()<10)
  {
    giay = "0"+String(n.getSeconds());
  }
  else
  {
    giay = String(n.getSeconds());
  }
  String timeLocal=gio+":"+phut+":"+giay;
  return timeLocal;
}
String getDateFromInternet(){
  n.update();
  String ngay, thang, nam;
  time_t epochTime = n.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
    if(ptm->tm_mday < 10)
    {
    ngay = "0"+String(ptm->tm_mday);
  }
  else
  {
    ngay = String(ptm->tm_mday);
  }
    if(ptm->tm_mon+1 < 10)
    {
    thang = "0"+String(ptm->tm_mon+1);
  }
  else
  {
    thang = String(ptm->tm_mon+1);
  }
    if(ptm->tm_year+1900 < 10)
    {
    nam = "0"+String(ptm->tm_year+1900);
  }
  else
  {
    nam = String(ptm->tm_year+1900);
  }
  String dateLocal=ngay+"/"+thang+"/"+nam;
  return dateLocal;
}
void Gsm_Init()
{
  Serial.println("ATE0");                     // Tat che do phan hoi (Echo mode)
  delay(2000);
  Serial.println("AT+IPR=9600");              // Dat toc do truyen nhan du lieu 9600 bps
  delay(2000);
  Serial.println("AT+CMGF=1");                // Chon che do TEXT Mode
  delay(2000);
  Serial.println("AT+CLIP=1");                // Hien thi thong tin nguoi goi den
  delay(2000);
  Serial.println("AT+CNMI=2,2");              // Hien thi truc tiep noi dung tin nhan
  delay(2000);
}
 
void Gsm_MakeCall(String phone)           
{
  Serial.println("ATD" + phone + ";");         // Goi dien 
  delay(10000);                               // Sau 10s
  Serial.println("ATH");                      // Ngat cuoc goi
  delay(2000);
}
 
void Gsm_MakeSMS(String phone,String content)
{
  Serial.println("AT+CMGS=\"" + phone + "\"");     // Lenh gui tin nhan
  delay(3000);                                     // Cho ky tu '>' phan hoi ve 
  Serial.print(content);                           // Gui noi dung
  Serial.print((char)26);                          // Gui Ctrl+Z hay 26 de ket thuc noi dung tin nhan va gui tin di
  delay(5000);                                     // delay 5s
}
