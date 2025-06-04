#include "esp_camera.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Arduino.h>

#define TFT_BL      4       // PWM pro podsvit displeje
#define BUTTON_PIN  14      // Tlačítko
#define IR_LED_PIN  12      // Výstup pro IR LED (MOSFET)
#define CAM_LED_PIN 4       // GPIO vestavěné LED na ESP32-CAM

TFT_eSPI tft = TFT_eSPI();
int brightness = 128;
bool fadeUp = true;
unsigned long lastFade = 0;

bool buttonHeld = false;
bool irState = false;
unsigned long pressStart = 0;
bool buttonPressed = false;

void setup() {
  Serial.begin(115200);

  // Vypnutí vestavěné LEDky na ESP32-CAM (GPIO 4)
  pinMode(CAM_LED_PIN, OUTPUT);
  digitalWrite(CAM_LED_PIN, LOW);

  // Inicializace displeje
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Inicializace...", 20, 120);

  // PWM pro podsvit
  ledcSetup(1, 5000, 8);
  ledcAttachPin(TFT_BL, 1);
  ledcWrite(1, brightness);

  // Inicializace tlačítka a IR LED
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(IR_LED_PIN, OUTPUT);
  digitalWrite(IR_LED_PIN, LOW);

  // Inicializace kamery
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = 5;
  config.pin_d1       = 18;
  config.pin_d2       = 19;
  config.pin_d3       = 21;
  config.pin_d4       = 36;
  config.pin_d5       = 39;
  config.pin_d6       = 34;
  config.pin_d7       = 35;
  config.pin_xclk     = 0;
  config.pin_pclk     = 22;
  config.pin_vsync    = 25;
  config.pin_href     = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn     = 32;
  config.pin_reset    = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size   = FRAMESIZE_QVGA;
  config.fb_count     = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    tft.fillScreen(TFT_RED);
    tft.drawString("KAMERA CHYBA!", 20, 120);
    while (true);
  }

  tft.fillScreen(TFT_BLACK);
  tft.drawString("Hotovo", 80, 120);
  delay(500);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  handleButton();
  displayCameraFrame();
}

void handleButton() {
  bool pressed = digitalRead(BUTTON_PIN) == LOW;

  if (pressed && !buttonHeld) {
    pressStart = millis();
    buttonHeld = true;
    buttonPressed = false;
  }

  if (pressed && buttonHeld && millis() - pressStart > 400) {
    buttonPressed = true;
    if (millis() - lastFade > 25) {
      brightness += (fadeUp ? 5 : -5);
      brightness = constrain(brightness, 0, 255);
      fadeUp = (brightness >= 255) ? false : (brightness <= 0) ? true : fadeUp;
      ledcWrite(1, brightness);
      lastFade = millis();
    }
  }

  if (!pressed && buttonHeld) {
    buttonHeld = false;
    if (!buttonPressed && millis() - pressStart < 400) {
      irState = !irState;
      digitalWrite(IR_LED_PIN, irState ? HIGH : LOW);
    }
  }
}

void displayCameraFrame() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Chyba: žádný snímek");
    return;
  }

  // Pokud se z kamery dostane RGB565 (např. při přenastavení výstupu), zobraz přímo
  if (fb->format == PIXFORMAT_RGB565) {
    tft.pushImage(0, 0, 240, 240, (uint16_t *)fb->buf);
  } else {
    // Pokud ne, zobraz informaci
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawString("Nespravny format!", 10, 120);
  }

  esp_camera_fb_return(fb);
}