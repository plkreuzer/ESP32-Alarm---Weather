#include <U8g2lib.h>
#include <U8x8lib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Keypad.h>
#include <ArduinoJson.h>

// the OLED used
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x2(15, 4, 16);

const byte ROWS = 4; 
const byte COLS = 4; 

const byte MOTION_PIN = 13;
const byte SPEAKER_PIN = 12;
byte rowPins[ROWS] = {2,17,5,18}; 
byte colPins[COLS] = {23,19,22,21}; 

bool IsArmed = true, IsTriggered = false, SoundAlarm = false;
#define MAX_TRIGGER_COUNT 20
int triggerCount = 0;
char prevMotion = LOW;
String PinCode = "1234";
String buttons;

const String weatherBaseUrl = "http://api.openweathermap.org/data/2.5/weather?zip=75401&APPID=";
const String forecastBaseUrl = "http://api.openweathermap.org/data/2.5/forecast?zip=75401&APPID=";
const String key = "8542ff48ef63a20058c95c4f0d581a89";
const String ssid = "SSID";
const String psk_key = "WPAPSK";

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};


Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

void setup(){
  Serial.begin(9600);
  u8x2.begin();
  u8x2.setFont(u8x8_font_amstrad_cpc_extended_u );
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(MOTION_PIN, INPUT);

  WiFi.begin(ssid.c_str(), psk_key.c_str());
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());
}

float kToF(float kelvin)
{
  float temp = kelvin - 273.13;
  temp = (temp*9.0)/5.0 + 32.0;
  return temp; 
}

void getForecast()
{
  HTTPClient http;

  u8x2.clear();
  u8x2.drawString(0,0,"GETTING WEATHER");
  
  http.begin(forecastBaseUrl + key);
  int httpCode = http.GET();

  if (httpCode <= 0)
  {
    Serial.println("Error on HTTP request");
    return;
  }

  String payload = http.getString();
  Serial.println(httpCode);
  //Serial.println(payload);

  // Allocate JsonBuffer
  // Use arduinojson.org/assistant to compute the capacity.
  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
  DynamicJsonBuffer jsonBuffer(capacity);

  // Parse JSON object
  JsonObject& root = jsonBuffer.parseObject(payload);
  if (!root.success()) {
    Serial.println(F("Parsing failed!"));
    return;
  }

  u8x2.clear();
  String state = root["list"][6]["weather"][0]["main"];
  state.toUpperCase();
  float maxTemp = kToF(root["list"][0]["main"]["temp_max"]);
  float minTemp = kToF(root["list"][0]["main"]["temp_min"]);
  // We get data in 3 hours incriments for the next 5 days
  // So only look at idx 3 to idx 7 for tomorrow's max/min temp
  for (int i = 3; i < 8; i++)
  {
    if (kToF(root["list"][i]["main"]["temp_max"]) > minTemp)
    {
      maxTemp = kToF(root["list"][i]["main"]["temp_max"]);
    }

    if (kToF(root["list"][i]["main"]["temp_min"]) < minTemp)
    {
      minTemp = kToF(root["list"][i]["main"]["temp_min"]);
    }
  }
  
  String obsMax = "MAX T: " + String(maxTemp) + "F";
  String obsMin = "MIN T: " + String(minTemp) + "F";
  u8x2.drawString(0,0,"FUTURE WEATHER");
  u8x2.drawString(0,1, state.c_str());
  u8x2.drawString(0,2, obsMax.c_str());
  u8x2.drawString(0,3, obsMin.c_str());
  u8x2.drawString(0,4,"PRESS ANY KEY");
  char customKey;
  do{
    customKey = customKeypad.getKey();  
    delay(50);
  } while(!customKey);
}

void getWeather()
{
  HTTPClient http;

  u8x2.clear();
  u8x2.drawString(0,0,"GETTING WEATHER");

  http.begin(weatherBaseUrl + key);
  Serial.println(weatherBaseUrl + key);
  int httpCode = http.GET();

  if (httpCode <= 0)
  {
    Serial.println("Error on HTTP request");
    return;
  }

  String payload = http.getString();
  Serial.println(httpCode);
  Serial.println(payload);

  // Allocate JsonBuffer
  // Use arduinojson.org/assistant to compute the capacity.
  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
  DynamicJsonBuffer jsonBuffer(capacity);

  // Parse JSON object
  JsonObject& root = jsonBuffer.parseObject(payload);
  if (!root.success()) {
    Serial.println(F("Parsing failed!"));
    return;
  }

  u8x2.clear();
  String state = root["weather"][0]["main"];
  state.toUpperCase();
  float currTemp = kToF(root["main"]["temp"]);
  float humid = root["main"]["humidity"];
  String obsT = "TEMP:  " + String(currTemp) + "F";
  String obsH = "HUMID: " + String(humid) + " Rel";
  u8x2.drawString(0,0,"CURRENT WEATHER");
  u8x2.drawString(0,1,state.c_str());
  u8x2.drawString(0,2,obsT.c_str());
  u8x2.drawString(0,3,obsH.c_str());
  u8x2.drawString(0,4,"PRESS ANY KEY");
  char customKey;
  do{
    customKey = customKeypad.getKey();  
    delay(50);
  } while(!customKey);
}

