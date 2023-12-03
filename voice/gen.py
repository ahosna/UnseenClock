from typing import Tuple, Optional
import json
import logging
import sys
from google.cloud import texttospeech
import os


class GeneratorAbstract:
    def _days(self, m_id: int):
        if m_id == 1:
            return 29
        elif m_id in (0, 2, 4, 6, 7, 9, 11):
            return 31
        else:
            return 30

    def banks(self):
        return {
            "date": {
                self.date(m_id, d_id)
                for m_id in range(12)
                for d_id in range(self._days(m_id))
            },
            "dow": {self.day_of_week(dow_id) for dow_id in range(7)},
            "time": {
                self.time(hr_id, min_id) for hr_id in range(24) for min_id in range(60)
            },
        }

    def audio_config(self) -> texttospeech.AudioConfig:
        return texttospeech.AudioConfig(
            audio_encoding=texttospeech.AudioEncoding.MP3,
            speaking_rate=0.75,
            pitch=-1.0,
        )

    def voice_selection_params(self) -> texttospeech.VoiceSelectionParams:
        raise NotImplementedError()

    def create_audio_file(self, path: str, text: str, dry_run: bool = True):
        file_name = ".".join([os.path.join("locales", self.LANG, path), "mp3"])
        speak = f"<speak>{text}</speak>"
        logging.info(f"Calling google API for {file_name}: {speak}")
        if dry_run:
            return
        try:
            client = texttospeech.TextToSpeechClient()
            synthesis_input = texttospeech.SynthesisInput(ssml=speak)
            response = client.synthesize_speech(
                input=synthesis_input,
                voice=self.voice_selection_params(),
                audio_config=self.audio_config(),
            )
            os.makedirs(os.path.dirname(file_name), exist_ok=True)
            with open(file_name, "wb") as out:
                out.write(response.audio_content)

        except Exception as e:
            logging.exception(e)
            raise


class GeneratorEN(GeneratorAbstract):
    _DAYS_OF_WEEK = [
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday",
        "Sunday",
    ]
    _MONTHS = [
        "January",
        "February",
        "March",
        "April",
        "May",
        "June",
        "July",
        "August",
        "September",
        "October",
        "November",
        "December",
    ]

    def date(self, m_id, d_id) -> Tuple[str, str]:
        """The first part is path to the file. The Second is how the text looks."""
        return (
            os.path.join(f"{m_id+1:02}", f"{d_id+1:02}"),
            f"<p>{self._MONTHS[m_id]} {d_id+1}.</p>",
        )

    def day_of_week(self, dow_id) -> Tuple[str, str]:
        return (f"{dow_id+1:01}", f"<p>Today is {self._DAYS_OF_WEEK[dow_id]}.</p>")

    def time(self, hr_id, min_id) -> Tuple[str, str]:
        return (
            os.path.join(f"{hr_id:02}", f"{min_id:02}"),
            f"<p>It is {hr_id:02}:{min_id:02}.</p>",
        )

    def sequence(self) -> Tuple[str]:
        return ("time", "dow", "date")


class GeneratorSK(GeneratorAbstract):
    LANG = "sk"
    _DAYS_OF_WEEK = [
        "Pondelok",
        "Utorok",
        "Streda",
        "Štvrtok",
        "Piatok",
        "Sobota",
        "Nedeľa",
    ]
    _MONTHS = [
        "Január",
        "Február",
        "Marec",
        "Apríl",
        "Máj",
        "Jún",
        "Júl",
        "August",
        "September",
        "Október",
        "November",
        "December",
    ]
    _DAYS = [
        "Prvý",
        "Druhý",
        "Tretí",
        "Štvrtý",
        "Piaty",
        "Šiesty",
        "Siedmy",
        "Ôsmy",
        "Deviaty",
        "Desiaty",
        "Jedenásty",
        "Dvanásty",
        "Trinásty",
        "Štrnásty",
        "Pätnásty",
        "Šestnásty",
        "Sedemnásty",
        "Osemnásty",
        "Devätnásty",
        "Dvadsiaty",
        "Dvadsiatyprvý",
        "Dvadsiatydruhý",
        "Dvadsiatytretí",
        "Dvadsiatyštvrtý",
        "Dvadsiatypiaty",
        "Dvadsiatyšiesty",
        "Dvadsiatysiedmy",
        "Dvadsiatyôsmy",
        "DvadsiatyDeviaty",
        "Tridsiaty",
        "TridsiatyPrvý",
    ]

    def load_meniny():
        try:
            with open("meniny.json") as f:
                meniny = json.load(f)
                return meniny
        except OSError as e:
            logging.exception(e)
            raise

    MENINY = load_meniny()

    def meniny(self, m_id, d_id) -> Optional[str]:
        try:
            return self.MENINY[str(m_id + 1)][str(d_id + 1)]
        except KeyError:
            return None

    def date(self, m_id, d_id) -> Tuple[str, str]:
        """The first part is path to the file. The Second is how the text looks."""
        text = f"<p>Dnes je {self._DAYS[d_id]} {self._MONTHS[m_id]}.</p>"
        meniny = self.meniny(m_id, d_id)
        if meniny:
            text += f"<p>Meniny má {meniny}.</p>"
        return (os.path.join(f"{m_id+1:02}", f"{d_id+1:02}"), text)

    def day_of_week(self, dow_id) -> Tuple[str, str]:
        return (f"{dow_id+1:01}", f"<p>{self._DAYS_OF_WEEK[dow_id]}.</p>")

    def time(self, hr_id, min_id) -> Tuple[str, str]:
        if hr_id < 4 or hr_id > 20:
            suffix = " v noci"
        elif hr_id < 12:
            suffix = " ráno"
        elif hr_id > 17:
            suffix = " večer"
        elif hr_id > 12:
            suffix = " poobede"
        else:
            suffix = ""
        return (
            os.path.join(f"{hr_id:02}", f"{min_id:02}"),
            f"<p>Práve je {hr_id:02}:{min_id:02}{suffix}.</p>",
        )

    def sequence(self) -> Tuple[str]:
        return ("time", "dow", "date")

    def voice_selection_params(self) -> texttospeech.VoiceSelectionParams:
        return texttospeech.VoiceSelectionParams(
            language_code="sk-SK", name="sk-SK-Wavenet-A"
        )


if __name__ == "__main__":
    dry_run = False
    logging.basicConfig(level=logging.DEBUG)
    gen = GeneratorSK()
    for bank, texts in gen.banks().items():
        for path, text in texts:
            path = os.path.join(bank, path)
            gen.create_audio_file(path, text, dry_run)
