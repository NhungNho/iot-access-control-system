#ifndef  QUANLYMK_H
#define QUANLYMK_H
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h> 
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>      
#include <Preferences.h>
#include <PubSubClient.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); 
// RFID
#define SS_PIN  5 
#define RST_PIN 17

MFRC522 rfid(SS_PIN, RST_PIN);

Preferences pref;

#define MAX_CARD 20

String dsThe[MAX_CARD];
int soLuongThe = 0;


// các biến phục vụ cho việc nhập mật khẩu
const byte ROWS = 4; 
const byte COLS = 4; 
char password[5]; // mảng lưu mật khẩu khi nhập 
char PassOP[]="1234"; // Mật khẩu mở cửa 
char PassCL[]="1111"; // Mật khẩu đóng cửa 
int i = 0; 
int on=0; 
extern int dem = 0; // đếm số lần nhập mk sai
extern int state = 0; // trạng thái cửa đóng
extern int tt_the = 0; // thẻ nhận vào là sai
extern int tt_mk = 0; // mk nhập vào là sai
extern int loai = 0; // 0 là dùng thẻ, 1 là dùng mk
bool addMode = false; 
extern PubSubClient client;
void publishCardList(); 
char MatrixKey[ROWS][COLS] = 
{ 
{'1','2','3','A'}, 
{'4','5','6','B'}, 
{'7','8','9','C'},
{'*','0','#','D'}
}; 
byte rowPins[ROWS] = {13,12,14,27}; // R1,R2,R3,R4 
byte colPins[COLS] = {26,25,33,32}; // C1,C2,C3,C4 
 
Keypad customKeypad  = Keypad( makeKeymap(MatrixKey), rowPins, colPins, ROWS, COLS); 

String UIDtoString(byte *buffer, byte size)
{
  String uid = "";

  for (byte i = 0; i < size; i++) {
    if (buffer[i] < 0x10)
      uid += "0";

    uid += String(buffer[i], HEX);
  }

  uid.toUpperCase();

  return uid;
}

bool checkCard(String uid)
{
  for (int i = 0; i < soLuongThe; i++) {
    if (dsThe[i] == uid)
      return true;
  }

  return false;
}

void saveCards()
{
    pref.clear();

    pref.putInt("count", soLuongThe);

    for(int i=0;i<soLuongThe;i++)
    {
        String key = "card" + String(i);

        pref.putString(
            key.c_str(),
            dsThe[i]
        );
    }
}

void loadCards()
{
  soLuongThe = pref.getInt("count", 0);

  for (int i = 0; i < soLuongThe; i++) {
    String key = "card" + String(i);
    dsThe[i] = pref.getString(key.c_str(), "");
  }
}

