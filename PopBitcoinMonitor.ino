//	todo: detect the right board
#define ARDUINO_WIFI

#if defined(ARDUINO_WIFI)
//	gr: if no shield, then wrong include
//#include <WiFi101.h>
#include <WiFiNINA.h>
#else
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
//#include <ESP8266WiFiMulti.h>
#endif

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
#define LEDPIN_DATAIN 5
#define LEDPIN_CLOCK  3
#define LEDPIN_CS     4
#define LED_COUNT     1
#define LED_BRIGHTNESS  7

#if defined(ENABLE_LED)
#if defined(ALLOC_LED)
LedControl* pLedDisplay = nullptr;
#else
LedControl LedDisplay = LedControl(LEDPIN_DATAIN,LEDPIN_CLOCK,LEDPIN_CS,LED_COUNT);
#endif
#endif

#define STASSID "Espolon"
#define STAPSK  ""
//#define STASSID "ZaegerMeister2"
//#define STAPSK  "Hello123456"
//#define STASSID "Tequila"
//#define STAPSK  "hello123"

const char* ssid = STASSID;
const char* password = STAPSK;

//	https://api.cryptowat.ch/markets/kraken/btcgbp/price
const char* host = "api.cryptowat.ch";
const int httpsPort = 443;
const char* request_url = "/markets/kraken/btcgbp/price";
const int RefreshSecs = 30;

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
//  gr: got correct one from serial debug
//const char apigithubcom_fingerprint[] PROGMEM = "5F F1 60 31 09 04 3E F2 90 D2 B0 8A 50 38 04 E8 37 9F BC 76";
//const char apigithubcom_fingerprint[] PROGMEM = "59 74 61 88 13 ca 12 34 15 4d 11 0a c1 7f e6 67 07 69 42 f5";
const char apicryptowatch_fingerprint[] PROGMEM = "25 b9 42 e7 41 37 d4 95 50 49 6c 66 3d 80 1d 2c cc 3b e4 46";
#define fingerprint apicryptowatch_fingerprint

//ESP8266WiFiMulti wifiMulti;

class CharDot_t
{
public:
	CharDot_t(char Char='\0',bool Dot=false) :
		mChar	( Char ),
		mDot	( Dot )
	{
	}
	char	mChar;
	bool	mDot;
};

void DisplayChars(const CharDot_t* Chars,int CharCount)
{
	int DisplayIndex = 0;
#if defined(ALLOC_LED)
	auto& LedDisplay = *pLedDisplay;
#endif
	LedDisplay.clearDisplay(DisplayIndex);

	for ( int Digit=0;	Digit<CharCount && Digit<8;	Digit++ )
	{
		auto& Char = Chars[Digit];
		int DigitIndex = 7-Digit;
		LedDisplay.setChar( DisplayIndex, DigitIndex, Char.mChar, Char.mDot );
	}
}


void SetDisplay(const String& Text,int ScrollSpeed=200)
{
	Serial.println("------ DISPLAY ------");
	Serial.println(Text);
	Serial.println("------ >>>><<< ------");

	const int MAX_LENGTH = 100;
	int Length = 0;
	CharDot_t CharBuffer[MAX_LENGTH];

	auto PushChar = [&](char Char,bool Dot=false)
	{
		if ( Length >= MAX_LENGTH )
			return;
		CharBuffer[Length] = CharDot_t( Char, Dot );
		Length++;
	};
	
	auto PushChars = [&](char CharA,char CharB)
	{
		PushChar(CharA);
		PushChar(CharB);
	};
	
	auto PushDot = [&]()
	{
		//	if no last char, add just a dot
		if ( Length == 0 )
		{
			PushChar(' ',true);
			return;
		}
		
		//	dot already on last char, add a dot on its own
		if ( CharBuffer[Length-1].mDot )
		{
			PushChar(' ',true);
			return;
		}

		//	add the dot to prev char
		CharBuffer[Length-1].mDot = true;
	};

	//	build the display
	for ( int i=0;  true;  i++ )
	{
 		char Char = Text[i];
		if ( Char == 0 )
	  	    break;

		//	special cases
		switch ( Char )
		{
			case 'T':	PushChars('^','T');	break;
			case 'M':	PushChars('N','N');	break;
			case 'm':	PushChars('n','n');	break;
			case 'W':	PushChars('U','U');	break;
			case 'w':	PushChars('u','u');	break;
			case '.':	PushDot();	break;
			default:	PushChar(Char);	break;
		}
	}

	int Scroll = 0;
	bool WillScroll = (Length > 7);
	do 
	{
		DisplayChars( CharBuffer+Scroll, Length-Scroll );

		//	if we're going to scroll, and this is the first display,
		//	hold it a bit longer, otherwise we miss the first 
		//	character as it disapears too quickly
		auto ScrollMult = ( Scroll == 0 && WillScroll ) ? 4 : 1;
		//	we don't delay if not scrolling at all
		if ( !WillScroll )
			ScrollMult = 0;
		delay(ScrollSpeed*ScrollMult);
		
		Scroll++;
	}
	while ( Length-Scroll > 7 );
}


