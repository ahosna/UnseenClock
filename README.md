# UnseenClock
Audio clock for visualy impaired/blind.

![Prototype](doc/unseen-clock-case.jpg)

## Motivation
Story: AS a blind person I want to have an easy to find clock, that allows me to get the current time, date and day of the week. This is so that I can orient myself better throughout the day.

## About
Simple to use (one button) clock for blind people. Single button allows the clock to wake up and tell the time and date. 

## Technical description
Audio packs are generated before with google api and stored on the SD card. The clock is built with Unexpected Maker's TinyS3, I2S AMP and RTC clock. MicroSD/SD is attached to SPI to get the voice files.


# Install/Dev
## Python
```
python3 -m venv env
source env/bin/activate
pip3 install google-cloud-texttospeech
```

## Google api credentials
```
cp <credentials.json> env/unseenclock-credentials.json
export GOOGLE_APPLICATION_CREDENTIALS=env/unseenclock-credentials.json
```

## Running
`python3 gen.py`
For now massage the gen.py to set the proper language/voice. Don't forget to export `GOOGLE_APPLICATION_CREDENTIALS`.
The app regenerates all the voices when run. Currently it's about `365 + 1440 + 7` entries of say less then 5 words on average. In total ~9k words.

## Packs
The latest packs are in the repo voice/locales/<LANG>
Encoding from google is `MPEG ADTS, layer III, v2,  32 kbps, 24 kHz, Monaural`

# Build
![Top](doc/unseen-clock-top.jpg)
![Bottom](doc/unseen-clock-bottom.jpg)
![Dismantled](doc/unseen-clock-dismantled.jpg)
