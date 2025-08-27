// Possible Speed Improvements:
// * Hardware I²C at 400kHz (disable ACK check)
// * faster software serial implementation with ignorable ACK check


#include <Arduino.h>
#include <Wire.h>
#include <XantoI2C.h>

//#include <time.h>
//#include <sys/time.h>

#include "VM_IIC.h"
#include <Fonts/TomThumb.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include "fonts/Orbitron_Medium_16.h"

#include <FastLED.h>

#include "config.h"
#include <WiFiMulti.h>
#include "ota.h"


#define DISPLAY_WIDTH   84
#define DISPLAY_HEIGHT  16
#define FLIP_TIME       550  // [ms]

#define LED_PIN         4  // Change this to your desired GPIO pin
#define NUM_LEDS        20  // Change this to your LED strip length
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB
#define BRIGHTNESS      255  // Maximum brightness (0-255)
#define FRAMES_PER_SECOND 12

// doesn't work, because it waits for ACK that never comes
void i2cWriteByteHardware(uint8_t addr, uint8_t data) {
    Wire.beginTransmission(addr);
    Wire.write(data);
    Wire.endTransmission();
}

XantoI2C i2c(21, 20, 0);

void i2cWriteByteSoftware(uint8_t addr, uint8_t data) {
    i2c.start();
    i2c.writeByte(addr);
    i2c.readAck(); // discard ack
    i2c.writeByte(data);
    i2c.readNack(); // discard nack
    i2c.stop();
}


VM_IIC flipdot(DISPLAY_WIDTH, DISPLAY_HEIGHT, FLIP_TIME, i2cWriteByteSoftware);

CRGB leds[NUM_LEDS];

void initLEDs() {
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
        .setRgbw(Rgbw(kRGBWDefaultColorTemp, kRGBWExactColors, W0));;
    FastLED.setBrightness(BRIGHTNESS);
    FastLED.clear();
    for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB(0, 0, 255);
    }
    FastLED.show();
}

WiFiMulti wifiMulti;

void setup() {
    Serial.begin(115200);

    initLEDs();

    flipdot.setModuleMapping(3, 2, 1);

    // blank display
    flipdot.clearDisplay();
    flipdot.writeDot(0, 0, 1); // indicate done

    flipdot.setFont(&TomThumb);
    //flipdot.setFont(&Orbitron_Medium_16);

    flipdot.print("Connecting to WiFi...");
    flipdot.update();

    wifiMulti.addAP(WIFI_SSID, WIFI_PASS);
    if(wifiMulti.run() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());

        // Configure time with Berlin timezone// Configure time with Berlin timezone (GMT+1 with daylight saving)
/*         configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");  // First 3600 is GMT+1, second 3600 is for DST
        setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
        tzset(); */
    }
    initOTA();

    flipdot.fillScreen(0);
    flipdot.fillScreen(1);
}

// Time scaling factors for each component
#define TIME_FACTOR_HUE 60
#define TIME_FACTOR_SAT 100
#define TIME_FACTOR_VAL 100

void ledEffect() {
    uint32_t ms = millis();
    for(int i = 0; i < NUM_LEDS; i++) {
        // Use different noise functions for each LED and each color component
        uint8_t hue = inoise16(ms * TIME_FACTOR_HUE, i * 1000, 0) >> 8;
        uint8_t sat = inoise16(ms * TIME_FACTOR_SAT, i * 2000, 1000) >> 8;
        uint8_t val = inoise16(ms * TIME_FACTOR_VAL, i * 3000, 2000) >> 8;

        // Map the noise to full range for saturation and value
        sat = map(sat, 0, 255, 30, 255);
        val = map(val, 0, 255, 100, 255);

        leds[i] = CHSV(hue, sat, val);
    }

    FastLED.show();
}

unsigned long previousMillis = 0;
const long interval = 5000;
bool showKleider = true;

void loop() {
/*      if (flipdot.scrollTextRunning()) {
        flipdot.scrollTextTick();
    } else {
        flipdot.startScrollText(28, 14, "Universal University - Understanding Sex Work Through an Intersectional Lens");
    }
    delay(100); */

    //ledEffect();

    //flipdot.setFont(&FreeMonoBold12pt7b);

  unsigned long currentMillis = millis();


    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        showKleider = !showKleider; // Toggle the text to display
    }

    flipdot.setFont(&Orbitron_Medium_16);
    flipdot.setTextColor(1);
    flipdot.fillScreen(0);
    if (showKleider) {
        flipdot.drawCenteredText(0, 14, "Kleider");
    } else {
        flipdot.drawCenteredText(0, 14, "tausch");
    }
    flipdot.update();

    delay(500);
    // delay(500);
    // flipdot.clearDisplay();
    // delay(500);
    // flipdot.clearDisplay(true);

/*     struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
        flipdot.fillScreen(0);
        flipdot.drawCenteredText(0, 14, "NO TIME");
        flipdot.update();
        delay(1000);
        return;
    }
    
    char timeStr[9];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", 
             timeinfo.tm_hour, 
             timeinfo.tm_min, 
             timeinfo.tm_sec);
    
    flipdot.fillScreen(0);
    flipdot.drawCenteredText(0, 14, timeStr);
    flipdot.update();
    delay(1000);  // Update every second */


    loopOTA();
    wifiMulti.run();

}

