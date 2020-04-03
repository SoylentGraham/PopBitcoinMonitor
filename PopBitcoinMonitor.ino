/*
    HTTP over TLS (HTTPS) example sketch

    This example demonstrates how to use
    WiFiClientSecure class to access HTTPS API.
    We fetch and display the status of
    esp8266/Arduino project continuous integration
    build.

    Limitations:
      only RSA certificates
      no support of Perfect Forward Secrecy (PFS)
      TLSv1.2 is supported since version 2.4.0-rc1

    Created by Ivan Grokhotkov, 2015.
    This example is in public domain.
*/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFiMulti.h>

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

void setup() 
{
}

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
  delay(1000*Secs);
}

void SetDisplay(String Text)
{
  Serial.println("------ DISPLAY ------");
  Serial.println(Text);
  Serial.println("------ >>>><<< ------");
  delay(100);
}

bool SerialInitialised = false;
bool InitSerial()
{
  if ( SerialInitialised )
  return true;
  Serial.begin(57600);
  Serial.println();

  SerialInitialised = true;
  return true;
}

bool InitWifi()
{
  if ( WiFi.status() == WL_CONNECTED )
    return true;
    
  Serial.print("connecting to ");
  Serial.println(ssid);
  SetDisplay(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  String ConnectingString = "wifi";
  while (WiFi.status() != WL_CONNECTED)
  {
    ConnectingString += ".";
    SetDisplay(ConnectingString);
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
    String IpString = "";
    IpString += Ip[i];
    IpString += ".";
    SetDisplay(IpString);
    delaySecs(1);
  }
  
  return true;
}


void loop() 
{
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

  String PriceString;
  PriceString += Price;
  SetDisplay(PriceString);
  delaySecs(60);
}
