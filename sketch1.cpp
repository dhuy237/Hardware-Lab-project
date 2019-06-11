#include "DHT.h"
#include <Ticker.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>
#define SS_PIN D8
#define RST_PIN 0
#define dht_dpin 3
#define DHTTYPE DHT11
#define buttonPin A0
#define TXSIM D3
#define RXSIM D0
Ticker secondtick;
DHT dht(dht_dpin, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial mySerial(TXSIM, RXSIM);
MFRC522 mfrc522(SS_PIN, RST_PIN);

const char *ssid = "Tommmm";
const char *password = "12345678";

const long utcTime = 25200;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcTime);
WiFiClient c;
int pass = 0;
int cardStatus = 0; //for rfid card
String cardContent = "";
String content_mqtt ="";
float humid = 0; //for humidity and temperature sensor
float temp = 0;

int menuCount = 0; //for menu screen

int state = 1; //for finite state machine

int buttonValue = 0; //for button
int buttonState = 0;
int buttonState1 = 0;
int last = 0;
int counter = 0;
int counter1 = 0;
int counter2 = 0;
int interval = 100;
int state_new_member = 0;
unsigned long int time_new_member = 0;
String cardStorage[5] = {"1C C5 DC 73","","","",""};
int posDelete = 0;
volatile int counterDog = millis();



void watchdog()
{
      if (millis() - counterDog >= 20000)
      {
         Serial.println();
         Serial.println("Watchdog activates");
         ESP.reset();
      }
}

void menuScreen()
{
  TempAndHumid();
  lcd.setCursor(1,0);
  lcd.print("T: ");
  lcd.setCursor(3,0);
  lcd.print(temp);
  lcd.setCursor(9,0);
  lcd.print("H: ");
  lcd.setCursor(11,0);
  lcd.print(humid);

  lcd.setCursor(1,1);
  lcd.print("Send Message");
}
void menuScreen2()
{
  lcd.setCursor(1,0);
  lcd.print("Card Management");
  lcd.setCursor(1,1);
  lcd.print("WiFi Settings");
}
void menuCard()
{
  lcd.setCursor(1,0);
  lcd.print("Add Card");
  lcd.setCursor(1,1);
  lcd.print("Delete Card");
}
void menuCard2()
{
  lcd.setCursor(1,0);
  lcd.print("Exit");
}
void menuScreenUnlock()
{
  //timeClient.update();
  lcd.setCursor(0,0);
  lcd.print("Locked!");
  /*
  lcd.setCursor(0,1);
  lcd.print("Time: ");
  lcd.setCursor(7,1);
  lcd.print(timeClient.getHours());
  lcd.print(":");
  lcd.print(timeClient.getMinutes());
  lcd.print(":");
  lcd.print(timeClient.getSeconds());
  */
}
void confirmScreen()
{
  lcd.setCursor(0,0);
  lcd.print("Use your card");
  lcd.setCursor(0,1);
  lcd.print("to confirm!");
}
void deleteMenu1()
{
  lcd.setCursor(1,0);
  lcd.print("Master");
  lcd.setCursor(1,1);
  if (cardStorage[1] != "")
    lcd.print("Card 1");
  else
    lcd.print("Card 1 Empty");
}
void deleteMenu2()
{
  lcd.setCursor(1,0);
  if (cardStorage[2] != "")
    lcd.print("Card 2");
  else if (cardStorage[2] == "")
    lcd.print("Card 2 Empty");
  lcd.setCursor(1,1);
  if (cardStorage[3] != "")
    lcd.print("Card 3");
  else if (cardStorage[3] == "")
    lcd.print("Card 3 Empty");
}
void deleteMenu3()
{
  lcd.setCursor(1,0);
  if (cardStorage[4] != "")
    lcd.print("Card 4");
  else if (cardStorage[4] == "")
    lcd.print("Card 4 Empty");
  lcd.setCursor(1,1);
  lcd.print("Exit");
}
void wifiMenu()
{
  lcd.setCursor(1,0);
  lcd.print("WiFi:");
  if(WiFi.status() == WL_CONNECTED)
    lcd.print("CONNECTED");
  else
    lcd.print("DISCONNECTED");
  lcd.setCursor(1,1);
  lcd.print("Exit");
}
void moveCursor()
{
    lcd.setCursor(0, menuCount);
    lcd.print(">");
}

