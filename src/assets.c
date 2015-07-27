#include <assert.h>
#include <SDL_mixer.h>
#include "assets.h"
#include "common.h"
#include "renderer.h"

typedef struct {
	char* filename;
	bool makeHitVersion;
	bool makeShadowVersion;
	bool makeSuperVersion;
} AssetDef;

typedef struct {
	char* filename;
	int volume;
} SoundDef;

static char* assetPath;
static Asset *assets;
static int assetCount;
static SoundAsset *sounds;
static int soundCount;
static MusicAsset *music;
static int musicCount;

MusicAsset getMusic(char *path) {
	//Loop through register until key is found, or we've exhausted the array's iteration.
	for(int i=0; i < musicCount; i++) {
		if(strcmp(music[i].key, path) == 0)			//if strings match.
			return music[i];
	}

	fatalError("Could not find Asset in register", path);
}

SoundAsset getSound(char *path) {
	//Loop through register until key is found, or we've exhausted the array's iteration.
	for(int i=0; i < soundCount; i++) {
		if(strcmp(sounds[i].key, path) == 0)			//if strings match.
			return sounds[i];
	}

	fatalError("Could not find Asset in register", path);
}

void playMusic(char* path, int loops) {
	Mix_PlayMusic(getMusic(path).music, loops);
}

void play(char* path) {
	Mix_PlayChannel(-1, getSound(path).sound, 0);
}

static Asset makeAsset(AssetDef definition) {
	assert(renderer != NULL);

	char *absPath = combineStrings(assetPath, definition.filename);
	//Check existence on file system.
	if(!fileExists(absPath))
		fatalError("Could not find Asset on disk", absPath);

	//Load file from disk.
	SDL_Surface *original = IMG_Load(absPath);
	free(absPath);

	Asset asset = {	definition.filename	};

	//Build initial texture and assign.
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, original);
	asset.textures[ASSET_DEFAULT] = texture;

	//If the definition calls for it, also add a 'hit' variant.
	if(definition.makeSuperVersion) {
		colouriseSprite(original, makeColour(0,0,8,255), COLOURISE_ADDITIVE);
		SDL_Texture *superTexture = SDL_CreateTextureFromSurface(renderer, original);
		asset.textures[ASSET_SUPER] = superTexture;
	}
	if(definition.makeHitVersion) {
		colouriseSprite(original, makeColour(128,0,0,255), COLOURISE_ADDITIVE);
		SDL_Texture *hitTexture = SDL_CreateTextureFromSurface(renderer, original);
//		SDL_SetTextureAlphaMod(original, 0);
//		SDL_SetTextureColorMod(hitTexture, 0, 0, 255);
		asset.textures[ASSET_HIT] = hitTexture;
	}
	if(definition.makeShadowVersion) {
		colouriseSprite(original, makeColour(0,0,0,210), COLOURISE_ABSOLUTE);
		SDL_Texture *shadowTexture = SDL_CreateTextureFromSurface(renderer, original);
//		SDL_SetTextureColorMod(shadowTexture, 0, 0, 0);
//		SDL_SetTextureAlphaMod(shadowTexture, 150);
		asset.textures[ASSET_SHADOW] = shadowTexture;
	}

	//Free up RAM used for original surface object.
	free(original);

	return asset;
}

SDL_Texture *getTexture(char *path) {
	return getTextureVersion(path, ASSET_DEFAULT);
}
SDL_Texture *getTextureVersion(char *path, AssetVersion version) {
	return getAsset(path).textures[version];
}
Asset getAsset(char *path) {
	//Loop through register until key is found, or we've exhausted the array's iteration.
	for(int i=0; i < assetCount; i++) {
		if(strcmp(assets[i].key, path) == 0)			//if strings match.
			return assets[i];
	}

	fatalError("Could not find Asset in register", path);
}

void shutdownAssets(void) {
	free(assetPath);
	free(assets);

	for(int i=0; i < soundCount; i++) Mix_FreeChunk(sounds[i].sound);
	for(int i=0; i < musicCount; i++) Mix_FreeMusic(music[i].music);

	free(sounds);
}