bool GetFloat(int& Major,int& Minor,const char* NumberString)
{
	Major = 0;
	Minor = 0;
	int PartCount = 0;
	//Serial.print("GetFloat() ");	Serial.println(NumberString);
		
	for ( int i=0;	true;	i++ )
	{
		char Char = NumberString[i];
		if ( Char == '\0' )
		{	
			//Serial.println("Got terminator");
			break;
		}
		if ( Char == '.' )
		{
			//Serial.println("Got.");
			PartCount++;
			continue;
		}
		int Number = Char - '0';
		//Serial.print("Got number"); Serial.println(Number);
		//	no more numbers or .
		if ( Number < 0 || Number > 9 )
			break;

		int& Value = ( PartCount == 0 ) ? Major : Minor;
		Value *= 10;
		Value += Number;
	}

	//	made no changes, didn't parse at least major
	if ( Major == 0 && Minor == 0 && PartCount == 0 )
		return false;

	return true;
}
	

bool GetPrice(int& PriceMajor,int& PriceMinor)
{
  // Use WiFiClientSecure class to create TLS connection
 #if defined(ARDUINO_WIFI)
WiFiSSLClient client;
 #else
WiFiClientSecure client;
 #endif
 
  Serial.print("connecting to ");
  Serial.println(host);

#if defined(ARDUINO_WIFI)
#else
  Serial.printf("Using fingerprint '%s'\n", fingerprint);
  client.setFingerprint(fingerprint);
#endif

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
	String Line = client.readStringUntil('}');
	const char* Prefix = "{\"result\":{\"price\":";
	client.stop();

	Serial.println("Reply:");
	Serial.println(Line);
	
	if ( !Line.startsWith(Prefix) )
	{
		Serial.print("Reply didn't start with ");
		Serial.println(Prefix);
		SetDisplay(Line);
		return false;
	}

	const int PrefixLen = strlen(Prefix);
	char PriceStringBuffer[PrefixLen+20];
 	Line.toCharArray(PriceStringBuffer,sizeof(PriceStringBuffer));
	if ( !GetFloat(PriceMajor,PriceMinor,PriceStringBuffer+PrefixLen) )
	{
		Serial.println("Error getting number from reply");
		SetDisplay(Line);
		return false;
	}

    return true;
}


void delaySecs(int Secs)
{
	#if defined(ARDUINO_WIFI)
	#else
	  yield();
   ESP.wdtFeed();
	  #endif
 delay(1000*Secs);
  //ESP.wdtFeed();
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
    //	test, this turns on different digits
    LedDisplay.setRow(address,0,0xff);
    LedDisplay.setRow(address,2,0xff);
    LedDisplay.setRow(address,4,0xff);
    LedDisplay.setRow(address,6,0xff);
    LedDisplay.setRow(address,8,0xff);
  }

	//	show alphabet
	SetDisplay("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz!_.\"?",40);	
  
  #endif
}

void setup() 
{
	InitSerial();
	InitDisplay();
	SetDisplay("Bitcoin monitor!");
	delay(100);
}

bool SerialInitialised = false;
bool InitSerial()
{
  if ( SerialInitialised )
    return true;

   while ( !Serial )
   {}
	Serial.begin(115200); //
	Serial.println();
	Serial.println("Serial initialised");

	SerialInitialised = true;
	return true;
}

String IPAddressToString(IPAddress& Address)
{
	String ipString = String(Address[0]) + '.' + String(Address[1]) + '.' + String(Address[2]) + '.' + String(Address[3]);
	return ipString;
}

const char* wl_status_to_string(wl_status_t status) 
{
  switch (status) {
    case WL_NO_SHIELD: return "WL_NO_SHIELD";
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
    case WL_AP_LISTENING: return "WL_AP_LISTENING";
    case WL_AP_CONNECTED: return "WL_AP_CONNECTED";
    case WL_AP_FAILED: return "WL_AP_FAILED";
    #if defined(WL_PROVISIONING)
    case WL_PROVISIONING: return "WL_PROVISIONING";
    #endif
    #if defined(WL_PROVISIONING_FAILED)
    case WL_PROVISIONING_FAILED: return "WL_PROVISIONING_FAILED";
    #endif
    default:				return "<wl_status_t unknown>";
  }
}
const char* wl_status_to_string(uint8_t status) 
{
	return wl_status_to_string((wl_status_t)status);
}

void Reboot(const char* Error)
{
	SetDisplay(Error);
	delaySecs(1);
	SetDisplay("Reboot!");
	delaySecs(1);
	NVIC_SystemReset();
}