void updatePin()
{
  delay(100);
  u8x2.clear();
  u8x2.drawString(0, 0, "INPUT NEW PIN");
  String newPin = "";  
  char customKey = customKeypad.getKey();
  
  while(customKey != '*')
  {
    if (customKey)
    {
      u8x2.home();
      if (customKey != '#')
      {
        newPin += customKey;
        u8x2.clearLine(1);
        u8x2.drawString(0, 1, newPin.c_str());
      }
      else
      {
        u8x2.clear();
        u8x2.drawString(0, 0, "CONFIRM PIN:");
        u8x2.drawString(0, 1, "#=Y, *=Q, Rty");
        u8x2.drawString(0, 2, newPin.c_str());
       
        do{
          customKey = customKeypad.getKey();  
          delay(50);
        } while(!customKey);
       
        if (customKey == '#')
        {
          PinCode = newPin;
          u8x2.clear();
          return;
        }
        else if (customKey == '*')
        {
          u8x2.clear();
          return;
        }
        else
        {
          u8x2.clear();
          u8x2.drawString(0, 0, "INPUT NEW PIN:");
          newPin = "";
        }
      }
    }
    delay(50);
    customKey = customKeypad.getKey();
  }
}

void doCommand()
{
  if (buttons.length() == 1)
  {
    // Do extra command
    switch(buttons.c_str()[0])
    {
      case 'A':
        updatePin();
        break;
        
      case 'B':
        getWeather();
        break;

      case 'C':
        getForecast();
        break;
    }
  }
  else
  {
    // Toggle Alarm
    if (buttons == PinCode)
    {
      if (IsArmed)
      {
        IsArmed = false;
      }
      else
      {
        IsArmed = true;
      }
      
      triggerCount = 0;
      IsTriggered = false;
      SoundAlarm = false;
      prevMotion = LOW;
    }
  }
  buttons = "";
  u8x2.clear();
}

void loop(){
  char customKey = customKeypad.getKey();
  char val = digitalRead(MOTION_PIN);

  if (IsArmed && !IsTriggered)
  {
    if (val == HIGH)
    {
      if (prevMotion == LOW)
      {
        IsTriggered = true;
      }
    }
    else
    {
      if (prevMotion == HIGH)
      {
        IsTriggered = false;
        triggerCount = 0;
      }
    }
    prevMotion = val;
  }

  if (IsArmed && !IsTriggered && !SoundAlarm)
  {
    u8x2.drawString(0, 0, "ALARM: ENABLED");
  }
  else if (SoundAlarm)
  {
    u8x2.drawString(0, 0, "ALARM: SOUNDING!");
  }
  else if (IsTriggered)
  {
    u8x2.drawString(0, 0, "ALARM: TRIGGERED");
  }
  else
  {
    u8x2.drawString(0, 0, "ALARM: DISABLED");
    u8x2.drawString(0, 4, "A=PIN,B=WTHR");
    u8x2.drawString(0, 5, "C=FRCST");
  }

  if (IsArmed && IsTriggered)
  {
    triggerCount += 1;
    if (triggerCount > MAX_TRIGGER_COUNT)
    {
      SoundAlarm = true;
      triggerCount = 0;
    }
  }

  if (SoundAlarm)
  {
    digitalWrite(SPEAKER_PIN, HIGH);
  }
  else
  {
    digitalWrite(SPEAKER_PIN, LOW);
  }

  if (customKey){
    u8x2.home();
    switch(customKey)
    {
      case '#':
        doCommand();
        break;
      
      case '*':
        u8x2.clear();
        buttons = "";
        break;

      default:
        buttons += customKey;
        break;
    }
  }
  
  u8x2.drawString(0, 2, "INPUT KEYS:");
  u8x2.drawString(0, 3, buttons.c_str());
  delay(100);
}
