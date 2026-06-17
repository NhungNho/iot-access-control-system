#include "QuanLyMK.h"
#include "WiFi.h"
#include <HTTPClient.h>

#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

#define LED_MO   4   // LED báo cửa mở
#define LED_DONG 16    // LED báo cửa đóng

const char* ssid = "WiFi Tro";  
const char* Password = "012345678"; 

// const char* ssid = "UTC2";  
// const char* Password = ""; 
String Web_App_URL = "https://script.google.com/macros/s/AKfycbyUJmlG3j_D9XgJE03I3inLGKE-aKu5OYNpfGkmhdrZKo2tToUpU3FisACedxYESa8w/exec";

// ===== MQTT HiveMQ Cloud =====
const char* mqtt_server = "741a15d1156f4924b1122e00e2cf1d9d.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "cam_nhung";
const char* mqtt_password = "123456789Camnhung";

// ===== MQTT =====
WiFiClientSecure espClient;
PubSubClient client(espClient);

// ================= CALLBACK (NHẬN DATA) =================
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println("Message arrived [" + String(topic) + "]: " + message);
// ===== WEB YÊU CẦU LẤY TRẠNG THÁI =====
  if (String(topic) == "esp32/get_state") {

      Serial.println("Web request current state");

      publishData();
  }

  if (String(topic) == "esp32/client") {
      loai = 2;
    // Cách 1: nhận dạng đơn giản
    if (message == "1") {
      if (state == 0){
      state = 1;
      Serial.println("Cua mo");
      }
      else {
        Serial.println("Cua dang mo");
      }

    } 
    else if (message == "0") {
     if (state == 1){
      state = 0;
      Serial.println("Cua dong");
      }
      else {
        Serial.println("Cua dang dong");
      }
    }
  }
  if(String(topic) == "esp32/add_mode")
{
    addMode = true;

    lcd.clear();
    lcd.print("Quet the moi");

    Serial.println("Che do them the");
}
if(String(topic) == "esp32/delete_card")
{
    deleteCard(message);

    client.publish(
        "esp32/card_deleted",
        message.c_str()
    );

    publishCardList();
}
if(String(topic) == "esp32/get_card_list")
{
    publishCardList();
}
}
// ================= RECONNECT MQTT =================
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    String clientID = "ESP32Client-";
    clientID += String(random(0xffff), HEX);

    if (client.connect(clientID.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Subscribe topic điều khiển LED
      client.subscribe("esp32/client");
      client.subscribe("esp32/get_state");

      client.subscribe("esp32/add_mode");
      client.subscribe("esp32/delete_card");
      client.subscribe("esp32/get_card_list");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// ================= PUBLISH DATA =================
void publishData() {

  DynamicJsonDocument doc(256);

  // ===== HÀNH ĐỘNG =====
  doc["hanhdong"] = (state == 1) ? "Mo" : "Dong";

  // ===== HÌNH THỨC =====
  if(loai == 0) doc["hinhthuc"] = "The";
  else if(loai == 1) doc["hinhthuc"] = "Mat_khau";
  else doc["hinhthuc"] = "Web";

  // ===== GHI CHÚ =====
  if(loai == 0){
    doc["ghichu"] = (tt_the == 1) ? "Hop_le" : "Sai";
  }
  else if(loai == 1){
    doc["ghichu"] = (tt_mk == 1) ? "Hop_le" : "Sai";
  }
  else{
    doc["ghichu"] = "Hop_le";
  }
  char buffer[128];
  serializeJson(doc, buffer);

client.publish("esp32/door", buffer, true);

  Serial.println("MQTT: " + String(buffer));
}
void publishCardList()
{
    DynamicJsonDocument doc(1024);

    JsonArray arr = doc.to<JsonArray>();

    for(int i = 0; i < soLuongThe; i++)
    {
        arr.add(dsThe[i]);
    }

    String json;
    serializeJson(doc, json);

    client.publish(
        "esp32/card_list",
        json.c_str(),
        true
    );

    Serial.println("Danh sach the:");
    Serial.println(json);
}
void setup() {

  Serial.begin(115200);
  pinMode(LED_MO, OUTPUT);
  pinMode(LED_DONG, OUTPUT);

  // Trạng thái ban đầu: cửa đóng
  digitalWrite(LED_MO, LOW);
  digitalWrite(LED_DONG, HIGH);

  lcd.init(); // Khởi tạo màn hình Màn hình 
  lcd.backlight(); // Bật đèn màn hình Màn hình 
  lcd.print(" Nhap mat khau!"); 
  //RFID
  SPI.begin(); // init SPI bus
  rfid.PCD_Init(); // init MFRC522
  pref.begin("RFID", false);

  loadCards();
  Serial.println("Danh sach the:");

  for(int i=0;i<soLuongThe;i++)
  {
      Serial.println(dsThe[i]);
  }
  // deleteCard("E378BA2C");
  
  // if (soLuongThe == 0) {
  //   addCard("4155D716");
  //   addCard("E378BA2C");
  // }
  // MQTT TLS
  espClient.setInsecure();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  //
  Serial.println("-------------");
  Serial.println("WIFI mode : STA");
  WiFi.mode(WIFI_STA);
  Serial.println("-------------");

  Serial.println();
  Serial.println("------------");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, Password);
  int timeWait = 30; //--> 30 seconds.
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    if (timeWait > 0) timeWait--;
    if (timeWait == 0) {
      delay(1000);
      ESP.restart();
    }
    delay(1000);
  }
}
int last_state = state;
int last_tt_the = tt_the;
int last_tt_mk = tt_mk;
int last_dem = dem;
void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  Xu_Ly_MK();
    // Điều khiển LED theo trạng thái cửa
  if(state == 1){ // Cửa mở
      digitalWrite(LED_MO, HIGH);
      digitalWrite(LED_DONG, LOW);
  }
  else{ // Cửa đóng
      digitalWrite(LED_MO, LOW);
      digitalWrite(LED_DONG, HIGH);
  }
  if (state != last_state || tt_the != last_tt_the ||tt_mk != last_tt_mk||last_dem !=dem){
  publishData();
  writeSheet();
  }

    last_state = state;
  last_tt_the = tt_the;
  last_tt_mk = tt_mk;
  last_dem = dem;
}
void writeSheet(){
  String Send_Data_URL = Web_App_URL + "?sts=write";
  // ===== HÀNH ĐỘNG =====
  if(state == 1){
    Send_Data_URL += "&hanhdong=Mo";
  } else {
    Send_Data_URL += "&hanhdong=Dong";
  }
  // ===== HÌNH THỨC =====
  if(loai == 0){
    Send_Data_URL += "&hinhthuc=The";
  } else if (loai ==1) {
    Send_Data_URL += "&hinhthuc=Mat_khau";
  }
  else {
    Send_Data_URL += "&hinhthuc=Web";
  }

  // ===== GHI CHÚ =====
if(loai == 0){ // dùng thẻ
  if(tt_the == 1){
    Send_Data_URL += "&ghichu=Hop_le";
  } else {
    Send_Data_URL += "&ghichu=Sai";
  }
}
else if (loai==1){ // dùng mật khẩu
  if(tt_mk == 1){
    Send_Data_URL += "&ghichu=Hop_le";
  } else {
    Send_Data_URL += "&ghichu=Sai";
  }
}
else {
  Send_Data_URL += "&ghichu=Hop_le";
}
  
  Serial.println();
  Serial.println("-------------");
  Serial.println("Send data to Google Spreadsheet...");
  Serial.print("URL : ");
  Serial.println(Send_Data_URL);
  HTTPClient http;

  // HTTP GET Request.
  http.begin(Send_Data_URL.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  // Gets the HTTP status code.
  int httpCode = http.GET(); 
  Serial.print("HTTP Status Code : ");
  Serial.println(httpCode);

  // Getting response from google sheets.
  String payload;
  if (httpCode > 0) {
    payload = http.getString();
    Serial.println("Payload : " + payload);    
  }
  
  http.end();
}
String dataForm(float value, int leng, int decimal){
  String str = String(value,decimal);
  if(str.length()<leng){
    int space = leng-str.length();
    for(int i=0;i<space;++i){
      str = " "+str;
    }
  }
  return str;
}

