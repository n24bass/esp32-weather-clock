// Weather Clock for ESP32-C3 WiFi Toothbrush 
// n24bass@gmail.com
// original - https://github.com/redpower1998/esp8266-st7735-weather-clock
// Hacking the WiFi Toothbrush - https://github.com/atc1441/ATC_Wifi_Toothbrush
// 
// IDE:Arduino
// ArduinoJson 7.1.0
// Adafruit_GFX 1.11.7
// Adafruit_ST7735  1.10.3
// ArduinoOTA 1.1.0

// 代码没有被检查，没有删除无用代码，甚至没有格式化。所以代码质量凑合看吧。 
// 代码发布在https://gitee.com/redpower/esp8266-st7735-weather-clock  
// IDE:Arduino
// 依赖库(通过Arduino安装):
// ArduinoJson 5.10.1
// Adafruit_GFX 1.10.6
// Adafruit_ST7735  1.7.0

// 开发板 NodeMCU 1.0  ESP-12E  显示屏1.4TFT
// 波特率115200

#define ESP32_RTOS

#include <WiFi.h> // #include <ESP8266WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <time.h>
#include <Ticker.h>

#include <HTTPClient.h> // #include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

#include <Fonts/Tiny3x3a2pt7b.h>
#include <Fonts/FreeSansOblique9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/TomThumb.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/Org_01.h>
#include <Fonts/FreeSansBoldOblique24pt7b.h>

#include "icons.h"
#include "configure.h"  //Please modify this file,It is configure file.

#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFDA0      /* 255, 180,   0 */
#define TFT_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define TFT_PINK        0xFC9F

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_SDI, TFT_CLK, TFT_RST); // uses RGB565

// set of variables used into this sketch for different pourpose
int weatherID = 0;

#if defined(ESP32_RTOS) && defined(ESP32)
void ota_handle( void * parameter ) {
  for (;;) {
    ArduinoOTA.handle();
    delay(500); // 3500);
  }
}
#endif

void setupOTA(const char* nameprefix, const char* ssid, const char* password) {
  // Configure the hostname
  uint16_t maxlen = strlen(nameprefix) + 7;
  char *fullhostname = new char[maxlen];
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(fullhostname, maxlen, "%s-%02x%02x%02x", nameprefix, mac[3], mac[4], mac[5]);
  ArduinoOTA.setHostname(fullhostname);
  delete[] fullhostname;

  // Configure and start the WiFi station
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    // Serial.println("Connection Failed! Rebooting...");
    tft.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232); // Use 8266 port if you are working in Sloeber IDE, it is fixed there and not adjustable

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    //NOTE: make .detach() here for all functions called by Ticker.h library - not to interrupt transfer process in any way.
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("\nAuth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("\nBegin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("\nConnect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("\nReceive Failed");
    else if (error == OTA_END_ERROR) Serial.println("\nEnd Failed");
  });

  ArduinoOTA.begin();

  Serial.println("OTA Initialized");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

#if defined(ESP32_RTOS) && defined(ESP32)
  xTaskCreate(
    ota_handle,          /* Task function. */
    "OTA_HANDLE",        /* String with name of task. */
    10000,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    1,                /* Priority of the task. */
    NULL);            /* Task handle. */
#endif
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  pinMode(TFT_SDI, OUTPUT);
  analogWrite(TFT_SDI, 50);
  lcd_init();

  configTime(timezone * 3600, 0, "pool.ntp.org"); // (utc/gmt offset, daylight saving offset, ntp server)
  Serial.println("\nWaiting for time sync");
  while (!time(nullptr))
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Synced");
  tft.fillScreen(ST77XX_BLACK);
  if (Images[1][1] > 0)
    Serial.println("Hello world");
}

