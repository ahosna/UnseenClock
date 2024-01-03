#include <SPI.h>
#include <SD.h>
#include <UMS3.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <Ethernet.h>
#include <DS3231-RTC.h>

#include "AudioGeneratorMP3.h"
#include "AudioFileSourceSD.h"
#include "AudioOutputI2S.h"
#include "esp_bt_main.h"
#include "esp_bt.h"
#include "esp_wifi.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

/* TODO - open questions
* Consider using RTC, but how do we set it ?
* WiFi NTP seems nice, might be too slow ?
* Store and set WIFI credentials ?
* Support multiple orderings based on locale time, date, dow - control from file on SD
*/


UMS3 ts3;
AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;
DS3231 myRTC;
struct tm tmstruct;
long timezone = 1;
byte daysavetime = 1;

#define SD_CS_PIN 34
#define COLOR_RED 0xff0000
#define COLOR_GREEN 0x00ff00
#define I2S_BCLK_PIN 2
#define I2S_WCLK_LRC_PIN 3
#define I2S_DATA_PIN 1
#define I2S_ENABLE_PIN gpio_num_t::GPIO_NUM_4
#define WAKE_PIN gpio_num_t::GPIO_NUM_21
#define WAKE_PIN_MASK GPIO_SEL_21
#define WIFI_SSID "SSID"
#define WIFI_PASS "PASS"
#define IP_HOST IPAddress(192, 168, 1, 31)
#define IP_GW IPAddress(192, 168, 1, 1)
#define IP_MASK IPAddress(255, 255, 255, 0)
#define IP_DNS1 IPAddress(8, 8, 8, 8)
#define IP_DNS2 IPAddress(4, 4, 4, 4)
#define TZ "CET1CEST2,M3.5.0/-2,M10.5.0/-1"

void blink(uint32_t color, uint32_t duration_ms) {
  // TODO add duty cycle
  ts3.setPixelPower(true);
  ts3.setPixelColor(color);
  while (1) {
    ts3.setPixelBrightness(128);
    delay(duration_ms / 2);
    ts3.setPixelBrightness(0);
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


/*************************** WIFI functions **********************************************/
void fetchTimeUsingWiFi() {
  Serial.println("Connecting to WIFI (500ms for each dot)");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected\nContacting Time Server ");
  configTime(3600 * timezone, daysavetime * 3600, "0.pool.ntp.org", "1.pool.ntp.org");
  do {
    tmstruct.tm_year = 0;
    GetLocalTime(&tmstruct, 5000);
    delay(500);
    Serial.print(".");
  } while (tmstruct.tm_year < 100);
  Serial.print("Done. Retrieved time is: ");
  dumpTime();
}

/*************************** RTC functions ************************************************/
void fetchTimeFromRtc() {
  bool century = false;
  bool h12Flag;
  bool pmFlag;
  Serial.println("Fetching the time from RTC clock");
  tmstruct.tm_year = myRTC.getYear() + 100;
  tmstruct.tm_mon = myRTC.getMonth(century);
  tmstruct.tm_mday = myRTC.getDate();
  tmstruct.tm_hour = myRTC.getHour(h12Flag, pmFlag);
  tmstruct.tm_min = myRTC.getMinute();
  tmstruct.tm_sec = myRTC.getSecond();
  tmstruct.tm_wday = myRTC.getDoW();
  dumpTime();
  Serial.print("Reported temperature: ");
  Serial.println(myRTC.getTemperature());
}

/*************************** Time functions **********************************************/
void dumpTime() {
  Serial.print("Time: ");
  Serial.print(tmstruct.tm_year + 1900);
  Serial.print("-");
  Serial.print(tmstruct.tm_mon);
  Serial.print("-");
  Serial.print(tmstruct.tm_mday);
  Serial.print("(");
  Serial.print(tmstruct.tm_wday);
  Serial.print(") ");
  Serial.print(tmstruct.tm_hour);
  Serial.print(":");
  Serial.print(tmstruct.tm_min);
  Serial.print(":");
  Serial.print(tmstruct.tm_sec);
  Serial.println("");
}

void syncRtcFromWifi() {
  Serial.print("Time before RTC fetch: ");
  dumpTime();
  Serial.print("RTC time: ");
  fetchTimeFromRtc();
  fetchTimeUsingWiFi();
  Serial.println("Writing time to RTC");
  myRTC.setClockMode(false);
  myRTC.setYear(tmstruct.tm_year - 100);
  myRTC.setMonth(tmstruct.tm_mon);
  myRTC.setDate(tmstruct.tm_mday);
  myRTC.setHour(tmstruct.tm_hour);
  myRTC.setMinute(tmstruct.tm_min);
  myRTC.setSecond(tmstruct.tm_sec);
  myRTC.setDoW(tmstruct.tm_wday);
  Serial.print("Final time");
  fetchTimeFromRtc();
}

void speakTime() {
  char buff[20];

  snprintf(buff, sizeof(buff) - 1, "/time/%02d/%02d.mp3", tmstruct.tm_hour, tmstruct.tm_min);
  prepPlayback(buff);

  snprintf(buff, sizeof(buff) - 1, "/dow/%01d.mp3", (tmstruct.tm_wday) % 7);
  prepPlayback(buff);

  snprintf(buff, sizeof(buff) - 1, "/date/%02d/%02d.mp3", tmstruct.tm_mon + 1, tmstruct.tm_mday);
  prepPlayback(buff);
}

void initWiFi() {
  delay(500);
  WiFi.persistent(true);
  WiFi.enableSTA(true);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.config(IP_HOST, IP_GW, IP_MASK, IP_DNS1, IP_DNS2);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void setTimezone() {
  // TODO pull from the SD card
  setenv("TZ", TZ, 1);
  tzset();
}

/*************************** SLEEP functions **********************************************/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void enterComa() {
  esp_bluedroid_disable();
  esp_bt_controller_disable();
  esp_wifi_stop();
  // audio module down
  rtc_gpio_pulldown_dis(I2S_ENABLE_PIN);
  rtc_gpio_pullup_en(I2S_ENABLE_PIN);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  rtc_gpio_pulldown_dis(WAKE_PIN);
  rtc_gpio_pullup_en(WAKE_PIN);
  esp_sleep_enable_ext1_wakeup(WAKE_PIN_MASK, ESP_EXT1_WAKEUP_ALL_LOW);
  Serial.println("Going to sleep now");
  delay(1000);
  esp_deep_sleep_start();
}

void setup() {
  ts3.begin();
  Serial.begin(115200);
  print_wakeup_reason();

  // check if button is held
  bool do_sync = true;
  pinMode(WAKE_PIN, INPUT_PULLUP);
  delay(100);
  for (int i=0; i<10; i++) {
    int v = digitalRead(WAKE_PIN);
    if (v==1) do_sync=false;
    delay(100);
  }

  initFS();
  setTimezone();
  Wire.begin();
  prepPlayback("/gong.mp3");
  if (do_sync) {
    initWiFi();
    syncRtcFromWifi();
  }
}

void loop() {
  fetchTimeFromRtc();
  speakTime();
  // speakTime();
  if (ts3.getVbusPresent()) {
    blink(COLOR_GREEN, 1000);
  }else{
    enterComa();
  }
}
