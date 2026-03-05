#include <Arduino.h>
#include <M5Unified.h>

#include <WiFi.h>
#include <FastLED.h>
#include <time.h> 
#include <LittleFS.h>

#define LED_PIN 1       // Atom S3 grove connector LED pin
#define NUM_LEDS 43 // (this just happens to be the number of LED's I am exposing)

CRGB leds[NUM_LEDS];

const char* ssid     = "LuciernagaGoogleWifi";
const char* password = "xxxxxxxx";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -8 * 3600;   // PST (change for your zone)
const int   daylightOffset_sec = 3600;   // DST (or 0 if not used)

#define LED_SHIMMER_BLUE 0
#define LED_RAINBOW      1
#define LED_BREATHING    2
#define LED_BLUE         3
#define LED_WHITE        4
#define LED_OFF          5

static const char * const COLOR_NAMES[] = {
  "Shimmer",
  "Rainbow",
  "Breathing",
  "Blue",
  "White",
  "Off"
};

/* set initial LED color */
uint8_t ledMode = LED_WHITE;
uint8_t hue = 0;

uint16_t brightness = 100;

M5Canvas sprite(&M5.Display);

void syncTime()
{
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);  

  Serial.print("Waiting for NTP time sync");

  struct tm timeinfo;
  int retries = 0;

  while (!getLocalTime(&timeinfo) && retries < 20) {
    Serial.print(".");
    delay(500);
    retries++;
  }

  if (retries < 20) {
    Serial.println("\nTime synchronized!");
  } else {
    Serial.println("\nNTP sync failed.");
  }
}

void setup() {
  // put your setup code here, to run once:
  auto cfg = M5.config();
  M5.begin(cfg);

  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed");
    return;
  }

  // ---- FastLED Init ----
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);

  Serial.begin(115200);
  Serial.println("Hello world!");

  // Create sprite the same size as screen
  sprite.setColorDepth(16);      // 16-bit color (safe choice)
  sprite.createSprite(
      M5.Display.width(),
      M5.Display.height()
  );

  FastLED.showColor(CRGB::Blue);
  FastLED.show();

  /* start Wifi in station mode (connect to home wifi) */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  syncTime();
}

void updateLED() {
  switch (ledMode) {
    case LED_SHIMMER_BLUE: // Solid status
      FastLED.showColor(CRGB::Blue);
      break;

    case LED_RAINBOW: // Rainbow
      for(int n = 0; n < NUM_LEDS; n++) {
        leds[n] = CHSV(hue + n * 10, 255, 255);
      }
      break;

    case LED_BREATHING: // Breathing
    {
      uint8_t b = beatsin8(20, 10, 255);
      for(int n = 0; n < NUM_LEDS; n++) {
        leds[n] = CRGB(0, b, b);
      }
      break;
    }

    case LED_BLUE:
    {
      fill_solid(leds, NUM_LEDS, CRGB::Blue);
      break;
    }

    case LED_WHITE:
    {
      fill_solid(leds, NUM_LEDS, CRGB::AntiqueWhite);
      break;
    }
    case LED_OFF:
    {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      break;
    }    
  }
  FastLED.show();
}

void loop() {
  static uint32_t lastMillis = 0;
  static uint32_t displayTimeoutMillis = 0;
  static struct tm timeinfo;

  updateLED();
  M5.update();

  sprite.fillSprite(BLACK);
  sprite.setTextSize(1);
  sprite.setTextColor(WHITE);

  /* if time is set then display it */
  if(timeinfo.tm_year > 0) {
    sprite.setTextSize(2);
    sprite.setCursor(10, 0);
    sprite.println(&timeinfo, "%Y-%m-%d %H:%M:%S");        

    /* timed events: DO THIS BETTER! */
    if(timeinfo.tm_hour == 07 && timeinfo.tm_min == 05 && timeinfo.tm_sec < 5) {      
      ledMode = LED_WHITE;
    }

    if(timeinfo.tm_hour == 22 && timeinfo.tm_min == 00 && timeinfo.tm_sec < 5) {      
      ledMode = LED_BLUE;
    }    

  }

  sprite.setCursor(6, 64);
  sprite.setTextSize(2);
  sprite.println(COLOR_NAMES[ledMode]);
  sprite.drawRect(5, 25, 118, 70, BLUE);  

  /* check for button presses or holdings */
  if(M5.BtnA.isHolding()) {
    displayTimeoutMillis = millis();
    Serial.println("Holding!");
    brightness ++;
    FastLED.setBrightness(brightness);
  }
  else {
    if (M5.BtnA.wasPressed()) {
      displayTimeoutMillis = millis();
      ledMode ++;
      if(ledMode > 5) ledMode = 0;
      sprite.setCursor(10, 100);
      sprite.println(ledMode);
    }
  }


  /* If wifi is connected print Wifi to the screen */
  if(WiFi.status() == WL_CONNECTED) {
    sprite.setTextSize(1);
    sprite.setCursor(10, 100);
    sprite.println("Wifi!");    
  }

  /* update the variable with time once a second */
  if(millis() - lastMillis > 1000)  {
    lastMillis = millis();

    getLocalTime(&timeinfo);
    if(timeinfo.tm_year == 0) {
      syncTime();
    } else {
      //Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
    }
  }


  if(millis() - displayTimeoutMillis > 5000) {
    File f;
    //displayTimeoutMillis = millis();
    if(ledMode == LED_WHITE) {
      f = LittleFS.open("/fish-day.png", "r");
    }
    else {
      f = LittleFS.open("/fish-night.png", "r");
    }

    if (!f) {
      Serial.println("Failed to open file for reading");
    }
    else 
    {
      sprite.drawPng(&f, 0, 0);
      f.close();
    }
  }
  

  // Push sprite to screen
  sprite.pushSprite(0, 0);
}

