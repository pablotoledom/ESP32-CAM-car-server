// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// ESP32 Remote WIFI Car

// ----------------------------- Start Imports -----------------------------

#include "esp_camera.h"
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>

extern "C" {
  #include "crypto/base64.h"
}

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_M5STACK_NO_PSRAM
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_WROVER_KIT)
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    21
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      19
#define Y4_GPIO_NUM      18
#define Y3_GPIO_NUM       5
#define Y2_GPIO_NUM       4
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

#elif defined(CAMERA_MODEL_M5STACK_NO_PSRAM)
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM 15
#define XCLK_GPIO_NUM 27
#define SIOD_GPIO_NUM 25
#define SIOC_GPIO_NUM 23
#define Y9_GPIO_NUM 19
#define Y8_GPIO_NUM 36
#define Y7_GPIO_NUM 18
#define Y6_GPIO_NUM 39
#define Y5_GPIO_NUM 5
#define Y4_GPIO_NUM 34
#define Y3_GPIO_NUM 35
#define Y2_GPIO_NUM 17
#define VSYNC_GPIO_NUM 22
#define HREF_GPIO_NUM 26
#define PCLK_GPIO_NUM 21

#elif defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32  // power to camera (on/off)
#define RESET_GPIO_NUM    -1  // -1 = not used
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26  // i2c sda
#define SIOC_GPIO_NUM     27  // i2c scl

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25  // vsync_pin
#define HREF_GPIO_NUM     23  // href_pin
#define PCLK_GPIO_NUM     22  // pixel_clock_pin

#else
#error "Camera model not selected"
#endif

extern String WiFiAddr ="";
extern String readFile(fs::FS &fs, const char * path);
void startCameraServer();

// ----------------------------- End Imports -----------------------------


// --------------------------- Start Declarations ---------------------------

// Access Point WIFI car and network named
const char *ap_ssid = "Remote WIFI car";
const char *ap_password = "12345678";
String hostname = "Remote WIFI Car";

// GPIO Setting
extern int gpLb =  2; // Left 1
extern int gpLf = 14; // Left 2
extern int gpRb = 15; // Right 1
extern int gpRf = 13; // Right 2
extern int gpLed =  4; // Light
extern int gpClaxon =  12; // Claxon

// Timer for WIFI connect
unsigned long previousMillis = 0;

// --------------------------- End Declarations ---------------------------


// -------------------------- Start main methods --------------------------

void setup() {
  // UART Comunication setup
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Check SPIFFS initialize
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Pins
  pinMode(gpLf, OUTPUT);     // Left Forward
  pinMode(gpLb, OUTPUT);     // Left Backward
  pinMode(gpRf, OUTPUT);     // Right Forward
  pinMode(gpRb, OUTPUT);     // Right Backward
  pinMode(gpLed, OUTPUT);    // Light
  pinMode(gpClaxon, OUTPUT); // Claxon

  // Outputs initialize off
  digitalWrite(gpLf, LOW);      // Left Forward
  digitalWrite(gpLb, LOW);      // Left Backward
  digitalWrite(gpRf, LOW);      // Right Forward
  digitalWrite(gpRb, LOW);      // Right Backward
  digitalWrite(gpLed, LOW);     // Light
  digitalWrite(gpClaxon, LOW);  // Claxon

  // Initialise the camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  //drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_CIF);

  
  Serial.print("Configuring access point...");

  // Get WIFI network ssid and pass
  String sta_ssid = readFile(SPIFFS, "/wifi_ssid.txt");
  String sta_password = readFile(SPIFFS, "/wifi_pass.txt");

  // Cast data to char
  const char* ssidchar = sta_ssid.c_str();
  const char* passchar = sta_password.c_str();

  // Decode ssi and password saved in Base64
  size_t outputLength1;
  size_t outputLength2;
  unsigned char * decodedssid = base64_decode((const unsigned char *)ssidchar, strlen(ssidchar), &outputLength1);
  unsigned char * decodedpass = base64_decode((const unsigned char *)passchar, strlen(passchar), &outputLength2);

  // Cast data to string
  String ssidstring = reinterpret_cast<const char*>(decodedssid);
  String passstring = reinterpret_cast<const char*>(decodedpass);

  // first, set NodeMCU as STA mode to connect with a Wifi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidstring.c_str(), passstring.c_str());
  Serial.println("");
  Serial.print("Connecting to: ");
  Serial.println(sta_ssid);
  Serial.print("Password: ");
  Serial.println(sta_password);

  // try to connect with Wifi network about 10 seconds
  unsigned long currentMillis = millis();
  previousMillis = currentMillis;
  while (WiFi.status() != WL_CONNECTED && currentMillis - previousMillis <= 10000) {
    delay(500);
    Serial.print(".");
    currentMillis = millis();
  }

  // if failed to connect with Wifi network set NodeMCU as AP mode
  IPAddress myIP;
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("*WiFi-STA-Mode*");
    Serial.print("IP: ");
    myIP=WiFi.localIP();
    Serial.println(myIP);
    delay(2000);
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(hostname.c_str());
    myIP = WiFi.softAPIP();
    Serial.println("");
    Serial.println("WiFi failed connected to " + sta_ssid);
    Serial.println("");
    Serial.println("*WiFi-AP-Mode*");
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    delay(2000);
  }

  // Start the Web server for play with the car
  startCameraServer();
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(myIP);
  WiFiAddr = myIP.toString();
  Serial.println("' to connect");
}
  
void loop() {
  // put your main code here, to run repeatedly:
}
