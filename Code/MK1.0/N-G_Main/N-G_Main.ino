#include "esp_camera.h"
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();  // GC9A01 display driver

// OV2640 camera pins (ESP32-CAM AI Thinker)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// === CONFIGURATION ===
const int CAMERA_XCLK_FREQ_HZ = 20000000;
const framesize_t CAMERA_FRAME_SIZE = FRAMESIZE_240X240;   // 240x240 = no scaling
const pixformat_t CAMERA_PIXEL_FORMAT = PIXFORMAT_RGB565;  // Compatible with TFT
const int CAMERA_JPEG_QUALITY = 12;                        // Medium quality
const int CAMERA_FB_COUNT = 2;                             // Double buffer for stability

const int DISPLAY_ROTATION = 0;
// ======================

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;

  config.xclk_freq_hz = CAMERA_XCLK_FREQ_HZ;
  config.pixel_format = CAMERA_PIXEL_FORMAT;
  config.frame_size   = CAMERA_FRAME_SIZE;
  config.jpeg_quality = CAMERA_JPEG_QUALITY;
  config.fb_count     = CAMERA_FB_COUNT;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("[ERROR] Camera initialization failed");
    while (true) delay(100);
  }

  // --- NEW: special filter configuration for OV2640 ---
  sensor_t * s = esp_camera_sensor_get();
  // Set your desired effect here:
  // 0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia
  s->set_special_effect(s, 4);
  // ----------------------------------------------------

  tft.init();
  tft.setRotation(DISPLAY_ROTATION);
  tft.fillScreen(TFT_BLACK);

  Serial.println("System ready.");
}

void loop() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[ERROR] Frame capture failed");
    delay(100);
    return;
  }

  // Check if format is RGB565
  if (fb->format == CAMERA_PIXEL_FORMAT) {
    tft.pushImage(0, 0, fb->width, fb->height, (uint16_t *)fb->buf);
  } else {
    Serial.println("[ERROR] Image format not compatible");
  }

  esp_camera_fb_return(fb);
}