void readCard()
{
  cardContent = "";
  if ( ! mfrc522.PICC_IsNewCardPresent() )
    return;
  if ( ! mfrc522.PICC_ReadCardSerial() )
    return;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    cardContent.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    cardContent.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  cardContent.toUpperCase();
}
bool checkCard()
{
  if(cardContent.substring(1) != "")
  {
     for (int i = 0; i < 5; i++)
      {
        if (cardStorage[i] == cardContent.substring(1))
        {
          return true;
        }
      }
      return false;
  }
}
int findSlot()
{
  for (int i = 1; i < 5; i++)
  {
    if (cardStorage[i] == "")
    {
      return i;
    }
  }
  return 5;
}
void addCard_ROM(int pos, String member)
{
  for(int i = 0; i < 11; i++)
  {
    EEPROM.write(pos + i, member[i]);
  }
  EEPROM.write(pos + 11, '\0');
  EEPROM.commit();
  Serial.print(readCard_ROM( pos));
}
String readCard_ROM(int pos)
{
  char new_member[100];
  int nextPos = 0;
  unsigned char temp;
  temp = EEPROM.read(pos);
  while ((37 < temp && temp  < 58) || (64 < temp && temp < 123) || temp == 32)
  {
    temp = EEPROM.read(pos + nextPos);
    new_member[nextPos] = temp;

    //Serial.print(new_member[nextPos]);

    nextPos++;
  }

  new_member[nextPos] = '\0';
  Serial.println(new_member);
  return String(new_member);
}
void addAllMember()
{
  for (int i = 1; i < 5; i++)
  {
    int add = i*20 + 100;
    cardStorage[i] = readCard_ROM(add);
  }
}
void deleteCard_ROM(int pos)
{
  for(int i = 0; i < 12; i++)
  {
    EEPROM.write(pos + i, '\0');
  }
  EEPROM.commit();
}
void addCard()
{
  if (findSlot() < 5)
  {
    if (!checkCard() && cardContent.substring(1) != "")
    {
      int pos = findSlot();
      cardStorage[pos] = cardContent.substring(1);
      Serial.println(cardStorage[pos]);
      addCard_ROM(pos*20 + 100, cardStorage[pos]);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Add card");
      lcd.setCursor(9,0);
      lcd.print(pos);
      lcd.setCursor(11,0);
      lcd.print("done");
      delay(1000);
      state_new_member = 1;
      Serial.println(cardStorage[pos]);
      readCard_ROM(pos*20 + 100);
    }
    if (checkCard() && state_new_member == 0)
    {
      int pos = 0;
      for (int i = 0; i < 5; i++)
      {
        if (cardStorage[i] == cardContent.substring(1))
        {
          pos = i;
        }
      }
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Card existed");

      if (pos == 0)
      {
        lcd.setCursor(5,1);
        lcd.print("Card");
        lcd.setCursor(10,1);
        lcd.print("MASTER");
      }
      else if (pos != 0)
      {
        lcd.setCursor(10,1);
        lcd.print("Card");
        lcd.setCursor(15,1);
        lcd.print(pos);
      }
      delay(1000);
      state_new_member = 1;
    }
  }
  else // findSlot = 5
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Full");
    delay(1000);
    state_new_member = 1;
  }
}
void deleteCard(int pos)
{
  if (pos != 0)
  {
    if (cardStorage[pos] != "")
      cardStorage[pos] = "";
  }
}

void TempAndHumid()
{
  humid = dht.readHumidity();
  temp = dht.readTemperature();
}

