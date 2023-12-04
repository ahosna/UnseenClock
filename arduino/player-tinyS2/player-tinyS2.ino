#include <SPI.h>
#include <SD.h>
#include <TinyS2.h>
#include <WiFi.h>
#include <time.h>

#include "AudioGeneratorMP3.h"
#include "AudioFileSourceSD.h"
#include "AudioOutputI2S.h"

/* TODO - open questions
* Consider using RTC, but how do we set it ?
* WiFi NTP seems nice, might be too slow ?
* Store and set WIFI credentials ?
* Support multiple orderings based on locale time, date, dow - control from file on SD
*/


UMTINYS2 ts2;
AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;
struct tm tmstruct;
long timezone = 1;
byte daysavetime = 1;

#define SD_CS_PIN 14
#define COLOR_RED 0xff0000
#define COLOR_GREEN 0x00ff00
#define I2S_BCLK_PIN 4
#define I2S_WCLK_LRC_PIN 5
#define I2S_DATA_PIN 6
#define WIFI_SSID "SSID"
#define WIFI_PASS "Pass"

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

void prepPlayback(const char *file_name) {
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

bool GetLocalTime(struct tm *info, uint32_t ms) {
  uint32_t count = ms / 10;
  time_t now;

  time(&now);
  localtime_r(&now, info);

  if (info->tm_year > (2016 - 1900)) {
    return true;
  }

  while (count--) {
    delay(10);
    time(&now);
    localtime_r(&now, info);
    if (info->tm_year > (2016 - 1900)) {
      return true;
    }
  }
  return false;
}

void fetchTimeUsingWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.print("Connected\nContacting Time Server ");
  configTime(3600 * timezone, daysavetime * 3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
  do {
    tmstruct.tm_year = 0;
    GetLocalTime(&tmstruct, 5000);
    delay(100);
    Serial.print(".");
  } while (tmstruct.tm_year < 100);
  Serial.println("Done");
}

void fetchTimeAndSpeak() {
  char buff[20];

  GetLocalTime(&tmstruct, 5000);

  snprintf(buff, sizeof(buff) - 1, "/time/%02d/%02d.mp3", tmstruct.tm_hour, tmstruct.tm_min);
  prepPlayback(buff);

  snprintf(buff, sizeof(buff) - 1, "/dow/%1.mp3", (tmstruct.tm_wday + 1) % 7);
  prepPlayback(buff);

  snprintf(buff, sizeof(buff) - 1, "/date/%02d/%02d.mp3", tmstruct.tm_mon + 1, tmstruct.tm_mday);
  prepPlayback(buff);
}

void setup() {
  ts2.begin();
  Serial.begin(115200);
  initFS();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  prepPlayback("/gong.mp3");
  fetchTimeUsingWiFi();
}

void loop() {
  fetchTimeAndSpeak();
}