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

// ----------------------------- Start Imports -----------------------------

#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "Arduino.h"
#include "web_index.h"
#include <FS.h>
#include <SPIFFS.h>
#define FILE_PHOTO "/photo.jpg"
#include <WiFi.h>

// ----------------------------- End Imports -----------------------------


// --------------------------- Start Declarations ---------------------------

// Digital outputs
extern int gpLf;     // Left Forward
extern int gpLb;     // Left Backward
extern int gpRf;     // Right Forward
extern int gpRb;     // Right Backward
extern int gpLed;    // Light
extern int gpClaxon; // Claxon

// Network IP
extern String WiFiAddr;

//void WheelAct(int nLf, int nLb, int nRf, int nRb);

// Camera stream declarations
typedef struct {
        size_t size; //number of values used for filtering
        size_t index; //current value index
        size_t count; //value count
        int sum;
        int * values; //array to be filled with values
} ra_filter_t;

typedef struct {
        httpd_req_t *req;
        size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static ra_filter_t ra_filter;
httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

// --------------------------- End Declarations ---------------------------


// ---------------------------- Start methods ----------------------------

// Save char format data to file in SPIFFS or SD Memory
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  // Declare and open file variaable
  File file = fs.open(path, "w");

  // Validate file variaable
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }

  // Write on file variaable
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }

  // Close file variable to leave memory
  file.close();
}

// Read char format data file from SPIFFS or SD Memory 
extern String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  // Declare and open file variaable
  File file = fs.open(path, "r");

  // Validate file variaable
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }

  // Cast data from file
  String fileContent;
  while(file.available()){
    fileContent+=(char)file.read();
  }

  // Close file variable to leave memory
  file.close();

  // Return content
  return fileContent;
}

// Web serve video stream on jpeg
static esp_err_t stream_handler(httpd_req_t *req){
  // Example from oficial website: https://github.com/espressif/esp32-camera/blob/master/README.md
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len;
  uint8_t * _jpg_buf;
  char * part_buf[64];
  static int64_t last_frame = 0;
  if(!last_frame) {
    last_frame = esp_timer_get_time();
  }

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

  while(true){
    fb = esp_camera_fb_get();
    if (!fb) {
      ESP_LOGE(TAG, "Camera capture failed");
      res = ESP_FAIL;
      break;
    }
    if(fb->format != PIXFORMAT_JPEG){
      bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
      if(!jpeg_converted){
          ESP_LOGE(TAG, "JPEG compression failed");
          esp_camera_fb_return(fb);
          res = ESP_FAIL;
      }
    } else {
      _jpg_buf_len = fb->len;
      _jpg_buf = fb->buf;
    }

    if(res == ESP_OK){
       res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(fb->format != PIXFORMAT_JPEG){
      free(_jpg_buf);
    }
    esp_camera_fb_return(fb);
    if(res != ESP_OK){
      break;
    }
    int64_t fr_end = esp_timer_get_time();
    int64_t frame_time = fr_end - last_frame;
    last_frame = fr_end;
    frame_time /= 1000;
    ESP_LOGI(TAG, "MJPG: %uKB %ums (%.1ffps)",
      (uint32_t)(_jpg_buf_len/1024),
      (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
  }

  last_frame = 0;
  return res;
}

// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  // Open file variable from SPIFFS or SD memory
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  // Return true or false if file has content
  return ( pic_sz > 100 );
}

// Capture Photo and Save it to SPIFFS
void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly

  do {
    // Take a photo with the camera
    Serial.println("Taking a photo...");

    // Open camera framebuffer
    fb = esp_camera_fb_get();

    // Validate framebuffer content
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    // Declare file name for capture
    Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

    // Insert the data in the photo file
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }

    // Save framebuffer to file
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    // Close file variable to leave memory
    file.close();

    // Close framebuffer variable to leave memory
    esp_camera_fb_return(fb);

    // check if file has been correctly saved in SPIFFS
    ok = checkPhoto(SPIFFS);

    // When its ok, this function end
  } while ( !ok );
}