void lcd_init()
{
  pinMode(18, OUTPUT); // LCD enable
  digitalWrite(18, HIGH); 
  pinMode(19, OUTPUT); // LCD Backlight
  digitalWrite(19, LOW);

  tft.initR(INITR_BLACKTAB);
  tft.invertDisplay(true);
  tft.fillScreen(ST77XX_BLACK);
  delay(200);

  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_WHITE);
  tft.setTextWrap(false);
  tft.println("System Init...");

  setupOTA("WeatherClock", ssid, password);

  //If connection successful show IP address in serial monitor
  tft.println("");
  tft.println("Connected @ IP:");
  tft.println(WiFi.localIP());  //IP address assigned to your ESP
  tft.setFont(&FreeSans12pt7b);
}

void clockDisplay()
{
  static int old_day;
  static int old_min;
  char buf[9];
  String displayTime;

  time_t now = time(nullptr);

  struct tm * timeinfo;
  timeinfo = localtime (&now);
  if (timeinfo->tm_year == 70)
  {
    tft.setFont();
    tft.fillRect(30, 0, 80, 49, ST77XX_BLACK);
    tft.setCursor(33, 22);
    tft.setTextColor(TFT_YELLOW);
    tft.println("Sync Time...");
    // tft.drawFastHLine(0, 50, 128, ST77XX_GREEN); //Line
    Serial.println("Sync Time...");
    return;
  }
  tft.setFont();
  tft.setCursor(43, 12); // (58, 12);
  tft.setTextColor(ST7735_WHITE);
  tft.setFont(&FreeSansOblique9pt7b);
  if (old_day != timeinfo->tm_mday)
  {
    old_day = timeinfo->tm_mday;
    tft.fillRect(30, 0, 80, 49, ST77XX_BLACK);
  }
  sprintf(buf, "%02d/%02d", timeinfo->tm_mon + 1, timeinfo->tm_mday);
  displayTime = buf;
  tft.println(displayTime);
  Serial.println(displayTime);

  // tft.drawFastHLine(0, 50, 128, ST77XX_GREEN); //Line

  if (old_min != timeinfo->tm_min)
  {
    old_min = timeinfo->tm_min;
    tft.fillRect(30, 25, 80, 25, ST77XX_BLACK);
  }
  tft.fillRect(75, 25, 40, 25, ST77XX_BLACK);
  sprintf(buf, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  displayTime = buf;
  tft.setCursor(30, 40);
  // tft.setTextColor(ST7735_YELLOW);
  // tft.setFont(&FreeSans9pt7b);
  tft.println(displayTime);
  Serial.println(displayTime);

}

void drawBmp(int id)
{
  tft.fillRect(48, 50, 40, 28, ST77XX_BLACK);
  tft.drawBitmap(48, 50, &Images[id][0], 40, 28, ST77XX_WHITE); // x, y, bitmap, w, h, color
}
bool isDay()
{
  time_t now = time(nullptr);
  struct tm * timeinfo = localtime (&now);
  return (timeinfo->tm_hour > 5 && timeinfo->tm_hour < 19 );
}
// Print WeatherIcon based on id
void printWeatherIcon(int id) {
  switch (id) {
    case 800:   //Clear
      if (isDay())
        drawBmp(0);
      else
        drawBmp(33);
      break;
    case 801:  //few clouds
    case 802: // scattered clouds
    case 803: // broken clouds
      if (isDay())
        drawBmp(1);
      else
        drawBmp(34);
      break;
    case 804: //overcast clouds
      if (isDay())
        drawBmp(2);
      else
        drawBmp(35);
      break;

    case 200: drawBmp(4); break; //thunderstorm with light rain
    case 201: drawBmp(4); break; //thunderstorm with rain
    case 202: drawBmp(5); break; //thunderstorm with heavy rain
    case 210: drawBmp(4); break; //light thunderstorm
    case 211: drawBmp(4); break;  //thunderstorm
    case 212: drawBmp(5); break; //heavy thunderstorm
    case 221: drawBmp(5); break;  //ragged thunderstorm
    case 230: drawBmp(4); break;  //thunderstorm with light drizzle
    case 231: drawBmp(4); break; //thunderstorm with drizzle
    case 232: drawBmp(4); break;  //thunderstorm with heavy drizzle

    case 300: drawBmp(3); break;  //light intensity drizzle
    case 301: drawBmp(7); break;  //drizzle  09d
    case 302: drawBmp(8); break;  //heavy intensity drizzle
    case 310: drawBmp(7); break;  //light intensity drizzle rain
    case 311: drawBmp(7); break;  //drizzle rain
    case 312: drawBmp(8); break;  //heavy intensity drizzle rain
    case 313: drawBmp(7); break;  //shower rain and drizzle
    case 314: drawBmp(8); break;  //heavy shower rain and drizzle
    case 321: drawBmp(8); break;  //shower drizzle

    case 500: drawBmp(8); break;  //light rain
    case 501: drawBmp(9); break;  //moderate rain
    case 502: drawBmp(10); break;  //heavy intensity rain
    case 503: drawBmp(11); break;//very heavy rain
    case 504: drawBmp(12); break;  //extreme rain
    case 511: drawBmp(6); break;  //freezing rain
    case 520: drawBmp(7); break;     //light intensity shower rain
    case 521: drawBmp(8); break;  //shower rain
    case 522: drawBmp(9); break;  //heavy intensity shower rain
    case 531: drawBmp(10); break; //ragged shower rain

    case 600: drawBmp(14); break;  //light snow
    case 601: drawBmp(15); break;  //Snow
    case 602: drawBmp(16); break;  //Heavy snow
    case 611: drawBmp(29); break;  //Sleet
    case 612: drawBmp(29); break;  //Light shower sleet
    case 613: drawBmp(29); break;  //Shower sleet
    case 615: drawBmp(6); break;  //Light rain and snow
    case 616: drawBmp(6); break;  //Rain and snow
    case 620: drawBmp(6); break; //Light shower snow
    case 621: drawBmp(14); break;  //Shower snow
    case 622: drawBmp(14); break; //Heavy shower snow

    case 701: drawBmp(18); break;  //mist
    case 711: drawBmp(18); break;  //Smoke
    case 721: drawBmp(32); break;  //Haze
    case 731: drawBmp(20); break;  //sand/ dust whirls
    case 741: drawBmp(18); break;  //fog
    case 751: drawBmp(29); break;  //sand
    case 761: drawBmp(29); break;  //dust
    case 762: drawBmp(29); break;  //Ash volcanic ash
    case 771: drawBmp(30); break;  //squalls
    case 781: drawBmp(31); break;  //tornado
    default: break;
  }
}
// get Weather data from openweathermap.org
// sent request for data
void getWeatherData() { //client function to send/receive GET request data.
  String result;
  WiFiClient client;    // WIFI Client
  char servername[] = "api.openweathermap.org";  // remote server we will connect to
  
  if (client.connect(servername, 80)) {  //starts client connection, checks for connection
    client.println("GET /data/2.5/weather?units=metric&id=" + CityID + "&APPID=" + APIKEY);
    client.println("Host: api.openweathermap.org");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
  } else {
    Serial.println("connection failed"); //error message if no client connect
    Serial.println();
    return;
  }
  Serial.println("Connect.....");
  // reading sent data
  int count = 0;
  while (client.connected() && !client.available()) {
    count++;
    delay(10);
    if (count > 2000)
      return;
  }
  Serial.println("Waiting for data");

  while (client.connected() || client.available()) { //connected or data available
    char c = client.read(); //gets byte from ethernet buffer
    if (c <= 0x1F || c >= 0x7F)
      return;
    result = result + c;
  }

  client.stop(); //stop client
  Serial.println(result);

  // format received data into a jsonArray.
  // to make this code working it has been becessary to install version
  char jsonArray [result.length() + 1];
  result.toCharArray(jsonArray, sizeof(jsonArray));
  jsonArray[result.length() + 1] = '\0';
  // StaticJsonBuffer<2048> json_buf;
  // JsonObject &root = json_buf.parseObject(jsonArray);
  JsonDocument doc; 
  DeserializationError error = deserializeJson(doc, jsonArray);
  if (error) { // (!root.success()) {
    // Serial.println("parseObject() failed");
    Serial.print("deserializeJson() returned ");
    Serial.println(error.c_str());
    return ;
  }

  //TODO : try to understand why this double assignement is necessary
  String temperatureLOC = doc["main"]["temp"]; // root["main"]["temp"];
  String weatherLOC = doc["weather"][0]["main"]; // root["weather"][0]["main"];
  String descriptionLOC = doc["weather"][0]["description"]; // root["weather"][0]["description"];
  String idStringLOC = doc["weather"][0]["id"]; // root["weather"][0]["id"];
  String umidityPerLOC = doc["main"]["humidity"]; // root["main"]["humidity"];
  String windLOC = doc["wind"]["speed"]; // root["wind"]["speed"];
  String pressureLOC = doc["main"]["pressure"]; // root["main"]["pressure"];
  String visibilityLOC = doc["visibility"]; // root["visibility"];
  String wind_angleLOC = doc["wind"]["deg"]; // root["wind"]["deg"];
  String cloudsLOC = doc["clouds"]["all"]; //["main"] // root ["clouds"]["all"] ;//["main"]
  String temp_max = doc["main"]["temp_max"]; // root["main"]["temp_max"];
  String temp_min = doc["main"]["temp_min"]; // root["main"]["temp_min"];

  String sunrise =  doc["sys"]["sunrise"]; // root["sys"]["sunrise"];
  String sunset =  doc["sys"]["sunset"]; // root["sys"]["sunset"];
  String city_name =  doc["name"]; // root["name"];
  time_t sun_time = sunrise.toInt();
  char buf[6];
  struct tm * timeinfo = localtime (&sun_time);
  sprintf(buf, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
  sunrise = buf;

  sun_time = sunset.toInt();
  timeinfo = localtime (&sun_time);
  sprintf(buf, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
  sunset = buf;

  weatherID = idStringLOC.toInt();
  printWeatherIcon(weatherID);

  tft.fillRect(30, 80, 80, 80, ST77XX_BLACK);
  tft.setCursor(0, 84); 
  tft.setFont();
  tft.setTextColor(ST7735_YELLOW);

  tft.println("     T:" + temperatureLOC + " (" + temp_min + "-" + temp_max + ")");
  tft.println("     W:" + weatherLOC);
  tft.println("     D:" + descriptionLOC + ":" + idStringLOC);
  tft.println("     W:" + wind_angleLOC + " Speed:" + windLOC);
  tft.println("     C:" + cloudsLOC + " Visi:" + visibilityLOC);
  tft.println("     H:" + umidityPerLOC);
  tft.println("     S:" + sunrise + "-" + sunset);
  tft.setTextColor(ST7735_GREEN);

  time_t cur_time = time(nullptr);
  timeinfo = localtime (&cur_time);
  sprintf(buf, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
  sunrise = buf;

  tft.println("       " + city_name);
  tft.println("       " + sunrise);
}

void loop()
{
  int count = 15 * 60;
  if (weatherID == 0 )
  {
    count = 20;
  }
  for (int i = 0; i < count; i++)
  {
    clockDisplay();
    delay(1000);
  }
  tft.setFont();
  tft.fillRect(30, 0, 80, 49, ST77XX_BLACK);
  tft.setCursor(43, 16);
  tft.setTextColor(TFT_RED);
  Serial.println("Update Weather");
  tft.print("Update");
  tft.setCursor(43, 26);
  tft.print("Weather...");
  getWeatherData();
  tft.fillRect(30, 0, 80, 49, ST77XX_BLACK);
  clockDisplay();
}
