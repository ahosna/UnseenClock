# UnseenClock
Audio clock for visualy impaired/blind.

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