// Method of changing the digital outputs from high to low and vice versa
// depending on the movement required to move the motors
void WheelAct(int nLf, int nLb, int nRf, int nRb) {
  digitalWrite(gpLf, nLf);
  digitalWrite(gpLb, nLb);
  digitalWrite(gpRf, nRf);
  digitalWrite(gpRb, nRb);
}

// Method return main website for controller Remote WIFI Car
static esp_err_t index_handler(httpd_req_t *req){
  // Read configuration data files, add data to response request
  String sta_ssid = readFile(SPIFFS, "/wifi_ssid.txt");
  String sta_pass = readFile(SPIFFS, "/wifi_pass.txt");
  String sta_usercontrol = readFile(SPIFFS, "/user_control.txt");

  Serial.println(sta_ssid);
  Serial.println(sta_pass);
  Serial.println(sta_usercontrol);
    
  // Set http response object
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_set_hdr(req, "Usercontrol", sta_usercontrol.c_str());
  httpd_resp_set_hdr(req, "Wifissid", sta_ssid.c_str());
  httpd_resp_set_hdr(req, "Wifipass", sta_pass.c_str());
  httpd_resp_set_hdr(req, "Server", WiFiAddr.c_str());
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Accept, X-Requested-With, X-File-Name");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST");
  return httpd_resp_send(req, reinterpret_cast<char*>(frontend_html_gz), frontend_html_gz_len);
}

// Asyncrhonous method to control wheels and save configuration
static esp_err_t cmd_handler(httpd_req_t *req){
  // Methos variables declarations
  char*  buf;
  size_t buf_len;
  char command[32] = {0,};
  char value[32] = {0,};
  char ssid[32] = {0,};
  char password[32] = {0,};
  char usercontrol[32] = {0,};
  int res = 0;

  // Get data from url query string
  buf_len = httpd_req_get_url_query_len(req) + 1;

  // Validate data query string
  if (buf_len > 1) {

    // Dynamic asignation of memory
    buf = (char*)malloc(buf_len);

    // Validate buffer
    if (!buf) {
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }

    // Validate query string exist
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      // Validate that the specific query strings are present
      if (httpd_query_key_value(buf, "command", command, sizeof(command)) == ESP_OK &&
          httpd_query_key_value(buf, "value", value, sizeof(value)) == ESP_OK) {
      } else if (httpd_query_key_value(buf, "command", command, sizeof(command)) == ESP_OK &&
          httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid)) == ESP_OK &&
          httpd_query_key_value(buf, "password", password, sizeof(password)) == ESP_OK &&
          httpd_query_key_value(buf, "usercontrol", usercontrol, sizeof(usercontrol)) == ESP_OK) {
      } else {
        // Leave memory and respose error
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    } else {
      // Leave memory and respose error
      free(buf);
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    // Leave memory ever
    free(buf);
  } else {
    // Leave memory and respose error
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  // Cast char to integer
  int val = atoi(value);

  // Look at values within URL to determine function
  // Change camera quality
  if (!strcmp(command, "framesize")) {
    Serial.println("framesize");
    sensor_t * s = esp_camera_sensor_get();
    if (s->pixformat == PIXFORMAT_JPEG) res = s->set_framesize(s, (framesize_t)val);
  }
  // Save config to memory
  else if (!strcmp(command, "saveconfig")){
    Serial.println(ssid);
    Serial.println(password);
    Serial.println(usercontrol);

    writeFile(SPIFFS, "/wifi_ssid.txt", ssid);
    writeFile(SPIFFS, "/wifi_pass.txt", password);
    writeFile(SPIFFS, "/user_control.txt", usercontrol);

    Serial.println("WIFI and configuration saved!, please restart ESP32");
  }
  // Car wheels control
  else if (!strcmp(command, "car")) {
    if (!strcmp(value, "forward")) {
      Serial.println("Forward");
      WheelAct(HIGH, LOW, HIGH, LOW);
    }
    else if (!strcmp(value, "turnleft")) {
      Serial.println("TurnLeft");
      WheelAct(HIGH, LOW, LOW, HIGH);
    }
    else if (!strcmp(value, "stop")) {
      Serial.println("Stop");
       WheelAct(LOW, LOW, LOW, LOW);
    }
    else if (!strcmp(value, "turnright")) {
      Serial.println("TurnRight");
      WheelAct(LOW, HIGH, HIGH, LOW);
    }
    else if (!strcmp(value, "backward")) {
      Serial.println("Backward");
      WheelAct(LOW, HIGH, LOW, HIGH);
    }
    else if (!strcmp(value, "lighton")) {
      Serial.println("Led on");
      digitalWrite(gpLed, HIGH);
    }
    else if (!strcmp(value, "lightoff")) {
      Serial.println("Led off");
      digitalWrite(gpLed, LOW);
    }
    else if (!strcmp(value, "claxonon")) {
      Serial.println("Claxon on");
      digitalWrite(gpClaxon, HIGH);
    }
    else if (!strcmp(value, "claxonoff")) {
      Serial.println("Claxon off");
      digitalWrite(gpClaxon, LOW);
    }
  }
  else
  {
    Serial.println("command");
    res = -1;
  }

  if (res) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

// Asyncrhonous method to capture photo from cam and save into SPIFFS
static esp_err_t takephoto_handler(httpd_req_t *req){
  capturePhotoSaveSpiffs();
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, "OK", 2);
}

