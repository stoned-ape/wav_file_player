# WAV File Player

This program loads a wav file, copies it into a DirectSound buffer, and plays it.

## Usage

```bat
wav_player.exe file.wav
&:: generate 216 Hz test waveforms
wav_player.exe -sine
wav_player.exe -square
wav_player.exe -saw
wav_player.exe -tri
```

## Example

with included wav files

```bat
wav_player alien_abduction.wav
wav_player jazz.wav
wav_player dubstep.wav
wav_player rockandroll.wav
```

## Build Instructions

From visual studio developer command prompt run:
```bat
cl main.cpp /link /out:wav_player.exe
```
or run `nmake`