void addCard(String uid)
{
  uid.toUpperCase();

  if (checkCard(uid))
    return;

  if (soLuongThe >= MAX_CARD)
    return;

  dsThe[soLuongThe] = uid;
  soLuongThe++;

  saveCards();

  Serial.println("Da them the: " + uid);
}
void deleteCard(String uid)
{
    uid.toUpperCase();

    for(int i = 0; i < soLuongThe; i++)
    {
        if(dsThe[i] == uid)
        {
            // Dịch các phần tử phía sau lên trước
            for(int j = i; j < soLuongThe - 1; j++)
            {
                dsThe[j] = dsThe[j + 1];
            }

            // Xóa phần tử cuối
            dsThe[soLuongThe - 1] = "";

            soLuongThe--;

            saveCards();

            Serial.println("Da xoa the: " + uid);

            return;
        }
    }

    Serial.println("Khong tim thay the");
}
void Xu_Ly_MK(){
if (rfid.PICC_IsNewCardPresent() &&
    rfid.PICC_ReadCardSerial())
{
    loai = 0;
    String uid = UIDtoString(
                    rfid.uid.uidByte,
                    rfid.uid.size);
    Serial.println("UID: " + uid);
    if(addMode)
{
    if(!checkCard(uid))
    {
        addCard(uid);

        client.publish(
            "esp32/card_added",
            uid.c_str()
        );

        publishCardList();

        lcd.clear();
        lcd.print("Them thanh cong");
    }
    else
    {
        lcd.clear();
        lcd.print("The da ton tai");
    }

    addMode = false;

    delay(1500);

    lcd.clear();
    lcd.print("Nhap mat khau!");

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    return;
}
    if (checkCard(uid))
    {
        dem = 0;
        tt_the = 1;

        lcd.clear();

        if (state == 1)
        {
            lcd.setCursor(0, 0);
            lcd.print("Dong cua");

            lcd.setCursor(5, 1);
            lcd.print("bang the");

            state = 0;
        }
        else
        {
            lcd.setCursor(0, 0);
            lcd.print("Mo cua");

            lcd.setCursor(5, 1);
            lcd.print("bang the");

            state = 1;
        }

        delay(1500);

        lcd.clear();
        lcd.print("Nhap mat khau!");
    }
    else
    {
        tt_the = 0;
        dem++;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("The khong hop le");

        delay(1500);

        lcd.clear();
        lcd.print("Nhap mat khau!");
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}

    char EnterKey = customKeypad.getKey(); // đọc giá trị khi nhấn bàn phím 
  
 if (EnterKey) 
 { 
  loai = 1;
    // Nhấn # để xóa toàn bộ mật khẩu đã nhập
    if (EnterKey == '#') {
      i = 0;
      on = 0;

      memset(password, 0, sizeof(password));

      lcd.setCursor(6, 1); // vị trí hiển thị dấu *
      lcd.print("    ");   // xóa 4 ký tự
      lcd.setCursor(6, 1);

      return;
    }
    password[i]=EnterKey; 
    i++; 
    on++; 
    if (i == 0) { 
      password[0] = EnterKey;
      lcd.setCursor(6, 1); 
      lcd.print(password[0]); 
      delay(100); 
      lcd.setCursor(6, 1); 
      lcd.print("*"); // Ký tự được thay bởi dấu * 
    } 
    if (i == 1) { 
      password[1] = EnterKey; 
      lcd.setCursor(7, 1); 
      lcd.print(password[1]); //in ra số đó trên màn hình 
      delay(100); 
      lcd.setCursor(7, 1); 
      lcd.print("*"); //đổi hiển thị số đó thành ký tự * 
    } 
    if (i == 2) { 
      password[2] = EnterKey;
      lcd.setCursor(8, 1); 
      lcd.print(password[2]); 
      delay(100); 
      lcd.setCursor(8, 1); 
      lcd.print("*"); 
    } 
    if (i == 3) { 
      password[3] = EnterKey;
      lcd.setCursor(9, 1); 
      lcd.print(password[3]); 
      delay(100); 
      lcd.setCursor(9, 1); 
      lcd.print("*"); 
    } 
 } 
  if(on==4) 
      { 
    if(!strcmp(password,PassOP)) 
      { 
        if(state == 1){
        lcd.clear(); 
        lcd.print(" Cua dang mo!");
        delay(1000); 
        }
        else{
        lcd.clear(); 
        lcd.print(" Dung mat khau!"); 
        delay(1000); 
        lcd.clear(); 
        lcd.print(" Cua da mo!");
        delay(1000); 
        state = 1; 
        tt_mk = 1;
        }
        lcd.clear(); 
        lcd.print(" Nhap mat khau!"); 
        i=0; 
      } 
      
      if(!strcmp(password,PassCL)) 
      { 
        if(state == 0){
        lcd.clear(); 
        lcd.print(" Cua dang dong!");
        delay(1000); 
      }
      else {
        lcd.clear(); 
        lcd.print("  Cua da dong!");
        state = 0; 
        tt_mk = 1;
        delay(1000); 
      }
        lcd.clear(); 
        lcd.print(" Nhap mat khau!"); 
        i=0; 
      } 
      
      if(strcmp(password,PassOP)) 
{ 
        if(strcmp(password,PassCL)) 
        { 
        
        lcd.clear(); 
        lcd.print(" Mat khau sai!"); 
        tt_mk = 0;
        dem++;
        delay(1500); 
        lcd.clear(); 
        lcd.print("  Xin thu lai!"); 
        delay(1500); 
        lcd.clear(); 
        lcd.print(" Nhap mat khau!"); 
        }
        i = 0;  
        } 
      on=0; 
      }
      if (dem >= 4){
        lcd.clear(); 
        lcd.print("Tam khoa 30s"); 
        for (int t = 30; t > 0; t--) {
        lcd.setCursor(2, 1);        
        lcd.print("Con lai: ");
        lcd.print(t);
        lcd.print("s   ");          
        delay(1000);               
  } 
        dem = 0;
        lcd.clear(); 
        lcd.print(" Nhap mat khau!"); 
      }
}
#endif