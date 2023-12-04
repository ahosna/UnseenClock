#include <SPI.h>
#include <SD.h>
#include <TinyS2.h>
#include "AudioGeneratorMP3.h"
#include "AudioFileSourceSD.h"
#include "AudioOutputI2S.h"


UMTINYS2 ts2;
AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;

#define SD_CS_PIN 14
#define COLOR_RED 0xff0000
#define COLOR_GREEN 0x00ff00
#define I2S_BCLK_PIN 4
#define I2S_WCLK_LRC_PIN 5
#define I2S_DATA_PIN 6

void blink(uint32_t color, uint32_t duration_ms) {
  // TODO add duty cycle
  ts2.setPixelPower(true);
  ts2.setPixelColor(color);
  while (1) {
    ts2.setPixelBrightness(128);
    delay(duration_ms / 2);
    ts2.setPixelBrightness(0);
    delay(duration_ms / 2);
  }
}

void errorSD() {
  Serial.println("SD card can't be initialized.");
  blink(COLOR_RED, 1000);
}

void wrongDataOnSD() {
  Serial.println("Wrong data on the SD card.");
  blink(COLOR_RED, 1000);
}

void initFS() {
  Serial.println("Initializing SD card");
  if (!SD.begin(SD_CS_PIN)) {
    errorSD();
  }
  if (!SD.exists("/dow/1.mp3")) {
    wrongDataOnSD();
  }
}

void prepPlayback(const char * file_name) {
  Serial.println("Playing");
  file = new AudioFileSourceSD(file_name);
  out = new AudioOutputI2S();
  out->SetOutputModeMono(true);
  out->SetPinout(I2S_BCLK_PIN, I2S_WCLK_LRC_PIN, I2S_DATA_PIN);
  mp3 = new AudioGeneratorMP3();
  mp3->begin(file, out);
  delay(10);
  while (mp3->isRunning()) {
    if (!mp3->loop()) mp3->stop();
  }
  Serial.println("done");
  file->close();
  out->stop();
}

void setup() {
  ts2.begin();
  Serial.begin(115200);
  initFS();
  delay(1000);
  prepPlayback("/time/01/01.mp3");
  prepPlayback("/dow/7.mp3");
  prepPlayback("/date/12/04.mp3");
}

void loop() {
  Serial.println("loop");
  blink(COLOR_GREEN, 1000);
}