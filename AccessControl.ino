#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>     // 引用程式庫
#include <Keypad.h>
#include <Servo.h>
#include <Ultrasonic.h>
#include <stdlib.h>
#include <time.h>

#define KEY_ROWS            4     // 按鍵模組的列數
#define KEY_COLS            4     // 按鍵模組的行數
#define RST_PIN             A0    // 讀卡機的重置腳位
#define SS_PIN              10    // 讀卡機晶片選擇腳位
#define PASS_LEN            4     //密碼長度
#define STUD_ID_LEN         9     //學號長度
#define SERVO_PIN           9     //RC servo的資料腳位
#define TRIGGER_PIN         A3    //Ultrasonic的TRIG pin
#define ECHO_PIN            A2    //Ultrasonic的Echo pin
#define DOOR_CLOSE_DISTANCE 3     //要關門的距離（公分）

#define MODE_INPUT_PW       true  // Used in GetInput() for getting a password
#define MODE_INPUT_ID       false // Used in GetInput() for getting a student ID


// 依照行、列排列的按鍵字元（二維陣列）
char keymap[KEY_ROWS][KEY_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte colPins[KEY_COLS] = {6, 7, 8, A1};  // 按鍵模組，行1~4接腳。
byte rowPins[KEY_ROWS] = {2, 3, 4, 5}; // 按鍵模組，列1~4接腳。
// 初始化Keypad物件
// 語法：Keypad(makeKeymap(按鍵字元的二維陣列), 模組列接腳, 模組行接腳, 模組列數, 模組行數)
Keypad myKeypad = Keypad(makeKeymap(keymap), rowPins, colPins, KEY_ROWS, KEY_COLS);

int const MAX_MEN = 10;       //最多可以註冊幾位會員
int NUM_MEN = 0;              //會員人數統計
bool REG_MODE = 0;            //註冊模式為否
byte admin[4] = {51, 44, 189, 2};//白卡的十進位UID
byte member[MAX_MEN][4]= {};            //會員表


char pw[PASS_LEN]; //密碼
char customKey;           //讀取

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);   //建立LiquidCrystal_I2C物件

MFRC522 mfrc522(SS_PIN, RST_PIN);  // 建立MFRC522物件

bool lockerSwitch = false;  // 伺服馬達（模擬開關）的切換狀態，預設為「關」
Servo servo;                // 宣告伺服馬達物件

Ultrasonic myUltrasonic (TRIGGER_PIN, ECHO_PIN);


void setup() {
  Serial.begin(9600);
  // Serial.println("RFID reader is ready!");
  SPI.begin();
  mfrc522.PCD_Init();   // 初始化MFRC522讀卡機模組
  lcd.begin(16, 2);           //初始化LCD
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Ready!");
  servo.attach(SERVO_PIN); //設定RC servo的資料腳位
  locker(lockerSwitch);  //設定關門，lockerSwitch預設為false
  srand(time(NULL));  //亂數種子
  // Serial.println(time(NULL));

}

