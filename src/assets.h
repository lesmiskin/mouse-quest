#ifndef ASSETS_H
#define ASSETS_H

#include "SDL2/SDL_image.h"
#include "SDL2/SDL_mixer.h"

typedef struct {
	char* key;
	SDL_Texture* texture;
	SDL_Texture* textures[4];
} Asset;

typedef struct {
	char* key;
	Mix_Chunk* sound;
	int volume;
} SoundAsset;

//Adjust the size of Asset.textures[] if items added or removed.
typedef enum {
	ASSET_DEFAULT = 0,
	ASSET_HIT = 1,
	ASSET_SHADOW = 2,
	ASSET_SUPER = 3
} AssetVersion;

extern void initAssets(void);
extern SDL_Texture *getTexture(char *path);
extern SDL_Texture *getTextureVersion(char *path, AssetVersion version);
extern Asset getAsset(char *path);
extern void shutdownAssets(void);
extern SoundAsset getSound(char *path);
extern void play(char* path);

#endif
