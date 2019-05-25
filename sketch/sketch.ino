#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

#define DHTTYPE DHT11
LiquidCrystal_I2C lcd(0x27, 16, 2);
//----------------
#define SS_PIN 2  
#define RST_PIN 15 
//----------------
#define dht_dpin 0
DHT dht(dht_dpin, DHTTYPE);

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
int statuss = 0;
int out = 0;

void setup(void)
{
  SPI.begin();      // Initiate  SPI bus
  dht.begin();
  
  lcd.begin(16,2);
  lcd.init();
  lcd.backlight();
  
  mfrc522.PCD_Init();   // Initiate MFRC522
  Serial.begin(9600);
  
  delay(700);
}
void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  lcd.setCursor(0,0);
  lcd.print("T: ");
  lcd.setCursor(2,0);
  lcd.print(t);
  lcd.setCursor(9,0);
  lcd.print("H: ");
  lcd.setCursor(11,0);
  lcd.print(h);
  lcd.setCursor(0,1);
  lcd.print("UID:");
  //--------------------------------------
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  //Show UID on serial monitor

  String content= "";
  byte letter;
  
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    //lcd.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    //lcd.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  lcd.setCursor(4,1);
  if (content.substring(1) == "1C C5 DC 73") //change UID of the card that you want to give access
  {
    lcd.print("Access");
    delay(1000);
    statuss = 1;
  }
  else   {
    lcd.print("Denied");
    delay(3000);
  }
  delay(800);
}
