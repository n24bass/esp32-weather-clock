#ifndef CONFIGURE_HH
#define CONFIGURE_HH

//SSID and Password of your WiFi router
const char* ssid = "XXXXXXXXx";
const char* password = "XXXXXXXXXXX";

String APIKEY = "XXXXXXXXXXXXXXXXX"; //openweathermap.org APIkey
String CityID = "XXXXXXX"; // 2128295 for Sapporo - ID not name

const int timezone = 9; // 9 for JAPAN
int dst = 0;

// Pins for ESP32-C3 WiFi Toothbrush
#define TFT_CS     10
#define TFT_RST    -1
#define TFT_DC     8
#define TFT_SDI    7
#define TFT_CLK    6

#endif
