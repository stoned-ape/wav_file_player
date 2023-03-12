all: wav_player.exe

wav_player.exe: main.cpp makefile
	cl main.cpp /link /out:wav_player.exe

run: wav_player.exe
	wav_player.exe alien_abduction.wav