void loop() {
  int cm; //門的距離
  char inPass[PASS_LEN] = {};     //user輸入的密碼
  char inStdID[STUD_ID_LEN] = {}; //user輸入的學號

  // 確認是否有新卡片
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    lcd.clear();
    byte *id = mfrc522.uid.uidByte;   // 取得卡片的UID
    byte idSize = mfrc522.uid.size;   // 取得UID的長度

    if (REG_MODE == 1) { //註冊模式
      if(isAdmin(id, idSize)){ //管理員在註冊模式下再刷一次卡，關閉註冊模式
        // Serial.println("You're the admin. REG_MODE is off.");
        REG_MODE = 0; //關閉註冊模式
        lcd.print("ADMIN, REG-off.");
      }
      else if(isMember(id, idSize)){ //會員在註冊模式下刷卡，關閉註冊模式
        // Serial.println("You're already a member.");
        REG_MODE = 0; //關密註冊模式
        lcd.print("Registered aldy.");
      }
      else {
        if (NUM_MEN == MAX_MEN){ //超過最大會員數即不開放註冊，關閉註冊模式
          // Serial.println("Registration failed. The number of members has reached maximum.");
          REG_MODE = 0; //關閉註冊模式
          lcd.print("Failed. MaxMen.");
        }
        else { //把讀到的卡片的UID存進member二維陣列，完成註冊
          for (byte i = 0; i < 4; i++) {
            member[NUM_MEN][i] = id[i];
          //   Serial.print(member[NUM_MEN][i]);
          //   Serial.print(' ');
          }
          // Serial.println("has become a member.");

          GetInput (inStdID, MODE_INPUT_ID);      //user輸入學號
          for (byte i = 0; i < STUD_ID_LEN; i++){ //輸出學號
            Serial.print(inStdID[i]);
          }
          Serial.print(',');
          PrintUID(id);                           //輸出卡號

          REG_MODE = 0; //完成註冊後，註冊模式關閉
          NUM_MEN++; //會員人數增加一位
          lcd.setCursor(0, 0);
          lcd.print("Registered.");
        }
      }

    }
    else{                //非註冊模式
      if(isAdmin(id, idSize)){ //檢查UID是否為管理員
        PasswordGenerator(pw, id);
        GetInput(inPass, MODE_INPUT_PW); //等待user輸入密碼
        if(CheckPass(inPass)){ //密碼正確
          // Serial.println("You're the admin. REG_MODE is on.");
          REG_MODE = 1;  //開啟註冊模式

          ClearLCD(0, 12, 15);
          lcd.setCursor(0, 0);
          lcd.print("ADMIN, REG-on.");
        }
        else{
          ClearLCD(0, 12, 15);
          lcd.setCursor(0, 0);
          lcd.print("Wrong Password."); //密碼錯誤
        }
      }
      else if(isMember(id, idSize)){ //檢查UID是否為會員
        PasswordGenerator(pw, id);
        GetInput(inPass, MODE_INPUT_PW); //等待user輸入密碼
        if(CheckPass(inPass)){ //密碼正確
          // Serial.println("You're a member.");
          PrintUID(id);
          REG_MODE = 0; //關閉註冊模式

          ClearLCD(0, 12, 15);
          lcd.setCursor(0, 0);
          lcd.print("YOU MAY ENTER.");

          lockerSwitch = true; //開鎖參數設true
          locker(lockerSwitch); //開鎖
          delay(3000);
          for(int i=0;i<50;i++){          // 剛開始的距離不准，所以要多測幾次
            cm = myUltrasonic.Ranging(CM); // 計算與門的距離，單位: 公分
          }

          while (lockerSwitch){
            cm = myUltrasonic.Ranging(CM);  // 計算與門的距離，單位: 公分
            //Serial.println(cm);
            if (    cm < DOOR_CLOSE_DISTANCE // 偵測到與門的距離小於特定公分
                //&&  abs((cm - myUltrasonic.Ranging(CM))) < 1
              ) {
                  // 有時會突然跳出0，
                  // 所以如果前次(cm)與這次(myUltrasonic.Ranging(CM))相差太多，
                  // 就不關門
              delay(1000);
              //一秒後如果們仍關上的狀態
              if (myUltrasonic.Ranging(CM) > DOOR_CLOSE_DISTANCE){
                continue;
              }//待門確實關緊後，再上鎖
              lockerSwitch = false;
              locker(lockerSwitch); //關鎖
            }
          }
        }
        else { //密碼錯誤
          ClearLCD(0, 12, 15);
          lcd.setCursor(0, 0);
          lcd.print("Wrong Password.");
        }
      }
      else { //非會員，不輸入密碼，直接擋在門外
        // Serial.println("You're not a member.");
        lcd.print("FORBIDDEN.");
        PrintUID(id);
        lockerSwitch = false;
        locker(lockerSwitch);
      }
    }
    mfrc522.PICC_HaltA();  // 讓卡片進入停止模式
    delay(1000);
    lcd.clear();
  }
}

bool isAdmin(byte *arr, byte idSize){  //檢查是否為admin
  for (byte i = 0; i < idSize; i++){
    if (arr[i] != admin[i]) return false;
  }
  return true;
}

bool isMember(byte *arr, byte idSize){  //檢查是否為member
  for (byte j = 0; j < MAX_MEN; j++){
    for (byte i = 0; i < idSize; i++){
      if (arr[i] != member[j][i]) break;
      if (i == idSize - 1) return true;
    }
  }
  return false;
}

bool CheckPass(char arr[]){ //檢查密碼是否正確
  for (byte i = 0; i < PASS_LEN; i++){
    if (arr[i] != pw[i]) return false;
  }
  return true;
}

void GetInput(char arr[], bool MODE){

  byte arr_count = 0;
  byte arr_length;
  lcd.setCursor(0, 0);

  if (MODE) {  //MODE = true -> getting a password
    arr_length = PASS_LEN;
    lcd.print("Password:");
  }
  else {      //MODE = false -> getting a student ID
    arr_length = STUD_ID_LEN;
    lcd.print("Student ID:");
  }

  //等待user輸入
  while (arr_count < arr_length){
    customKey = myKeypad.getKey();
    if (customKey == NO_KEY) continue; //從KeyPad讀不到，就持續讀
    else { //讀到user按下按鍵
      arr[arr_count] = customKey;
      lcd.setCursor(arr_count, 1);//依user輸入順序，由左至右，顯示在lcd第二排上
      lcd.print(arr[arr_count]);
      //customKey = NO_KEY; //不用指定為NO_KEY，因為上面的getKey()讀不到時就會是NO_KEY
      arr_count++; //下一個字元
    }
  }
  //lcd.setCursor(0, 0);
}

// 開鎖或關鎖
void locker(bool toggle) {
  if (toggle) {
    servo.write(90); // 開鎖
  } else {
    servo.write(180); // 關鎖
  }
  delay(15); // 等伺服馬達轉到定位
}

void PasswordGenerator(char arr[], char *card){
  int temp;
  for(byte i = 0; i< PASS_LEN; i++){
    temp = (rand() % card[i])%10;
    lcd.setCursor(i + 12, 0);  //右邊數來第4個開始顯示
    lcd.print(temp);
    arr[i] = (((temp * temp) % 10) + '0');
    //密碼為顯示在螢幕上各位數平方在模10
    //e.g.  1234 -> 1496
    //      9487 -> 1649
  }
}

void ClearLCD(int row, int begin, int end){ //在LCD上清除第row行從第begin到第end格的字元
  for (int i = begin; i <= end; i++){
    lcd.setCursor(i, row);
    lcd.print(" ");
  }
}

void PrintUID(byte *arr){  //把UDI印到Serial上
  for(int i = 0; i < 4; i++){
    Serial.print(arr[i]);
  }
  Serial.println("");
}
