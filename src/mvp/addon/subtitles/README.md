# Police radio subtitles

`subtitles.en.json` is copied from the MTA HP2 `pursuit_voice_bank`
resource. The catalog uses native clip filenames based on byte offsets in
the US PS2 `ENGLISHDAT.PCK`. The add-on verifies the speech file hash,
then uses the offsets selected by the original radio system in playback
order. The WAV keys identify clips in the transcription catalog. The
game reads the actual audio from the player's disc.

The graphics host owns the subtitle overlay and stores the shared
preference in `launcher.cfg` under `[accessibility]`.
