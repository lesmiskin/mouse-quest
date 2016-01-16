#include "sound.h"
#include "assets.h"
#include "stdbool.h"
#include "SDL2/SDL_mixer.h"

static bool musicPlaying = true;

void toggleMusic() {
	if(musicPlaying) {
		Mix_PauseMusic();
	}else{
		Mix_ResumeMusic();
	}
	musicPlaying = !musicPlaying;
}

void playMusic(char* path, int loops) {
	Mix_PlayMusic(getMusic(path).music, loops);
}

void play(char* path) {
	Mix_PlayChannel(-1, getSound(path).sound, 0);
}