void ListSsids() 
{
	SetDisplay("Scanning...",100);
	int SsidCount = WiFi.scanNetworks();
	if ( SsidCount == -1) 
	{
		Reboot("scan error");
	}

	{
		String Debug;
		Debug += "Found x";
		Debug += SsidCount;
		SetDisplay(Debug);
		delay(500);
	}
	
	for ( int i=0;	i<SsidCount;	i++ )
	{
		String Debug;
		auto Ssid = WiFi.SSID(i);
		Debug += Ssid;
		Debug += "  (";
		Debug += WiFi.RSSI(i);
		Debug += ")";
		SetDisplay(Debug,100);
		delay(100);
	}
}

bool InitWifi()
{
	Serial.println("InitWifi");
	if ( WiFi.status() == WL_CONNECTED )
    	return true;


	String fv = WiFi.firmwareVersion();
	if (fv < WIFI_FIRMWARE_LATEST_VERSION) 
	{
		Serial.println("Please upgrade the firmware");
	}

	ListSsids();

	byte mac[6];
	WiFi.macAddress(mac);
	Serial.print("MAC: ");
	Serial.print(mac[5],HEX);
	Serial.print(":");
	Serial.print(mac[4],HEX);
	Serial.print(":");
	Serial.print(mac[3],HEX);
	Serial.print(":");
	Serial.print(mac[2],HEX);
	Serial.print(":");
	Serial.print(mac[1],HEX);
	Serial.print(":");
	Serial.println(mac[0],HEX);
	
	Serial.print("connecting to ");
	Serial.println(ssid);
	SetDisplay( String(ssid)+String("...?") );
	delay(1000);
	#if !defined(ARDUINO_WIFI)
	WiFi.mode(WIFI_STA);
	#endif

	Serial.print("init wifi status"); Serial.println(wl_status_to_string(WiFi.status()));
	while (WiFi.status() != WL_CONNECTED)
	{
		auto Status = WiFi.status();
 		Serial.print("wifi status"); Serial.println(wl_status_to_string(Status));

		bool Reconnect = false;
		switch(Status)
		{
			//	after 12 hours or so, this was being reported. Reboot arduino worked
			case WL_NO_SHIELD:
				Reboot("WL_NO_SHIELD");
				break;
			
			case WL_NO_SSID_AVAIL:
				SetDisplay("WL_NO_SSID_AVAIL");
				delay(1000);
				ListSsids();
				Reconnect = true;
				break;
				
			case WL_CONNECT_FAILED:
			case WL_DISCONNECTED:
			case WL_IDLE_STATUS:
			case WL_SCAN_COMPLETED:
				Reconnect = true;
				break;
		}
		if ( Reconnect )
 		{
 			//	https://forum.arduino.cc/index.php?topic=566338.msg3861355#msg3861355
			Serial.println("WiFi.end");
 			WiFi.end();
			Serial.println("WiFi.disconnect");
 			WiFi.disconnect();
			Serial.println("WiFi.begin");
			auto NewStatus = WiFi.begin(ssid, password);
 			Serial.print("WiFi.begin status"); Serial.println(wl_status_to_string(NewStatus));
 			delay(2000);
 		}

		Serial.println("waiting to connect...");
	    delay(500);
	}
	
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	IPAddress Ip = WiFi.localIP();
	Serial.println(Ip);

	auto IpString = IPAddressToString(Ip);
    SetDisplay(IpString);
    delaySecs(1);
  
  return true;
}

int LastPriceMajor = -1;

void loop() 
{
   Serial.println("Loop");
  InitDisplay();

  if ( !InitSerial() )
  {
    SetDisplay("No serial!");
    delaySecs(1);
    return;
  }

  if ( !InitWifi() )
  {
    SetDisplay("Wifi failed!");
    delaySecs(1);
    return;
  }

  int PriceMajor = 0;
  int PriceMinor = 0;
  if ( !GetPrice(PriceMajor,PriceMinor) )
  {
    SetDisplay("Error with price!");
    delaySecs(30);
    return;
  }

	//	countdown changes
	const char* ChangeString = "";
	if ( LastPriceMajor != -1 )
	{
		if ( PriceMajor == LastPriceMajor )
			ChangeString = "   =";
		else if ( PriceMajor > LastPriceMajor )
			ChangeString = "  -+";
		else
			ChangeString = "   -";
	}
	else 
	{
		//LastPriceMajor = PriceMajor;
		LastPriceMajor = (PriceMajor / 4) * 3;
	}

	int Step = (PriceMajor > LastPriceMajor) ? 1 : -1;
	for ( int i=LastPriceMajor;	i!=PriceMajor;	i+=Step )
	{
		String PriceString;
		PriceString += i;
		PriceString += '.';
		PriceString += ChangeString;
		SetDisplay(PriceString);
  		delay(20);
	}
	LastPriceMajor = PriceMajor;

	delaySecs(RefreshSecs);
}
