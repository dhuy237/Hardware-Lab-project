#include "DHT.h"
#include <Ticker.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define SS_PIN D8
#define RST_PIN 0
#define dht_dpin 3
#define DHTTYPE DHT11
#define buttonPin A0

Ticker secondtick;
DHT dht(dht_dpin, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

MFRC522 mfrc522(SS_PIN, RST_PIN);

const char *ssid = "Duong Huy";
const char *password = "0903059497";

const long utcTime = 25200;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcTime);

int cardStatus = 0; //for rfid card
String cardContent = "";

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

volatile int counterDog = millis();

void setup()
{
  SPI.begin();
  dht.begin();

  lcd.begin(16,2);
  lcd.init();
  lcd.backlight();
  pinMode(buttonPin, INPUT);
  mfrc522.PCD_Init();

  //WiFi.begin(ssid,password);
  /*
  while ( WiFi.status() != WL_CONNECTED )
  {
    delay ( 500 );
    Serial.print ( "." );
  }
  */
  //timeClient.begin();

  secondtick.attach(1, watchdog);
  Serial.begin(9600);
}

void loop()
{
  button();
  stateMachine();
  counterDog = millis();
}

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
}
void menuCard()
{
  lcd.setCursor(1,0);
  lcd.print("Add Card");
  lcd.setCursor(1,1);
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
  /*
  for (int i = 0; i < 5; i++)
  {
        if (cardStorage[i] == cardContent.substring(1))
        {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Read Done");
          delay(100);
          cardStatus = 1;
          cardContent = "";
        }
  }*/
  /*
  if(cardContent.substring(1) == "1C C5 DC 73" && cardStatus == 0)
  {
    cardStatus = 1;
    cardContent = "";
  }

  else
  {
    lcd.clear();
    lcd.print("ERRORERRORERRORE");
    lcd.setCursor(0,1);
    lcd.print("ERRORERRORERRORE");
    cardContent = "";
    cardStatus = 0;
    lcd.clear();
  }
  */
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
  for (int i = 0; i < 5; i++)
  {
    if (cardStorage[i] == "")
    {
      return i;
    }
  }
  return 5;
}
void addCard()
{
  if (findSlot() < 5)
  {
    if (!checkCard() && cardContent.substring(1) != "")
    {
      int pos = findSlot();
      cardStorage[pos] = cardContent.substring(1);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Add Done");
      delay(1000);
      state_new_member = 1;
    }
    if (checkCard() && state_new_member == 0)
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Existed");
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
    state_new_member = 0;
  }
}
void deleteCard()
{
  if(cardContent.substring(1) != "")
  {
     for (int i = 0; i < 5; i++)
      {
        if (cardStorage[i] == cardContent.substring(1))
        {
          cardStorage[i] = "";
        }
      }
  }
}
void TempAndHumid()
{
  humid = dht.readHumidity();
  temp = dht.readTemperature();
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
      if (cardStatus == 1)
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
      lcd.print("Sending...");
      delay(1000);
      lcd.clear();
      lcd.print("Done!");
      delay(1000);
      lcd.clear();
      state = 2;
      counterDog = millis();
      break;
    case 6: //main menu - page 2 - card management
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
    case 8: //card management menu - exit
      menuCard();
      moveCursor();
      if (counter == 1) //if (buttonValue >= 350 && buttonValue <= 400)
      {
          counter = 0;
          buttonState = 0;
          lcd.clear();
          menuCount = 0;
          state = 7;
      }
      if (counter1 == 1 && menuCount == 1)// if (buttonValue >= 850 && buttonValue <= 880 && menuCount == 1)
      {
          counter1 = 0;
          buttonState1 = 0;
          lcd.clear();
          menuCount = 0;
          state = 2;
      }
      break;
    case 9: //add card menu
      lcd.setCursor(0,0);
      lcd.print("Scan your card!");
      readCard();
      if (state_new_member == 0)
        addCard();
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
  }
}
