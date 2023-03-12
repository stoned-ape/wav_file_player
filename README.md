# WAV File Player

This program loads a wav file, copies it into a DirectSound buffer, and plays it.

## Usage

```bat
wav_player.exe file.wav
```

## Example

with included wav file

```bat
wav_player alien_abduction.wav
```

## Build Instructions

From visual studio developer command prompt run:
```bat
cl main.cpp /link /out:wav_player.exe
```
or run `nmake`