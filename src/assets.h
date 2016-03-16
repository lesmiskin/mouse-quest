#ifndef ASSETS_H
#define ASSETS_H

#include "SDL2/SDL_image.h"
#include "SDL2/SDL_mixer.h"

#define ASSET_VERSIONS 5
typedef enum {
	ASSET_DEFAULT = 0,
	ASSET_HIT = 1,
	ASSET_SHADOW = 2,
	ASSET_SUPER = 3,
	ASSET_ALPHA = 4
} AssetVersion;

typedef struct {
	char* key;
	SDL_Texture* texture;
	SDL_Texture* textures[ASSET_VERSIONS];
} Asset;

typedef struct {
	char* key;
	Mix_Chunk* sound;
} SoundAsset;

typedef struct {
	char* key;
	Mix_Music*music;
} MusicAsset;

extern void initAssets(void);
extern SDL_Texture *getTexture(char *path);
extern SDL_Texture *getTextureVersion(char *path, AssetVersion version);
extern Asset getAsset(char *path);
extern void shutdownAssets(void);
extern SoundAsset getSound(char *path);
extern MusicAsset getMusic(char *path);

#endif
