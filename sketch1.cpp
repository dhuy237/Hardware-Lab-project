#include "DHT.h"
#include <Ticker.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN D8
#define RST_PIN 0
#define dht_dpin D3
#define DHTTYPE DHT11
#define buttonPin A0

Ticker secondtick;
DHT dht(dht_dpin, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

MFRC522 mfrc522(SS_PIN, RST_PIN);

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
void menuScreenUnlock()
{
  lcd.setCursor(0,0);
  lcd.print("Locked! Use your");
  lcd.setCursor(0,1);
  lcd.print("card to unlock");
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
void RFIDCard()
{
  if ( ! mfrc522.PICC_IsNewCardPresent() && cardStatus == 0)
    return;
  if ( ! mfrc522.PICC_ReadCardSerial() && cardStatus == 0)
    return;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    cardContent.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    cardContent.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  cardContent.toUpperCase();
  if(cardContent.substring(1) == "1C C5 DC 73" && cardStatus == 0)
    cardStatus = 1;
  else
  {
    cardContent = "";
    cardStatus = 0;
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
  Serial.println(buttonValue);
  if (millis() - last > interval)
  {
    if (buttonValue >= 350 && buttonValue <= 400)
    {
      counter = 0;
      buttonState = 1;
    }
    if (buttonValue >= 850 && buttonValue <= 880)
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
    case 1:
      menuScreenUnlock();
      RFIDCard();
      if (cardStatus == 1)
      {
        lcd.clear();
        state = 2;
      }
      counterDog = millis();
      break;
    case 2:
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
      if (buttonValue >= 900 && buttonValue <= 950 && menuCount == 0)
      {
          lcd.clear();
          cardStatus = 0;
          cardContent = "";
          state = 1;
      }
      counterDog = millis();
      break;
    case 3:
      menuScreen();
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
          cardStatus = 0;
          cardContent = "";
          lcd.clear();
          state = 4;
      }
      counterDog = millis();
      break;
    case 4:
      confirmScreen();
      RFIDCard();
      if (cardStatus == 1)
      {
        lcd.clear();
        state = 5;
      }
      else
        state = 4;
      counterDog = millis();
      break;
    case 5:
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
  }
}
