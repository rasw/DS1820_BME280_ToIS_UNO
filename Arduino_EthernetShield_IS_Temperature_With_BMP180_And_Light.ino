/*
  Web client
 This sketch sends an event to Initial State (http://insecure-groker.initialstate.com)
 using an Arduino Wiznet Ethernet shield.
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 */

#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SFE_BMP180.h>
#include <Wire.h>

#define ONE_WIRE_BUS 7
#define LED 5
#define P13LED 13
#define LightSensor A0

float temperatureDS18B20 = 0;
double temperatureBMP180 = 0;
double baroPressure = 0;

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

SFE_BMP180 bmp180;                        // create BMP180 object called bmp180
float BaroCalibrationCorrection = 1.00660;
int lightSensorValue = 0;
#define ALTITUDE 45.0                     // Altitude of 3 Ryebank in meters.

void setup() 
{
  pinMode(LED, OUTPUT);
  pinMode(P13LED, OUTPUT);

  Serial.begin(9600);

  ds18b20.begin();   // Start up the ds library  

  if (bmp180.begin())
    Serial.println("BMP180 init success");
  else
  {
    Serial.println("BMP180 init fail\n\n");
  }
}

void loop() 
{
  char status;
  double T,P;
  
  digitalWrite(P13LED,HIGH);
  String readingHttp ="";
  lightSensorValue = map(analogRead(LightSensor),0,1023,0,100);
  
  ds18b20.requestTemperatures(); // Send the command to get temperatures DS1820
  delay(500);
  temperatureDS18B20 = ds18b20.getTempCByIndex(0);  // get the ds1820 temperature
 
  status = bmp180.startTemperature();
   
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = bmp180.getTemperature(T);
    if (status != 0)
    {
      temperatureBMP180 = T;
            
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait). If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.

      status = bmp180.startPressure(3);
      if (status != 0)
      {
        delay(status);  // Wait for the measurement to complete:

        // Retrieve the completed pressure measurement:
        // Note that the measurement is stored in the variable P. Note also that the function requires the previous temperature measurement (T).
        // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
        // Function returns 1 if successful, 0 if failure.

        status = bmp180.getPressure(P,T);
        if (status != 0)
        {
          //baroPressure = (P * 0.0295333727);
          baroPressure = (P * BaroCalibrationCorrection);
          // Print out the measurement:
//          Serial.print("absolute pressure: ");
//          Serial.print(P * BaroCalibrationCorrection,2);
//          Serial.print(" mb, ");
//          Serial.print(P * 0.0295333727,2); // ORIGINAL
//          Serial.println(" inHg");
        }
        else Serial.println("error retrieving pressure\n");
      }
      else Serial.println("error starting pressure\n");
    }
    else Serial.println("error retrieving temperature\n");
  }

  //Build HTTP string of parameters  
  readingHttp += "&Temperature=" + String(temperatureDS18B20);
  readingHttp += "&TemperatureInternal=" + String(temperatureBMP180);
  readingHttp += "&BarometricPressure=" + String(baroPressure);
  readingHttp += "&LightLevel=" + String(lightSensorValue);
  
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, gateway, subnet);
  }

  digitalWrite(P13LED,LOW);
  
  delay(1000);    // give the Ethernet shield a second to initialize:
  Serial.println("connecting.");
  if (client.connect(server, 80))
  {
    Serial.println("connected");
    //client.println("GET /api/events?accessKey=V4iVTL0C9Mh5m5gMPP7XLwwF4y3mouYI&bucketKey=EF67ENPARL5F&rasw=99.9 HTTP/1.1");
    client.println(httpString + readingHttp + httpEnd);
    client.println("Host: insecure-groker.initialstate.com");
    //client.println("Connection: close");
    client.println();
   } 
   else 
   {
      Serial.println("connection failed"); // if you didn't get a connection to the server:
   }

  // if there are incoming bytes available from the server, read them and print them:
  if (client.available()) {
    char c = client.read();
    Serial.println(c);
  }

  // if the server's disconnected, stop the client:
  if (!client.connected())
  {
    Serial.println("disconnecting.");
    client.stop();
  }

    //Serial.println(httpString + readingHttp + httpEnd);
    client.stop();
    delay(30000);   // wait 30 seconds
}

float GetTemperatures()
{
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;
  float fahrenheit;

  if ( !ds.search(addr)) {
    //Serial.println("No more addresses.");
    //Serial.println();
    ds.reset_search();
    delay(250);
    return -100.00;
  }
  
//  Serial.print("ROM =");
//  for( i = 0; i < 8; i++) {
//    Serial.write(' ');
//    Serial.print(addr[i], HEX);
//  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return 888;
  }
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 0);        // start conversion, with parasite power off at the end (1 for ON)
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad 

  //Serial.print("  Data = ");
  //Serial.print(present, HEX);
  //Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    //Serial.print(data[i], HEX);
    //Serial.print(" ");
  }
  //Serial.print(" CRC=");
  //Serial.print(OneWire::crc8(data, 8), HEX);
  //Serial.println();

  // Convert the data to actual temperature because the result is a 16 bit signed integer, 
  // it should be stored to an "int16_t" type, which is always 16 bits even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  //fahrenheit = celsius * 1.8 + 32.0;
   //Serial.println(celsius);
   return celsius;
}
