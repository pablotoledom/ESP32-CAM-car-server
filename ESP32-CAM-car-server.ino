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

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
//#define CAMERA_MODEL_ALTERNATIVE

#include "camera_pins.h"

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

  // Initialize the camera
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
