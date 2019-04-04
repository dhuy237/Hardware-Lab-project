#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define DHTPIN D4
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  Serial.begin(9600);
  WiFi.begin("The Coffee House","thecoffeehouse");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("connecting");
    delay(100);
  }
  dht.begin();
}
void loop()
{
  float temp = dht.readTemperature();
  float humid = dht.readHumidity();
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print("Humidity: ");
  Serial.print(humid);
  delay(5000);
}
