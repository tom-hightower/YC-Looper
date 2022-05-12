#include <Encoder.h>
#include <Audio.h>
#include <Wire.h>               // SCL pin 19, SDA pin 18
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "YC_Looper.h"

void setup() {
  Serial.begin(9600);
  Serial.println("YC Looper Test");
  init_display();
  init_leds();
  init_pbs();
  init_audio_shield();
  init_menus();
  init_timers();
}

// MAIN LOOP
void loop() {
  if (any_channel_recording()) try_record();

  check_metronome();
  check_encoder();
  check_pbs();

  if (screenNeedsUpdate) {
    update_display();
  }

  if (ledNeedsUpdate) {
    update_leds();
  }
}
