#include "DHT.h"
#include "DFRobot_SHT20.h"
#include <WiFiManager.h> 
#include "ThingSpeak.h" 
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

LiquidCrystal_I2C lcd(0x27, 20, 4);
DHT dht(DHTPIN, DHTTYPE);
DFRobot_SHT20 sht20;
WiFiClient  client;
WiFiUDP ntpUDP;

const long offsetTime = 25200; 
NTPClient timeClient(ntpUDP, "pool.ntp.org", offsetTime);

unsigned long myChannelNumber = 2348592; //Input Your ChannelID Number
const char * myWriteAPIKey = "488T2HMU14PJW3ME"; //Input Your API_Key ThinkSpeak

String myStatus = "";
int year;
int month;
int day;
int hour;
int minute;
int second;
int count =0;
bool set = false;

int relay1_pin = 15;
int relay2_pin = 2;

void setup() {
  Serial.begin(115200);
  pinMode(relay1_pin, OUTPUT);
  pinMode(relay2_pin, OUTPUT);
  // initialize the LCD
  lcd.begin();

  // Turn on the blacklight and print a message.
  lcd.backlight();

  Serial.println(F("DHT22 Ready!"));
  dht.begin();
  Serial.println("SHT20 Ready!");
  sht20.initSHT20(); // Init SHT20 Sensor
  delay(100);
  sht20.checkSHT20(); // Check SHT20 Sensor
  setupWiFi();

  ThingSpeak.begin(client);  // Initialize ThingSpeak
  timeClient.begin();
  timeClient.update();
  hour = timeClient.getHours();
  minute = timeClient.getMinutes();
  second = timeClient.getSeconds();
  Serial.print(F("Time Begin : "));
  Serial.println(timeClient.getFormattedTime());
  delay(5000);

}

void loop() {
  timeClient.update();
  Serial.print(F("Count : "));
  Serial.println(count);
    if(count > 9){
      count =0 ;
      set = true;
    }
  hour = timeClient.getHours();
  minute = timeClient.getMinutes();
  second = timeClient.getSeconds();
  Serial.println(F("Time Begin : "));
  Serial.println(timeClient.getFormattedTime());

  lcd.clear();
  // Wait a few seconds between measurements.
  sensorRead();
}

void sensorRead() {
  delay(2000);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  }
  Serial.print(F("DHT22 = "));
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F(" % | "));
  Serial.print(F("Temperature: "));
  Serial.print(t);
  Serial.print(F(" C \n"));

  // =============================================================
  delay(2000);
  float shtHum[5];
  float avgShthum = 0, sum = 0;
  float humd = sht20.readHumidity(); // Read Humidity
  float temp = sht20.readTemperature(); // Read Temperature
  Serial.print(F("SHT20 = "));
  Serial.print("Humidity: ");
  Serial.print(humd, 1);
  Serial.print(" % | ");
  Serial.print("Temperature: ");
  Serial.print(temp, 1);
  Serial.print(" C ");
  Serial.println();

  for (int i = 0; i < 5; i++){
    float humd = sht20.readHumidity(); // Read Humidity
    shtHum[i] = humd;
    Serial.print("H: ");
    Serial.print(shtHum[i]);
    Serial.print(" | ");
    delay (1000);
  }
    Serial.print("\n");

  for (int i = 0; i < 5; i++){
    sum += shtHum[i];
  }
  avgShthum = sum / 5.0;
  Serial.print(F("SHT20 Avg = "));
  Serial.print("Humidity: ");
  Serial.print(avgShthum);
  Serial.print(" % ");
  Serial.println();

  lcd.setCursor(0, 0); // แถวที่ 1
  lcd.print("HumdOut = ");
  lcd.setCursor(12, 0); // แถวที่ 1
  lcd.print(h);

  lcd.setCursor(0, 1); // แถวที่ 2
  lcd.print("TempOut = ");
  lcd.setCursor(12, 1); // แถวที่ 2
  lcd.print(t);

  lcd.setCursor(0, 2); // แถวที่ 3
  lcd.print("HumdIn = ");
  lcd.setCursor(12, 2); // แถวที่ 3
  lcd.print(humd);

  lcd.setCursor(0, 3); // แถวที่ 4
  lcd.print("TempIn = ");
  lcd.setCursor(12, 3); // แถวที่ 4
  lcd.print(temp);

  checkHumd(avgShthum);
  sendThing(h, t, avgShthum, temp);
}

void checkHumd(float avgShthum){
  int cHour = timeClient.getHours();
  Serial.print("\nCheckHourNow = ");
  Serial.print(cHour);
  Serial.print("\n");
  if (cHour >= 6 && cHour < 22) {
    digitalWrite(relay2_pin, HIGH);
    Serial.println("Fog On.....");
  } else {
    digitalWrite(relay2_pin, LOW);
    Serial.println("Fog Off.....");
  }

  if (avgShthum < 80.00) {
    digitalWrite(relay1_pin, HIGH);
    Serial.println("Pump On.....");
  } else {
    digitalWrite(relay1_pin, LOW);
    Serial.println("Pump Off.....");
  }

}

void sendThing(float DH, float DT, float SH, float ST){
  Serial.print("\n Sending Data");
  for (int i = 0; i < 3; i++){
    delay(1000);
    Serial.print(".");
  }

  ThingSpeak.setField(1, DH);
  ThingSpeak.setField(2, DT); 
  ThingSpeak.setField(3, SH);
  ThingSpeak.setField(4, ST);

  delay(15000);
  ThingSpeak.setStatus(myStatus);

  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
}

void setupWiFi() {
    WiFiManager wm;
    bool res;
    res = wm.autoConnect("Chamber V.2","12345678"); // password protected
    if(!res) {
      Serial.println("Failed to connect");     
    } 
    else {
      Serial.println("connected...:) | Yoohoo!!");
    }
}
