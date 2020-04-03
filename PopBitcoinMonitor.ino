#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
//#include <ESP8266WiFiMulti.h>

#define ENABLE_LED


#if defined(ENABLE_LED)
#include "LedControl.h"
#define ALLOC_LED
#endif

//  https://github.com/espressif/arduino-esp32/issues/801#issuecomment-383081333
//  pin6 maybe causing watchdog timeout
//  pin 1 causing watchdog
//  pin 5  causing watchdog
//  pin 6 causing watchdog
//  324
#define LEDPIN_DATAIN 4 
#define LEDPIN_CLOCK  2
#define LEDPIN_CS     3
#define LED_COUNT     1
#define LED_BRIGHTNESS  7

#if defined(ENABLE_LED)
#if defined(ALLOC_LED)
LedControl* pLedDisplay = nullptr;
#else
LedControl LedDisplay = LedControl(LEDPIN_DATAIN,LEDPIN_CLOCK,LEDPIN_CS,LED_COUNT);
#endif
#endif

#ifndef STASSID
#define STASSID "FoxDrop"
#define STAPSK  "Basement123"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

const char* host = "api.cryptowat.ch";
const int httpsPort = 443;
const char* request_url = "/markets/kraken/btcgbp/price";
// Use web browser to view and copy
// SHA1 fingerprint of the certificate
//  gr: got correct one from serial debug
//const char apigithubcom_fingerprint[] PROGMEM = "5F F1 60 31 09 04 3E F2 90 D2 B0 8A 50 38 04 E8 37 9F BC 76";
//const char apigithubcom_fingerprint[] PROGMEM = "59 74 61 88 13 ca 12 34 15 4d 11 0a c1 7f e6 67 07 69 42 f5";
const char apicryptowatch_fingerprint[] PROGMEM = "25 b9 42 e7 41 37 d4 95 50 49 6c 66 3d 80 1d 2c cc 3b e4 46";
#define fingerprint apicryptowatch_fingerprint

//ESP8266WiFiMulti wifiMulti;


bool GetPrice(int& CurrentPrice)
{
  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(host);

  Serial.printf("Using fingerprint '%s'\n", fingerprint);
  client.setFingerprint(fingerprint);

  if (!client.connect(host, httpsPort)) 
  {
    Serial.println("connection failed");
    return false;
  }

  String url = request_url;
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266_soylentgraham\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }

  /*
  {"result":{"price":5514.2},"allowance":{"cost":525989,"remaining":3999474011,"remainingPaid":0,"upgrade":"Upgrade for a higher allowance, starting at $15/month for 16 seconds/hour. https://cryptowat.ch/pricing"}}
   */
  String line = client.readStringUntil('}');
  CurrentPrice = -1;
  if ( line.startsWith("{\"result\":{\"price\":") )
  {
    //  extract price
    CurrentPrice = 9999;    
  }
  /*
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successfull!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }*/
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");

   if ( CurrentPrice < 0 )
    return false;
    return true;
}


void delaySecs(int Secs)
{
  yield();
   ESP.wdtFeed();
 delay(1000*Secs);
  ESP.wdtFeed();
}

void SetDisplay(const char* Text)
{
  Serial.println("------ DISPLAY ------");
  Serial.println(Text);
  Serial.println("------ >>>><<< ------");

#if defined(ENABLE_LED)
  int DisplayIndex = 0;
#if defined(ALLOC_LED)
auto& LedDisplay = *pLedDisplay;
#endif
  LedDisplay.clearDisplay(DisplayIndex);
  
  for ( int Digit=0;  Digit<8;  Digit++ )
  {
    bool Decimal = true;
    char Char = Text[Digit];
    if ( Char == 0 )
      break;
    LedDisplay.setChar(DisplayIndex, Digit, Char,Decimal);
    Serial.print("digit");
    Serial.print(Digit);
    Serial.println("");
  }
#endif
  delay(100);
}


void InitDisplay()
{
  #if defined(ALLOC_LED)
  if ( pLedDisplay )
    return;
    #endif
/*

  //pinMode(SPI_CLK,OUTPUT);
  digitalWrite(0,LOW);
  digitalWrite(2,LOW);
  digitalWrite(3,LOW);
    */
  Serial.println("Initialising display");
#if defined(ALLOC_LED)
 pLedDisplay = new LedControl(LEDPIN_DATAIN,LEDPIN_CLOCK,LEDPIN_CS,LED_COUNT);
#endif
  Serial.println("display ready!");


#if defined(ENABLE_LED)
#if defined(ALLOC_LED)
auto& LedDisplay = *pLedDisplay;
#endif
 int devices=LedDisplay.getDeviceCount();

  //for(int address=0;address<devices;address++) 
  int address=0;
  {
    //The MAX72XX is in power-saving mode on startup
    LedDisplay.shutdown(address,false);
   // Set the brightness to a medium values 
    LedDisplay.setIntensity(address,LED_BRIGHTNESS);
    // and clear the display
    LedDisplay.clearDisplay(address);
    LedDisplay.setRow(address,0,0xff);
   // LedDisplay.setRow(address,1,0xff);
    //LedDisplay.setRow(address,2,0xff);
    //LedDisplay.setRow(address,3,0xff);
  }
  #endif
   SetDisplay("Hello");
}

void setup() 
{
  InitSerial();
 InitDisplay();
  SetDisplay("Hello");
  delay(1000);
}

bool SerialInitialised = false;
bool InitSerial()
{
  if ( SerialInitialised )
    return true;
  Serial.begin(115200); //
  Serial.println();

  SerialInitialised = true;
  return true;
}

bool InitWifi()
{
  Serial.println("InitWifi");
  if ( WiFi.status() == WL_CONNECTED )
    return true;
    
  Serial.print("connecting to ");
  Serial.println(ssid);
  SetDisplay(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 // String ConnectingString = "wifi";
  while (WiFi.status() != WL_CONNECTED)
  {
    //ConnectingString += ".";
    //SetDisplay(ConnectingString);
    delay(500);
  }

  auto Ip = WiFi.localIP();
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(Ip);

  //  show ip!
  for ( int i=0;  i<4;  i++ )
  {
    /*
    String IpString = "";
    IpString += Ip[i];
    IpString += ".";
    SetDisplay(IpString);
    delaySecs(1);*/
  }
  
  return true;
}


void loop() 
{
   Serial.println("Loop");
  InitDisplay();

  if ( !InitSerial() )
  {
    SetDisplay("NOSERIAL!");
    delaySecs(1);
    return;
  }

  if ( !InitWifi() )
  {
    SetDisplay("NOIWIFI!");
    delaySecs(1);
    return;
  }

  int Price = 0;
  if ( !GetPrice(Price) )
  {
    SetDisplay("ERROR!");
    delaySecs(30);
    return;
  }
/*
  String PriceString;
  PriceString += Price;
  SetDisplay(PriceString);*/
  delaySecs(30);
}