void send_message(String str) {
  mySerial.println("AT+CMGF=1\r\n");
  delay(200);
  mySerial.println("AT+CMGS=\"+84788966736\"\r\n");
  delay(200);
  mySerial.print(str);
  delay(200);
  while(mySerial.available()){
    mySerial.read();
  }
  mySerial.write(26);
  delay(1000);
  String checkStr="";
  while(mySerial.available()){
    char c = mySerial.read();
    if(c!='\r'&&c!='\n')
    checkStr+=c;
  }
  char temptemp[16];
  checkStr.toCharArray(temptemp , 16);
}
void getdata(char * tp, byte *nd, unsigned int length)
{
 String topic(tp);
 String cmd = String((char*)nd);
 cmd.remove(length);
 if(topic == "unlock")
 {
    if(cmd == "123")
    {
      pass = 1;
      content_mqtt = "1C C5 DC 73";
    }
    else
      pass = 0;
 }
}
PubSubClient TempControl("m16.cloudmqtt.com", 11875 , getdata, c);

void WiFiset()
{
  int state_wifi = 0;
  //unsigned long int time_check_wifi = millis();
  WiFi.begin(ssid, password);
  unsigned long int time_check = millis();
  while(WiFi.status() != WL_CONNECTED && millis() - time_check < 5000)
  {
    Serial.print("*");
    delay(100);
  }
  time_check = millis();
  if (WiFi.status() == WL_CONNECTED)
  {
    while(!TempControl.connect("Temperature control", "doaaiktw", " 1DznqNdzgtWb") && millis() - time_check < 6000){
      Serial.print("/");
      delay(100);}
      Serial.println("MQTT done");
      TempControl.subscribe("unlock");
      TempControl.publish("unlock","Success");
      delay(500);
  }

}

