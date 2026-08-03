#include "esp_camera.h"
#include <Arduino.h>

// Camera module pin definitions
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13
#define BUTTON_PIN        D8

// Global variables
camera_config_t config;
int i = 0;
unsigned long lastPressTime = 0;  // Store the time of the last button press
const unsigned long debounceDelay = 5000;  // 5 seconds debounce delay

// configure microcontroller
void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  
  // configure the camera struct
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.frame_size = FRAMESIZE_UXGA;   // assuming PSRAM available
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY; // assuming PSRAM unavailable
  config.fb_location = CAMERA_FB_IN_PSRAM;  // assuming PSRAM available
  config.jpeg_quality = 10; // assuming PSRAM unavailable
  config.fb_count = 1;  // assuming PSRAM unavailable

  // ensure external RAM is being used
  if(psramFound()){
    config.jpeg_quality = 2;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } 
  else {  // reduce pixel resolution when PSRAM isn't available
    config.frame_size = FRAMESIZE_HD;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // init OV3660 using the config struct
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // configure the OV3660 image sensor
  sensor_t *s = esp_camera_sensor_get();
  if(s->id.PID == OV3660_PID){
    s->set_brightness(s, 10); // up the brightness just a bit
  }
  
  // Set exposure time (adjust this value as needed)
  s->set_exposure_ctrl(s, 1); // Enable exposure control
  s->set_aec2(s, 0); // Turn off automatic exposure adjustment
  s->set_agc_gain(s, 0); // Turn off automatic gain control
  s->set_aec_value(s, 2000);

  // Drop down frame size for higher initial frame rate (speed up camera init)
  if(config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }
}



// main loop
void loop() {
  i+=1;
   
  if(Serial.available() > 0){

    // expect String data from the RaspberryPi
    String command = Serial.readStringUntil('\n');
    
    // if the String is recognized, capture image and write its data to serial
    if(command == "TRIGGER"){
        digitalWrite(LED_BUILTIN, LOW);

        // get a pointer to a *frozen* frame buffer in PSRAM
        camera_fb_t *fb = esp_camera_fb_get();
        if(!fb){
          Serial.println("Camera capture failed");
          return;
        }

        // flush serial port before writing to it
        while(Serial.available() > 0){
          Serial.read();
        }

        // Send image data over Serial
        Serial.write((uint8_t*) &fb->len, sizeof(fb->len)); // send length of the image
        Serial.write(fb->buf, fb->len); // send image buffer
        
        // release the **frozen** frame buffer in RAM
        esp_camera_fb_return(fb);

        // Delay for a bit to avoid flooding the serial port
        delay(200);
        digitalWrite(LED_BUILTIN, HIGH);
    }    
  }

  // poll GPIO pin 8 for a button press
  String take_photo = "Take_Photo";
  unsigned long currentTime = millis();
  if((digitalRead(BUTTON_PIN) == HIGH) && (Serial.available() == 0) && (currentTime-lastPressTime > debounceDelay)){

    // instruct RaspberryPi to write "TRIGGER" over serial
    Serial.println(take_photo);
    lastPressTime = currentTime;
  }
}