static void loadImages(void) {
	//Define assets to be loaded.
	AssetDef definitions[] = {
		{ "life.png", false, false, false },
		{ "life-half.png", false, false, false },
		{ "life-none.png", false, false, false },
		{ "lm-presents.png", false, false, false },
		{ "super-streak.png", false, false, false },
		{ "super-fleck.png", false, false, false },
		{ "level-1.png", false, false, false },
		{ "title.png", false, false, false },
		{ "base-chip.png", false, false, false },
		{ "base-resistor.png", false, false, false },
		{ "base-resistor-2.png", false, false, false },
		{ "base.png", false, false, false },
		{ "base-n.png", false, false, false },
		{ "base-n-resistor.png", false, false, false },
		{ "base-n-resistor-2.png", false, false, false },
		{ "base-s.png", false, false, false },
		{ "base-s-resistor.png", false, false, false },
		{ "base-s-resistor-2.png", false, false, false },
		{ "base-e.png", false, false, false },
		{ "base-e-resistor-2.png", false, false, false },
		{ "base-w.png", false, false, false },
		{ "base-w-resistor.png", false, false, false },
		{ "base-w-resistor-2.png", false, false, false },
		{ "base-ne.png", false, false, false },
		{ "base-nw.png", false, false, false },
		{ "base-se.png", false, false, false },
		{ "base-sw.png", false, false, false },
		{ "base-ne-inner.png", false, false, false },
		{ "base-nw-inner.png", false, false, false },
		{ "base-se-inner.png", false, false, false },
		{ "base-sw-inner.png", false, false, false },
		{ "virus-shot.png", false, true, false },
		{ "planet-01.png", false, false, false },
		{ "planet-02.png", false, false, false },
		{ "planet-03.png", false, false, false },
		{ "planet-04.png", false, false, false },
		{ "shot-blue-01.png", false, true},
		{ "shot-blue-02.png", false, true},
		{ "shot-neon-01.png", false, true},
		{ "shot-neon-02.png", false, true},
		{ "shot-aqua.png", false, true, false },
		{ "shot-orange.png", false, true, false },
		{ "space.png", false, false, false },
		{ "space-01.png", false, false, false },
		{ "space-02.png", false, false, false },
		{ "space-03.png", false, false, false },
		{ "space-04.png", false, false, false },
		{ "space-05.png", false, false, false },
		{ "space-06.png", false, false, false },
		{ "space-07.png", false, false, false },
		{ "space-08.png", false, false, false },
		{ "speech-entry.png", false, false, false },
		{ "speech-coin.png", false, false, false },
		{ "virus-01.png", true, true, false },
		{ "virus-02.png", true, true, false },
		{ "virus-03.png", true, true, false },
		{ "virus-04.png", true, true, false },
		{ "virus-05.png", true, true, false },
		{ "virus-06.png", true, true, false },
		{ "bug-01.png", true, true, false },
		{ "bug-02.png", true, true, false },
		{ "bug-03.png", true, true, false },
		{ "bug-04.png", true, true, false },
		{ "bug-05.png", true, true, false },
		{ "bug-06.png", true, true, false },
		{ "cd-01.png", true, true, false },
		{ "cd-02.png", true, true, false },
		{ "cd-03.png", true, true, false },
		{ "cd-04.png", true, true, false },
		{ "disk-01.png", true, true, false },
		{ "disk-02.png", true, true, false },
		{ "disk-03.png", true, true, false },
		{ "disk-04.png", true, true, false },
		{ "disk-05.png", true, true, false },
		{ "disk-06.png", true, true, false },
		{ "disk-07.png", true, true, false },
		{ "disk-08.png", true, true, false },
		{ "disk-09.png", true, true, false },
		{ "disk-10.png", true, true, false },
		{ "disk-11.png", true, true, false },
		{ "disk-12.png", true, true, false },
		{ "disk-blue-01.png", true, true, false },
		{ "disk-blue-02.png", true, true, false },
		{ "disk-blue-03.png", true, true, false },
		{ "disk-blue-04.png", true, true, false },
		{ "disk-blue-05.png", true, true, false },
		{ "disk-blue-06.png", true, true, false },
		{ "disk-blue-07.png", true, true, false },
		{ "disk-blue-08.png", true, true, false },
		{ "disk-blue-09.png", true, true, false },
		{ "disk-blue-10.png", true, true, false },
		{ "disk-blue-11.png", true, true, false },
		{ "disk-blue-12.png", true, true, false },
		{ "mike-01.png", true, true, true },
		{ "mike-02.png", true, true, true },
		{ "mike-03.png", true, true, true },
		{ "mike-04.png", true, true, true },
		{ "mike-05.png", true, true, true },
		{ "mike-06.png", true, true, true },
		{ "mike-07.png", true, true, true },
		{ "mike-08.png", true, true, true },
		{ "mike-lean-left-01.png", true, true, false },
		{ "mike-lean-left-02.png", true, true, false },
		{ "mike-lean-left-03.png", true, true, false },
		{ "mike-lean-left-04.png", true, true, false },
		{ "mike-lean-left-05.png", true, true, false },
		{ "mike-lean-left-06.png", true, true, false },
		{ "mike-lean-left-07.png", true, true, false },
		{ "mike-lean-left-08.png", true, true, false },
		{ "mike-lean-right-01.png", true, true, false },
		{ "mike-lean-right-02.png", true, true, false },
		{ "mike-lean-right-03.png", true, true, false },
		{ "mike-lean-right-04.png", true, true, false },
		{ "mike-lean-right-05.png", true, true, false },
		{ "mike-lean-right-06.png", true, true, false },
		{ "mike-lean-right-07.png", true, true, false },
		{ "mike-lean-right-08.png", true, true, false },
		{ "mike-shoot-evil-01.png", true, true, false },
		{ "mike-shoot-evil-02.png", true, true, false },
		{ "mike-shoot-01.png", true, true, false },
		{ "mike-shoot-02.png", true, true, false },
		{ "mike-shoot-left-01.png", true, true, false },
		{ "mike-shoot-left-02.png", true, true, false },
		{ "mike-shoot-right-01.png", true, true, false },
		{ "mike-shoot-right-02.png", true, true, false },
		{ "exp-01.png", false, true, false },
		{ "exp-02.png", false, true, false },
		{ "exp-03.png", false, true, false },
		{ "exp-04.png", false, true, false },
		{ "exp-05.png", false, true, false },
		{ "exp-06.png", false, true, false },
		{ "exp-07.png", false, true, false },
	};

	//Infer asset path from current directory.
	char* workingPath = SDL_GetBasePath();
	char assetsFolder[] = "assets/";
	assetPath = combineStrings(workingPath, assetsFolder);

	//Allocate memory to Asset register.
	assetCount = sizeof(definitions) / sizeof(AssetDef);
	assets = malloc(sizeof(Asset) * assetCount);

	//Build and load each Asset into the register.
	for(int i=0; i < assetCount; i++) {
		assets[i] = makeAsset(definitions[i]);
	}
}