void button()
{
  buttonValue = analogRead(buttonPin);
  if (millis() - last > interval)
  {
    if (buttonValue >= 350 && buttonValue <= 400) //if (buttonValue >= 400 && buttonValue <= 450)
    {
      counter = 0;
      buttonState = 1;
    }
    if (buttonValue >= 800 && buttonValue <= 880) //if (buttonValue >= 950 && buttonValue <= 980)
    {
      counter1 = 0;
      buttonState1 = 1;
    }
    if (buttonValue >= 0 && buttonValue <= 10 && buttonState == 1 && counter == 0)
    {
      counter = 1;
      buttonState = 0;
      last = millis();
    }
    if (buttonValue >= 0 && buttonValue <= 10 && buttonState1 == 1 && counter1 == 0)
    {
      counter1 = 1;
      buttonState1 = 0;
      last = millis();
    }
  }
}
void stateMachine()
{
  switch(state)
  {

    case 1: //lock screen
      menuScreenUnlock();
      readCard();

      if (checkCard())
        cardStatus = 1;
      if (cardStatus == 1 || pass == 1)
      {
        lcd.clear();
        state = 2;
      }
      counterDog = millis();
      break;
    case 2: //main menu - page 1 - temp/humid
      menuScreen();
      moveCursor();
      if (counter == 1)//if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 1;
          state = 3;
      }
      if (buttonValue >= 900 && buttonValue <= 1030 && menuCount == 0) //if (buttonValue >= 1000 && buttonValue <= 1030 && menuCount == 0)
      {
          lcd.clear();
          cardStatus = 0;
          cardContent = "";
          state = 1;
      }
      counterDog = millis();
      break;
    case 3: //main menu - page 1 - send message
      menuScreen();
      moveCursor();
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 0;
          state = 6;
      }
      if (counter1 == 1 && menuCount == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          counter1 = 0;
          buttonState1 = 0;
          cardStatus = 0;
          cardContent = "";
          lcd.clear();
          state = 4;
      }
      if (buttonValue >= 900 && buttonValue <= 1030 && menuCount == 0)//if (buttonValue >= 1000 && buttonValue <= 1030 && menuCount == 0)
      {
          lcd.clear();
          cardStatus = 0;
          cardContent = "";
          state = 1;
      }
      counterDog = millis();
      break;
    case 4: //send message screen
      confirmScreen();
      readCard();
      if (checkCard())
        cardStatus = 1;
      if (cardStatus == 1)
      {
        lcd.clear();
        state = 5;
      }
      if (counter1 == 1)
      {
          counter1 = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 0;
          state = 2;
      }
      counterDog = millis();
      break;
    case 5: // call send message function
      //send messages
      {
      humid = dht.readHumidity();
      temp = dht.readTemperature();
      String temp_humid = (String) humid;
      String temp_temp = (String) temp;
      String SMS = "";
      SMS += "Temperature: " + temp_temp + ". Humidity: " + temp_humid;
      send_message(SMS);
      lcd.print("Sending...");
      delay(1000);
      lcd.clear();
      lcd.print("Done!");
      delay(1000);
      lcd.clear();
      state = 2;
      counterDog = millis();
      break;
      }
    case 6: //main menu - page 2 - card management
      menuScreen2();
      moveCursor();
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 1;
          state = 19;
      }
      if (counter1 == 1 && menuCount == 0)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          counter1 = 0;
          buttonState1 = 0;
          lcd.clear();
          state = 7;
      }
      counterDog = millis();
      break;
    case 7: //card management menu - add card
      menuCard();
      moveCursor();
      state_new_member = 0;
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 1;
          state = 8;
      }
      if (counter1 == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          counter1 = 0;
          buttonState1 = 0;
          lcd.clear();
          menuCount = 0;
          state = 9;
      }
      break;
    case 8: //card management menu - delete
      menuCard();
      moveCursor();
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 0;
          state = 10;
      }
      if (counter1 == 1 && menuCount == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          counter1 = 0;
          buttonState1 = 0;
          lcd.clear();
          menuCount = 0;
          state = 11;
      }
      break;
    case 9: //add card menu
      lcd.setCursor(0,0);
      lcd.print("Scan your card!");
      readCard();
      if (state_new_member == 0)
      {
        addCard();
      }
      /*
      if (state_new_member == 0)
      {
        readCard();
        state_new_member = 1;
        time_new_member = millis();
      }
      if (state_new_member == 1 && millis() - time_new_member > 100)
      {
         addCard();
      }
      */
      if(state_new_member == 1)
      {
        lcd.clear();
        state = 7;
      }
      if (counter1 == 1)
      {
          counter1 = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 0;
          state = 7;
      }
      break;
    case 10: //card management menu - exit
      menuCard2();
      moveCursor();
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 0;
          state = 7;
      }
      if (counter1 == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          counter1 = 0;
          buttonState1 = 0;
          lcd.clear();
          menuCount = 0;
          state = 6;
      }
      break;
    case 11: //delete menu 1 - master
      deleteMenu1();
      moveCursor();
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 1;
          state = 12;
      }
      if (counter1 == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          counter1 = 0;
          buttonState1 = 0;
          lcd.clear();
          menuCount = 0;
          state = 17;
      }
      break;
    case 12: //delete menu 1 - user 1
      deleteMenu1();
      moveCursor();
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 0;
          state = 13;
      }
      if (counter1 == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          lcd.clear();
          counter1 = 0;
          buttonState1 = 0;
          menuCount = 0;
          posDelete = 1;
          state = 18;
      }
      break;
    case 13: //delete menu 2 - user 2
      deleteMenu2();
      moveCursor();
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 1;
          state = 14;
      }
      if (counter1 == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          lcd.clear();
          counter1 = 0;
          buttonState1 = 0;
          menuCount = 0;
          posDelete = 2;
          state = 18;
      }
      break;
    case 14: //delete menu 2 - user 3
      deleteMenu2();
      moveCursor();
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 0;
          state = 15;
      }
      if (counter1 == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          lcd.clear();
          counter1 = 0;
          buttonState1 = 0;
          menuCount = 0;
          posDelete = 3;
          state = 18;
      }
      break;
    case 15: //delete menu 3 - user 4
      deleteMenu3();
      moveCursor();
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 1;
          state = 16;
      }
      if (counter1 == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          lcd.clear();
          counter1 = 0;
          buttonState1 = 0;
          menuCount = 0;
          posDelete = 4;
          state = 18;
      }
      break;
    case 16: //delete menu 3 - user 4
      deleteMenu3();
      moveCursor();
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 0;
          state = 11;
      }
      if (counter1 == 1 && menuCount == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          counter1 = 0;
          buttonState1 = 0;
          lcd.clear();
          menuCount = 0;
          state = 8;
      }
      break;
    case 17:
      lcd.setCursor(0,0);
      lcd.print("Cannot delete");
      lcd.setCursor(0,1);
      lcd.print("Master card");
      delay(1000);
      lcd.clear();
      state = 11;
      break;
    case 18:
      readCard();

      if(cardStorage[posDelete] != "")
      {
        lcd.setCursor(0,0);
        lcd.print("Use MASTER card");
        lcd.setCursor(0,1);
        lcd.print("to confirm!");
        if (cardContent.substring(1) == "1C C5 DC 73")
        {
          lcd.clear();
          deleteCard(posDelete);
          deleteCard_ROM(posDelete*20 + 1);
          lcd.setCursor(0,0);
          lcd.print("Delete");
          lcd.setCursor(0,1);
          lcd.print("card");
          lcd.setCursor(5,1);
          lcd.print(posDelete);
          lcd.setCursor(7,1);
          lcd.print("done");
          delay(1000);
          lcd.clear();
          posDelete = 0;
          state = 11;
        }
      }
      else if (cardStorage[posDelete] == "")
      {
        state = 11;
      }
      if (counter1 == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          counter1 = 0;
          buttonState1 = 0;
          lcd.clear();
          menuCount = 0;
          state = 11;
      }
      break;
    case 19:
      menuScreen2();
      moveCursor();
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 0;
          state = 2;
      }
      if (counter1 == 1 && menuCount == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          counter1 = 0;
          buttonState1 = 0;
          lcd.clear();
          menuCount = 0;
          state = 20;
      }
      break;
    case 20:
      moveCursor();
      wifiMenu();
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 1;
          state = 21;
      }
      if (counter1 == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          counter1 = 0;
          buttonState1 = 0;
          lcd.clear();
          menuCount = 0;
          state = 22;
      }
      break;
    case 21:
      moveCursor();
      wifiMenu();
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 0;
          state = 20;
      }
      if (counter1 == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          counter1 = 0;
          buttonState1 = 0;
          lcd.clear();
          menuCount = 0;
          state = 1;
      }
      break;
    case 22:
      if (WiFi.status() == WL_CONNECTED)
      {
        lcd.clear();
        lcd.print("   connected    ");
        delay(1500);
        counter1 = 0;
        buttonState1 = 0;
        lcd.clear();
        menuCount = 0;
        state = 21;
        return;
      }
      WiFiset();
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("    checking    ");
      while(WiFi.status() != WL_CONNECTED)
      {
        delay(100);
        if (counter1 == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
        {
          counter1 = 0;
          buttonState1 = 0;
          lcd.clear();
          menuCount = 0;
          state = 21;
        }
        break;
      }
      break;

  }
}


void setup()
{
  SPI.begin();
  dht.begin();
  Serial.begin(9600);
  lcd.begin(16,2);
  lcd.init();
  lcd.backlight();
  pinMode(buttonPin, INPUT);
  mfrc522.PCD_Init();
  EEPROM.begin(512);
  WiFiset();

  //timeClient.begin();
  addAllMember();

  secondtick.attach(1, watchdog);

}

void loop()
{
  TempControl.loop();
  button();
  stateMachine();
  counterDog = millis();
}
