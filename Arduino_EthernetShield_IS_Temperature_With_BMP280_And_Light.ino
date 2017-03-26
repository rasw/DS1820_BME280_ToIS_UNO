/*
  Web client
 This sketch sends an event to Initial State (http://insecure-groker.initialstate.com)
 using an Arduino Wiznet Ethernet shield.
 Circuit:
 * Ethernet shield attached to pins 3, 4, 6, 10, 11, 12, 13
 */

// Pins 3 - 4 - 6 are used by the W5100 board
#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>              // library I2C address = 0x77h

#define ONE_WIRE_BUS 7
#define LED 6
#define P13LED 13
#define LightSensor A0
#define ALTITUDE 45.0                     // Altitude of 3 Ryebank in meters.

double temperatureCalibration = 0.966;    // Temperature calibration correction value 0.915 drops by -1 oC
double pressureCalibration = 1.0062;       // Pressure calibration correction value  (1.0055 ORIG)
double humidityCalibration = 1.0;         // Humidity calibration correction value
double temperatureDS18B20 = 0;
double temperatureBMP280 = 0;
double baroPressureBMP280 = 0;
double humidityBMP280 = 0;
int lightSensorValue = 0;

String httpString = "GET /api/events?accessKey=V4iVTL0C9Mh5m5gMPP7XLwwF4y3mouYI&bucketKey=EF67ENPARL5F";
String httpEnd = " HTTP/1.1";

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// if you don't want to use DNS (and reduce your sketch size) use the numeric IP instead of the name for the server:
//IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
char server[] = "insecure-groker.initialstate.com";    // name address for Google (using DNS)

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 1, 149);
byte gateway[] = { 192, 168,1,254};
byte subnet[] = { 255, 255, 255, 0 };

// Initialize the Ethernet client library with the IP address and port of the server that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

OneWire ds(ONE_WIRE_BUS); 
DallasTemperature ds18b20(&ds);           // Pass our oneWire reference to Dallas Temperature.

Adafruit_BME280 bme; // I2C               // create the BMP280 object

void setup() 
{
  pinMode(LED, OUTPUT);
  pinMode(P13LED, OUTPUT);

  //Serial.begin(9600);

  ds18b20.begin();   // Start up the ds library  
  bme.begin();
  
//  if (bme.begin())
//  {
//    Serial.println("BMP280 init success");
//  }
//  else
//  {
//    Serial.println("BMP280 init fail\n\n");
//  }

}

void loop() 
{
  digitalWrite(P13LED,HIGH);
  digitalWrite(LED,HIGH);
  String readingHttp = "";
  
  if(lightSensorValue < 509)
  {
    lightSensorValue = 509;
  }
  lightSensorValue = map(analogRead(LightSensor),509,1023,0,100);
  //lightSensorValue = analogRead(LightSensor);
  
  ds18b20.requestTemperatures(); // Send the command to get temperatures DS1820
  delay(500);
  temperatureDS18B20 = ds18b20.getTempCByIndex(0);  // get the ds1820 temperature

  temperatureBMP280 = bme.readTemperature();
  temperatureBMP280 = (temperatureBMP280 * temperatureCalibration);

  baroPressureBMP280 = bme.readPressure() / 100.0F;
  baroPressureBMP280 = (baroPressureBMP280 * pressureCalibration);

  humidityBMP280 = bme.readHumidity();
  humidityBMP280 = (humidityBMP280 * humidityCalibration); 

  //Build HTTP string of parameters  
  readingHttp += "&Temperature=" + String(temperatureDS18B20);
  readingHttp += "&TemperatureInternal=" + String(temperatureBMP280);
  readingHttp += "&BarometricPressure=" + String(baroPressureBMP280);
  readingHttp += "&LightLevel=" + String(lightSensorValue);
  readingHttp += "&Humidity=" + String(humidityBMP280);
  
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, gateway, subnet);
  }

  digitalWrite(P13LED,LOW);
  
  delay(1000);    // give the Ethernet shield a second to initialize:
  //Serial.println("connecting.");
  if (client.connect(server, 80))
  {
    //Serial.println("connected");
    //client.println("GET /api/events?accessKey=V4iVTL0C9Mh5m5gMPP7XLwwF4y3mouYI&bucketKey=EF67ENPARL5F&rasw=99.9 HTTP/1.1");
    client.println(httpString + readingHttp + httpEnd);
    client.println("Host: insecure-groker.initialstate.com");
    //client.println("Connection: close");
    client.println();
   } 
   else 
   {
      //Serial.println("connection failed"); // if you didn't get a connection to the server:
   }

  // if there are incoming bytes available from the server, read them and print them:
  if (client.available()) {
    char c = client.read();
    //Serial.println(c);
  }

  // if the server's disconnected, stop the client:
  if (!client.connected())
  {
    //Serial.println("disconnecting.");
    client.stop();
  }

    //Serial.println(httpString + readingHttp + httpEnd);
    client.stop();
    digitalWrite(LED,LOW);
    digitalWrite(P13LED,HIGH); 
    
    delay(30000);   // wait 30 seconds
}