static void loadSounds(void) {
	const int SOUND_VOLUME = 12 ;

	SoundDef defs[] = {
		{ "intro-presents.wav", SOUND_VOLUME * 2 },
		{ "Powerup8.wav", SOUND_VOLUME },
		{ "warp.wav", SOUND_VOLUME },
		{ "start.wav", SOUND_VOLUME },
		{ "Hit_Hurt10.wav", SOUND_VOLUME },
		{ "Hit_Hurt9.wav", SOUND_VOLUME },
		{ "Laser_Shoot34.wav", SOUND_VOLUME / 2 },
		{ "Laser_Shoot18.wav", SOUND_VOLUME / 1.5 },
		{ "Laser_Shoot5.wav", SOUND_VOLUME / 2 },
		{ "Explosion14.wav", SOUND_VOLUME },
		{ "Explosion3.wav", SOUND_VOLUME },
		{ "Explosion2.wav", SOUND_VOLUME },
		{ "Explosion.wav", SOUND_VOLUME }
	};

	soundCount = sizeof(defs) / sizeof(SoundDef);
	sounds = malloc(sizeof(SoundAsset) * soundCount);

	for(int i=0; i < soundCount; i++) {
		//Load music.
		char* path = combineStrings(assetPath, defs[i].filename);
		Mix_Chunk* chunk = Mix_LoadWAV(path);
		if(!chunk) fatalError("Could not find Asset on disk", path);

		//Reduce volume if called for.
		if(defs[i].volume < SDL_MIX_MAXVOLUME) Mix_VolumeChunk(chunk, defs[i].volume);

		//Add to register
		SoundAsset snd = {
			defs[i].filename,
			chunk
		};
		sounds[i] = snd;
	}
}

//TODO: Lots of duplication in music+sound loading.
//TODO: Stop 'tic' static on end of sound loop.

static void loadMusic(void) {
	char* defs[] = {
		"intro-battle.ogg",
		"intro-battle-3.ogg",
		"level-01.ogg",
		"level-01c.ogg",
		"title.ogg"
	};

	musicCount = sizeof(defs) / sizeof(char*);
	music = malloc(sizeof(MusicAsset) * musicCount);

	for(int i=0; i < musicCount; i++) {
		//Load music.
		char* path = combineStrings(assetPath, defs[i]);
		Mix_Music* chunk = Mix_LoadMUS(path);
		if(!chunk) fatalError("Could not find Asset on disk", path);

		//Add to register
		MusicAsset snd = {
			defs[i],
			chunk
		};
		music[i] = snd;
	}
}

void initAssets(void) {
	loadImages();
	loadSounds();
	loadMusic();
}
