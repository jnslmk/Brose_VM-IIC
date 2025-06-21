// Possible Speed Improvements:
// * Hardware I²C at 400kHz (disable ACK check)
// * faster software serial implementation with ignorable ACK check


#include <Arduino.h>
#include <Wire.h>
#include <XantoI2C.h>

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
#define FRAMES_PER_SECOND 120

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

    flipdot.setFont(&Orbitron_Medium_16);
    //flipdot.startScrollText(0, 16, "  Casino  ");

    flipdot.print("Connecting to WiFi...");
    flipdot.update();

    wifiMulti.addAP(WIFI_SSID, WIFI_PASS);
    if(wifiMulti.run() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
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

void loop() {
    if (flipdot.scrollTextRunning()) {
        flipdot.scrollTextTick();
    } else {
        flipdot.startScrollText(84, 14, "///Casino...");
    }
    delay(10);

    //ledEffect();

    // flipdot.setFont(&FreeMonoBold12pt7b);
    // flipdot.setTextColor(1);
    // flipdot.fillScreen(0);
    // flipdot.drawCenteredText(0, 14, "Casino");
    // flipdot.update();
    // delay(1000);
    // flipdot.fillScreen(0);
    // flipdot.drawCenteredText(0, 14, "Space");
    // flipdot.update();

    // delay(500);
    // flipdot.clearDisplay();
    // delay(500);
    // flipdot.clearDisplay(true);

    loopOTA();
    wifiMulti.run();

}

