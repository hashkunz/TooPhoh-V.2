#include "DHT.h"
#include "DFRobot_SHT20.h"
#include <WiFiManager.h> 
#include "ThingSpeak.h" 
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 13     // Digital pin connected to the DHT sensor
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

const int Del = 2;

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
int relay2_pin = 16;
int relay3_pin = 17;

void setup() {
  Serial.begin(115200);
  pinMode(relay1_pin, OUTPUT);
  pinMode(relay2_pin, OUTPUT);
  pinMode(relay3_pin, OUTPUT);
  // initialize the LCD
  lcd.begin();

  // Turn on the blacklight and print a message.
  lcd.backlight();

  lcd.setCursor(4, 1); // แถวที่ 2
  lcd.print("Starting Up");

  lcd.setCursor(4, 2); // แถวที่ 3
  lcd.print("The System..");

  delay(2000);
  lcd.setCursor(3, 3); // แถวที่ 4
  lcd.print("C H A M B E R");

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
  lcd.clear();
}

void loop() {
  timeClient.update();
  Serial.println("\n=============================================\n");
  Serial.print(F("Count : "));
  Serial.println(count);
    if(count > 9){
      count = 0;
      set = true;
    }
  hour = timeClient.getHours();
  minute = timeClient.getMinutes();
  second = timeClient.getSeconds();
  Serial.println(F("Time Begin : "));
  Serial.println(timeClient.getFormattedTime());
  // Wait a few seconds between measurements.
  sensorRead();
}

void sensorRead() {
  delay(1000);
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
  lcd.clear();

  lcd.setCursor(1, 0); // แถวที่ 1
  lcd.print("HumOut : ");
  // lcd.setCursor(10, 0); // แถวที่ 1
  lcd.print(h);
  lcd.print(" %");

  lcd.setCursor(1, 1); // แถวที่ 2
  lcd.print("TempOut : ");
  // lcd.setCursor(10, 1); // แถวที่ 2
  lcd.print(t);
  lcd.print(" C");

  lcd.setCursor(1, 2); // แถวที่ 3
  lcd.print("HumIn : ");
  // lcd.setCursor(10, 2); // แถวที่ 3
  lcd.print(humd);
  lcd.print(" %");

  lcd.setCursor(1, 3); // แถวที่ 4
  lcd.print("TempIn : ");
  // lcd.setCursor(10, 3); // แถวที่ 4
  lcd.print(temp);
  lcd.print(" C");

  checkHumd(avgShthum, temp);
  delay(1000);
  sendThing(h, t, avgShthum, temp);
}

void checkHumd(float avgShthum, float temp){
  int cHour = timeClient.getHours();
  Serial.print("\nCheckHourNow = ");
  Serial.print(cHour);
  Serial.print("\n");
  //relay channel 2
  if (cHour >= 6 && cHour < 20) {
    digitalWrite(relay2_pin, LOW);
    Serial.println("LED On.....");
    Serial.print("\nCheck Temp Now = ");
    Serial.print(temp);
    //relay channel 3
      if (temp > (25.00 - Del)) {
        digitalWrite(relay3_pin, LOW);
        Serial.println("\nSun_Electric On.....");
      } else if (temp <= 20.00) {
        digitalWrite(relay3_pin, HIGH);
        Serial.println("\nSun_Electric Off.....");
      } else {
        digitalWrite(relay3_pin, HIGH);
        Serial.println("\nSun_Electric Else.....");
      }
  } else {
    //relay channel 2
    digitalWrite(relay2_pin, HIGH);
    Serial.println("LED Off.....");
    Serial.print("\nCheck Temp Now = ");
    Serial.print(temp);
    //relay channel 3
      if (temp > 18.00) {
        digitalWrite(relay3_pin, LOW);
        Serial.println("\nMoon_Electric On.....");
      } else if (temp <= 16.00) {
        digitalWrite(relay3_pin, HIGH);
        Serial.println("\nMoon_Electric Off.....");
      } else {
        digitalWrite(relay3_pin, HIGH);
        Serial.println("\nMoon_Electric Else.....");
      }
  }

  //relay channel 1
  if (avgShthum < 85.00) {
    Serial.print("\nCheck avgShthum Now = ");
    Serial.print(avgShthum);
    digitalWrite(relay1_pin, HIGH);
    Serial.println("\nFog On.....");
  } else {
    Serial.print("\nCheck avgShthum Now = ");
    Serial.print(avgShthum);
    digitalWrite(relay1_pin, LOW);
    Serial.println("\nFog Off.....");
  }
  // delay(10000);
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

  delay(1000);
  ThingSpeak.setStatus(myStatus);

  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("Channel update successful.");
    Serial.println("\n=============================================");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
    Serial.println("\n=============================================");
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
