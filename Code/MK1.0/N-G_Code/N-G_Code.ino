#include "esp_camera.h"
#include <TFT_eSPI.h>
#include <SPI.h>

#define IR_LED_PIN 15      // Výstup pro MOSFET IR LED
#define BUTTON_PIN 0       // Tlačítko GND → aktivní LOW
#define TFT_BL 4           // Podsvícení displeje (PWM)

TFT_eSPI tft = TFT_eSPI();

bool irLedState = false;
bool dimming = false;

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

int brightness = 255;  // 0–255
int fadeDirection = -5; // Směr stmívání (− stmívá, + rozjasňuje)
unsigned long lastFadeTime = 0;
const unsigned long fadeInterval = 30;

void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_QQVGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QQVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Kamera selhala");
  }
}

void setupPWM() {
  ledcSetup(1, 5000, 8);        // Channel 1, 5kHz, 8bit
  ledcAttachPin(TFT_BL, 1);     // BLK pin na PWM
  ledcWrite(1, brightness);     // Start brightness
}

void setup() {
  Serial.begin(115200);

  pinMode(IR_LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  setupPWM();
  digitalWrite(IR_LED_PIN, LOW);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  setupCamera();
}

void loop() {
  static bool lastButtonState = HIGH;
  bool reading = digitalRead(BUTTON_PIN);

  // Detekce hran
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Stisk – krátký (toggle IR LED)
    if (reading == LOW && lastButtonState == HIGH && !dimming) {
      irLedState = !irLedState;
      digitalWrite(IR_LED_PIN, irLedState ? HIGH : LOW);
    }

    // Držení tlačítka – aktivuje stmívání
    if (reading == LOW) {
      dimming = true;
    } else {
      dimming = false;
    }
  }
  lastButtonState = reading;

  // Stmívání, pokud držím tlačítko
  if (dimming && millis() - lastFadeTime > fadeInterval) {
    brightness += fadeDirection;
    if (brightness <= 30 || brightness >= 255) {
      fadeDirection = -fadeDirection;
      brightness = constrain(brightness, 30, 255);
    }
    ledcWrite(1, brightness);
    lastFadeTime = millis();
  }

  // Živý obraz
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Chyba snímku");
    return;
  }

  // Převedení RGB565 obrazu do displeje
  tft.pushImage(0, 0, fb->width, fb->height, (uint16_t *)fb->buf);
  esp_camera_fb_return(fb);
}