// Asyncrhonous method to capture photo from cam return into website call
static esp_err_t getphoto_handler(httpd_req_t *req){
  // Variable Declarations
  // Set initial status response
  esp_err_t res = ESP_OK;
  // Framebuffer
  camera_fb_t * fb = NULL;
  // Boolean indicating if the picture has been taken correctly
  bool ok = 0;

  // Take a photo with the camera and set to framebuffer
  fb = esp_camera_fb_get();

  // Validate framebuffer
  if (!fb) {
    Serial.println("Camera capture failed");
    return ESP_FAIL;
  }

  // Set http response object
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=photo.jpg");
  res = httpd_resp_send(req, (const char*)fb->buf,fb->len);
  
  // Close framebuffer variable to leave memory
  esp_camera_fb_return(fb);
  fb = NULL;

  // Return fianl response
  return res;
}

// Asyncrhonous method return a wifi scan list
static esp_err_t wifilist_handler(httpd_req_t *req){
  Serial.println("scan start");

  String wifi_list_csv_resp = "";

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } else {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {
          // Print SSID and RSSI for each network found
          Serial.print(i + 1);
          Serial.print(": ");
          Serial.print(WiFi.SSID(i));
          Serial.print(" (");
          Serial.print(WiFi.RSSI(i));
          Serial.print(")");
          Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
          wifi_list_csv_resp += WiFi.SSID(i) + ";" + WiFi.RSSI(i) + ";" + ((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "OPEN" : "NEED-PASS") + "\r\n";
          delay(10);
      }
  }
 
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, &wifi_list_csv_resp[0], strlen(&wifi_list_csv_resp[0]));
}

// Main method to define valid url list
void startCameraServer(){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t takephoto_uri = {
        .uri       = "/take_photo",
        .method    = HTTP_GET,
        .handler   = takephoto_handler,
        .user_ctx  = NULL
    };
   
    httpd_uri_t photo_uri = {
        .uri       = "/get_photo",
        .method    = HTTP_GET,
        .handler   = getphoto_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t cmd_uri = {
        .uri       = "/control",
        .method    = HTTP_GET,
        .handler   = cmd_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t wifilist_uri = {
        .uri       = "/wifilist",
        .method    = HTTP_GET,
        .handler   = wifilist_handler,
        .user_ctx  = NULL
    };

    //ra_filter_init(&ra_filter, 20);

    // Mount http for web server
    Serial.printf("Starting web server on port: '%d'", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &cmd_uri);
        httpd_register_uri_handler(camera_httpd, &takephoto_uri);
        httpd_register_uri_handler(camera_httpd, &photo_uri);
        httpd_register_uri_handler(camera_httpd, &wifilist_uri);
    }

    // Mount http for cam stream server
    config.server_port += 1;
    config.ctrl_port += 1;
    Serial.printf("Starting stream server on port: '%d'", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}

// ---------------------------- End methods ----------------------------
