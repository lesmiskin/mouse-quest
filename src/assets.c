#include "myc.h"
#include <assert.h>
#include "assets.h"
#include "common.h"
#include "renderer.h"

typedef struct {
	char* filename;
	bool makeHitVersion;
	bool makeShadowVersion;
	bool makeSuperVersion;
	bool makeAlphaVersion;
	bool isBackground;
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
static const int MUSIC_VOLUME = 100;

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

static Asset makeAsset(AssetDef definition) {
	assert(renderer != NULL);

	char *absPath = combineStrings(assetPath, definition.filename);
	//Check existence on file system.
	if(!fileExists(absPath))
		fatalError("Could not find Asset on disk", absPath);

	//Load file from disk.
	SDL_Surface *original = IMG_Load(absPath);
//	SDL_Surface *original = SDL_ConvertSurface(unoptimised, SDL_PIXELFORMAT_ABGR8888, 0);
	free(absPath);

	Asset asset = {	definition.filename	};

	//Build initial texture and assign.
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, original);

	//Darken background elements to provide contrast with foreground.
	//A hint of blue is added for atmosphere.
	if(definition.isBackground) {
		SDL_SetTextureColorMod(texture, 164, 164, 164);
	}

	asset.textures[ASSET_DEFAULT] = texture;

	//IMPORTANT: Some of our colourisation calls are destructive to the original surface asset,
	// so the order they're done in is significant. We should try to change these to SDL
	// modulation calls instead, which operate on textures, not the surface.

	//If the definition calls for it, also add a 'hit' variant.
	if(definition.makeAlphaVersion) {
		SDL_Texture *alphaTexture = SDL_CreateTextureFromSurface(renderer, original);
		SDL_SetTextureBlendMode(alphaTexture, SDL_BLENDMODE_BLEND);
		SDL_SetTextureAlphaMod(alphaTexture, 96);
		asset.textures[ASSET_ALPHA] = alphaTexture;
	}
	if(definition.makeShadowVersion) {
		SDL_Texture *shadowTexture = SDL_CreateTextureFromSurface(renderer, original);
		SDL_SetTextureColorMod(shadowTexture, 0, 0, 0);
		if(ALPHA_SHADOWS) {
			SDL_SetTextureAlphaMod(shadowTexture, 225);
		}
		asset.textures[ASSET_SHADOW] = shadowTexture;
	}
	if(definition.makeSuperVersion) {
		colouriseSprite(original, makeColour(0,0,8,255), COLOURISE_ADDITIVE);
		SDL_Texture *superTexture = SDL_CreateTextureFromSurface(renderer, original);
		asset.textures[ASSET_SUPER] = superTexture;
	}
	if(definition.makeHitVersion) {
		colouriseSprite(original, makeColour(128,0,0,255), COLOURISE_ADDITIVE);
		SDL_Texture *hitTexture = SDL_CreateTextureFromSurface(renderer, original);
		asset.textures[ASSET_HIT] = hitTexture;
	}

	//Free up RAM used for original surface object.
#if !defined(_WIN32)
	free(original);
#endif

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

void shutdownAssets() {
	free(assetPath);
	free(assets);

	for(int i=0; i < soundCount; i++) Mix_FreeChunk(sounds[i].sound);
	for(int i=0; i < musicCount; i++) Mix_FreeMusic(music[i].music);

	free(sounds);
}

static void loadImages() {
	//Define assets to be loaded.
	AssetDef definitions[] = {
		{ "text-keyface.png", false, false, false, false, false },
		{ "health-bar.png", false, false, false, false, false },
		{ "health-bar-bg.png", false, false, false, false, false },
		{ "text-laser-upgraded.png", false, true, false, false, false },
		{ "text-full-power.png", false, true, false, false, false },
		{ "top-score.png", false, false, false, false, false },
		{ "insert-coin-0.png", false, false, false, false, false },
		{ "insert-coin-1.png", false, false, false, false, false },
		{ "insert-coin-dim-0.png", false, false, false, false, false },
		{ "insert-coin-dim-1.png", false, false, false, false, false },
		{ "font-x.png", false, true, false, false, false },
		{ "font-00.png", false, true, false, false, false },
		{ "font-01.png", false, true, false, false, false },
		{ "font-02.png", false, true, false, false, false },
		{ "font-03.png", false, true, false, false, false },
		{ "font-04.png", false, true, false, false, false },
		{ "font-05.png", false, true, false, false, false },
		{ "font-06.png", false, true, false, false, false },
		{ "font-07.png", false, true, false, false, false },
		{ "font-08.png", false, true, false, false, false },
		{ "font-09.png", false, true, false, false, false },
		{ "warning.png", false, false, false, false, false },
		{ "mike-shades-01.png", false, true, false, false, false },
		{ "mike-shades-02.png", false, true, false, false, false },
		{ "mike-shades-03.png", false, true, false, false, false },
		{ "mike-shades-04.png", false, true, false, false, false },
		{ "mike-shades-05.png", false, true, false, false, false },
		{ "mike-shades-06.png", false, true, false, false, false },
		{ "mike-shades-07.png", false, true, false, false, false },
		{ "mike-shades-08.png", false, true, false, false, false },
		{ "mike-shades-09.png", false, true, false, false, false },
		{ "mike-shades-10.png", false, true, false, false, false },
		{ "mike-shades-11.png", false, true, false, false, false },
		{ "mike-shades-12.png", false, true, false, false, false },
		{ "mike-shades-13.png", false, true, false, false, false },
		{ "mike-fright-left.png", false, true, false, false, false },
		{ "mike-fright-right.png", false, true, false, false, false },
		{ "key-a.png", true, true, false, false, false },
		{ "keyboss-mini-01.png", false, false, false, false, false },
		{ "keyboss-mini-02.png", false, false, false, false, false },
		{ "keyboss-01.png", true, true, false, false, false },
		{ "keyboss-02.png", true, true, false, false, false },
		{ "keyboss-03.png", true, true, false, false, false },
		{ "keyboss-04.png", true, true, false, false, false },
		{ "keyboss-05.png", true, true, false, false, false },
		{ "keyboss-06.png", true, true, false, false, false },
		{ "keyboss-07.png", true, true, false, false, false },
		{ "keyboss-08.png", true, true, false, false, false },
		{ "keyboss-09.png", true, true, false, false, false },
		{ "keyboss-10.png", true, true, false, false, false },
		{ "keyboss-11.png", true, true, false, false, false },
		{ "keyboss-12.png", true, true, false, false, false },
		{ "magnet-01.png", true, true, false, false, false },
		{ "magnet-02.png", true, true, false, false, false },
		{ "magnet-03.png", true, true, false, false, false },
		{ "magnet-04.png", true, true, false, false, false },
		{ "magnet-05.png", true, true, false, false, false },
		{ "magnet-06.png", true, true, false, false, false },
		{ "magnet-07.png", true, true, false, false, false },
		{ "magnet-08.png", true, true, false, false, false },
		{ "coin-01.png", false, true, false, true, false },
		{ "coin-02.png", false, true, false, true, false },
		{ "coin-03.png", false, true, false, true, false },
		{ "coin-04.png", false, true, false, true, false },
		{ "coin-05.png", false, true, false, true, false },
		{ "coin-06.png", false, true, false, true, false },
		{ "coin-07.png", false, true, false, true, false },
		{ "coin-08.png", false, true, false, true, false },
		{ "coin-09.png", false, true, false, true, false },
		{ "coin-10.png", false, true, false, true, false },
		{ "coin-11.png", false, true, false, true, false },
		{ "coin-12.png", false, true, false, true, false },
		{ "hud-powerup.png", false, false, false, false, false },
		{ "hud-powerup-double.png", false, false, false, false, false },
		{ "hud-powerup-triple.png", false, false, false, false, false },
		{ "hud-powerup-fan.png", false, false, false, false, false },
		{ "powerup.png", false, true, false, false, false },
		{ "powerup-double.png", false, true, false, false, false },
		{ "powerup-double-01.png", false, true, false, false, false },
		{ "powerup-double-02.png", false, true, false, false, false },
		{ "powerup-double-x2.png", false, true, false, false, false },
		{ "powerup-triple.png", false, true, false, false, false },
		{ "powerup-triple-01.png", false, true, false, false, false },
		{ "powerup-triple-02.png", false, true, false, false, false },
		{ "powerup-triple-x2.png", false, true, false, false, false },
		{ "powerup-fan.png", false, true, false, false, false },
		{ "grape-01.png", false, true, false, false, false },
		{ "grape-02.png", false, true, false, false, false },
		{ "grapes.png", false, true, false, false, false },
		{ "pineapple.png", false, true, false, false, false },
		{ "pineapple-01.png", false, true, false, false, false },
		{ "pineapple-02.png", false, true, false, false, false },
		{ "battery-pack.png", false, true, false, false, false },
		{ "battery-pack-01.png", false, true, false, false, false },
		{ "battery-pack-02.png", false, true, false, false, false },

		{ "battery.png", true, true, true, true, false },
		{ "battery-half.png", true, true, true, true, false },
		{ "battery-low-01.png", true, true, true, true, false },
		{ "battery-low-02.png", true, true, true, true, false },

		{ "battery-none.png", false, false, true, false, false },
		{ "battery-none-01.png", false, false, true, false, false },
		{ "battery-none-02.png", false, false, true, false, false },
		{ "life.png", false, false, false, false, false },
		{ "life-half.png", false, false, false, false, false },
		{ "life-none.png", false, false, false, false, false },
		{ "lm-presents.png", false, false, false, false, false },
		{ "cherries.png", false, true, false, false, false },
		{ "cherries-01.png", false, true, false, false, false },
		{ "cherries-02.png", false, true, false, false, false },
		{ "super-streak.png", false, false, false, false, false },
		{ "super-fleck.png", false, false, false, false, false },
		{ "level-1.png", false, false, false, false, false },
		{ "game-over.png", false, false, false, false, false },
		{ "title.png", false, false, false, false, false },

		{ "base-chip.png", false, false, false, false, true },
		{ "base-resistor.png", false, false, false, false, true },
		{ "base-resistor-2.png", false, false, false, false, true },
		{ "base-large.png", false, false, false, false, true },
		{ "base-large-chip.png", false, false, false, false, true },
		{ "base-large-resistor.png", false, false, false, false, true },
		{ "base-large-terminal.png", false, false, false, false, true },
		{ "base-large-n.png", false, false, false, false, true },
		{ "base-large-e.png", false, false, false, false, true },
		{ "base-large-s.png", false, false, false, false, true },
		{ "base-large-w.png", false, false, false, false, true },
		{ "base-large-ne.png", false, false, false, false, true },
		{ "base-large-nw.png", false, false, false, false, true },
		{ "base-large-se.png", false, false, false, false, true },
		{ "base-large-sw.png", false, false, false, false, true },
		{ "base-large-chip-n.png", false, false, false, false, true },
		{ "base-large-chip-e.png", false, false, false, false, true },
		{ "base-large-chip-s.png", false, false, false, false, true },
		{ "base-large-chip-w.png", false, false, false, false, true },

		{ "base-large-double.png", false, false, false, false, true },
		{ "base-large-double-chip.png", false, false, false, false, true },
		{ "base-large-double-n.png", false, false, false, false, true },
		{ "base-large-double-e.png", false, false, false, false, true },
		{ "base-large-double-s.png", false, false, false, false, true },
		{ "base-large-double-w.png", false, false, false, false, true },
		{ "base-large-double-ne.png", false, false, false, false, true },
		{ "base-large-double-nw.png", false, false, false, false, true },
		{ "base-large-double-se.png", false, false, false, false, true },
		{ "base-large-double-sw.png", false, false, false, false, true },
		{ "base-large-double-chip-n.png", false, false, false, false, true },
		{ "base-large-double-chip-e.png", false, false, false, false, true },
		{ "base-large-double-chip-s.png", false, false, false, false, true },
		{ "base-large-double-chip-w.png", false, false, false, false, true },

		{ "base.png", false, false, false, false, true },
		{ "base-n.png", false, false, false, false, true },
		{ "base-n-resistor.png", false, false, false, false, true },
		{ "base-n-resistor-2.png", false, false, false, false, true },
		{ "base-s.png", false, false, false, false, true },
		{ "base-s-resistor.png", false, false, false, false, true },
		{ "base-s-resistor-2.png", false, false, false, false, true },
		{ "base-e.png", false, false, false, false, true },
		{ "base-e-resistor-2.png", false, false, false, false, true },
		{ "base-w.png", false, false, false, false, true },
		{ "base-w-resistor.png", false, false, false, false, true },
		{ "base-w-resistor-2.png", false, false, false, false, true },
		{ "base-ne.png", false, false, false, false, true },
		{ "base-nw.png", false, false, false, false, true },
		{ "base-se.png", false, false, false, false, true },
		{ "base-sw.png", false, false, false, false, true },
		{ "base-ne-inner.png", false, false, false, false, true },
		{ "base-nw-inner.png", false, false, false, false, true },
		{ "base-se-inner.png", false, false, false, false, true },
		{ "base-sw-inner.png", false, false, false, false, true },
		{ "planet-01.png", false, false, false, false, true },
		{ "planet-02.png", false, false, false, false, true },
		{ "planet-03.png", false, false, false, false, true },
		{ "planet-04.png", false, false, false, false, true },
		{ "space.png", false, false, false, false, true },
		{ "space-01.png", false, false, false, false, true },
		{ "space-02.png", false, false, false, false, true },
		{ "space-03.png", false, false, false, false, true },
		{ "space-04.png", false, false, false, false, true },
		{ "space-05.png", false, false, false, false, true },
		{ "space-06.png", false, false, false, false, true },
		{ "space-07.png", false, false, false, false, true },
		{ "space-08.png", false, false, false, false, true },

		{ "virus-shot.png", false, true, false, false, false },
		{ "shot-blue-01.png", false, true},
		{ "shot-blue-02.png", false, true},
		{ "shot-neon-01.png", false, true},
		{ "shot-neon-02.png", false, true},
		{ "shot-aqua.png", false, true, false, false, false },
		{ "shot-orange.png", false, true, false, false, false },
		{ "speech-entry.png", false, false, false, false, false },
		{ "speech-coin.png", false, false, false, false, false },
		{ "virus-01.png", true, true, false, false, false },
		{ "virus-02.png", true, true, false, false, false },
		{ "virus-03.png", true, true, false, false, false },
		{ "virus-04.png", true, true, false, false, false },
		{ "virus-05.png", true, true, false, false, false },
		{ "virus-06.png", true, true, false, false, false },
		{ "bug-01.png", true, true, false, false, false },
		{ "bug-02.png", true, true, false, false, false },
		{ "bug-03.png", true, true, false, false, false },
		{ "bug-04.png", true, true, false, false, false },
		{ "bug-05.png", true, true, false, false, false },
		{ "bug-06.png", true, true, false, false, false },
		{ "cd-01.png", true, true, false, false, false },
		{ "cd-02.png", true, true, false, false, false },
		{ "cd-03.png", true, true, false, false, false },
		{ "cd-04.png", true, true, false, false, false },
		{ "disk-01.png", true, true, false, false, false },
		{ "disk-02.png", true, true, false, false, false },
		{ "disk-03.png", true, true, false, false, false },
		{ "disk-04.png", true, true, false, false, false },
		{ "disk-05.png", true, true, false, false, false },
		{ "disk-06.png", true, true, false, false, false },
		{ "disk-07.png", true, true, false, false, false },
		{ "disk-08.png", true, true, false, false, false },
		{ "disk-09.png", true, true, false, false, false },
		{ "disk-10.png", true, true, false, false, false },
		{ "disk-11.png", true, true, false, false, false },
		{ "disk-12.png", true, true, false, false, false },
		{ "disk-blue-01.png", true, true, false, false, false },
		{ "disk-blue-02.png", true, true, false, false, false },
		{ "disk-blue-03.png", true, true, false, false, false },
		{ "disk-blue-04.png", true, true, false, false, false },
		{ "disk-blue-05.png", true, true, false, false, false },
		{ "disk-blue-06.png", true, true, false, false, false },
		{ "disk-blue-07.png", true, true, false, false, false },
		{ "disk-blue-08.png", true, true, false, false, false },
		{ "disk-blue-09.png", true, true, false, false, false },
		{ "disk-blue-10.png", true, true, false, false, false },
		{ "disk-blue-11.png", true, true, false, false, false },
		{ "disk-blue-12.png", true, true, false, false, false },
		{ "mike-facing-01.png", true, true, true, true, false },
		{ "mike-facing-02.png", true, true, true, true, false },
		{ "mike-facing-03.png", true, true, true, true, false },
		{ "mike-facing-04.png", true, true, true, true, false },
		{ "mike-facing-05.png", true, true, true, true, false },
		{ "mike-facing-06.png", true, true, true, true, false },
		{ "mike-facing-07.png", true, true, true, true, false },
		{ "mike-facing-08.png", true, true, true, true, false },
		{ "mike-01.png", true, true, true, true, false },
		{ "mike-02.png", true, true, true, true, false },
		{ "mike-03.png", true, true, true, true, false },
		{ "mike-04.png", true, true, true, true, false },
		{ "mike-05.png", true, true, true, true, false },
		{ "mike-06.png", true, true, true, true, false },
		{ "mike-07.png", true, true, true, true, false },
		{ "mike-08.png", true, true, true, true, false },
		{ "mike-lean-left-01.png", true, true, false, true, false },
		{ "mike-lean-left-02.png", true, true, false, true, false },
		{ "mike-lean-left-03.png", true, true, false, true, false },
		{ "mike-lean-left-04.png", true, true, false, true, false },
		{ "mike-lean-left-05.png", true, true, false, true, false },
		{ "mike-lean-left-06.png", true, true, false, true, false },
		{ "mike-lean-left-07.png", true, true, false, true, false },
		{ "mike-lean-left-08.png", true, true, false, true, false },
		{ "mike-lean-right-01.png", true, true, false, true, false },
		{ "mike-lean-right-02.png", true, true, false, true, false },
		{ "mike-lean-right-03.png", true, true, false, true, false },
		{ "mike-lean-right-04.png", true, true, false, true, false },
		{ "mike-lean-right-05.png", true, true, false, true, false },
		{ "mike-lean-right-06.png", true, true, false, true, false },
		{ "mike-lean-right-07.png", true, true, false, true, false },
		{ "mike-lean-right-08.png", true, true, false, true, false },
		{ "mike-shoot-evil-01.png", true, true, false, true, false },
		{ "mike-shoot-evil-02.png", true, true, false, true, false },
		{ "mike-shoot-01.png", true, true, false, true, false },
		{ "mike-shoot-02.png", true, true, false, true, false },
		{ "mike-shoot-left-01.png", true, true, false, true, false },
		{ "mike-shoot-left-02.png", true, true, false, true, false },
		{ "mike-shoot-right-01.png", true, true, false, true, false },
		{ "mike-shoot-right-02.png", true, true, false, true, false },
		{ "mike-shock.png", false, true, false, true, false },
		{ "mike-shock2.png", false, true, false, true, false },
		{ "mike-shock3.png", false, true, false, true, false },
		{ "exp-01.png", false, true, false, false, false },
		{ "exp-02.png", false, true, false, false, false },
		{ "exp-03.png", false, true, false, false, false },
		{ "exp-04.png", false, true, false, false, false },
		{ "exp-05.png", false, true, false, false, false },
		{ "exp-06.png", false, true, false, false, false },
		{ "bg.png", false, false, false, false, false },
		{ "bg-8.png", false, false, false, false, false },
		{ "star-bright.png", false, false, false, false, true },
		{ "star-dim.png", false, false, false, false, false },
		{ "star-dark.png", false, false, false, false, false },
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

static void loadSounds() {
	const int SOUND_VOLUME = 12;

	SoundDef defs[] = {
		{ "mike-die.wav", SOUND_VOLUME * 4 },
		{ "intro-presents.wav", SOUND_VOLUME * 2 },
		{ "warning.wav", SOUND_VOLUME * 2.25 },
		{ "Powerup9.wav", SOUND_VOLUME * 4 },
		{ "Powerup8.wav", SOUND_VOLUME * 4 },
		{ "loss.wav", SOUND_VOLUME * 5 },
		{ "Pickup_Coin4.wav", (int)ceil(SOUND_VOLUME * 1.5) },
		{ "Pickup_Coin14.wav", (int)ceil(SOUND_VOLUME * 1.5) },
		{ "Pickup_Coin34.wav", SOUND_VOLUME },
		{ "Pickup_Coin34b.wav", (int)ceil(SOUND_VOLUME * 2.5) },
		{ "ping.wav", (int)ceil(SOUND_VOLUME * 2) },
		{ "ping2.wav", (int)ceil(SOUND_VOLUME * 2) },
		{ "warp.wav", SOUND_VOLUME },
		{ "start.wav", SOUND_VOLUME },
		{ "Hit_Hurt10.wav", (int)ceil(SOUND_VOLUME * 3) },		// When we get hit.
		{ "Hit_Hurt18.wav", (int)ceil(SOUND_VOLUME * 3) },		// When we get hit.
		{ "Hit_Hurt9.wav", SOUND_VOLUME },
		{ "Laser_Shoot34.wav", SOUND_VOLUME },					// Enemy shot
		{ "Laser_Shoot18.wav", SOUND_VOLUME / 1.5 },			// Player shot.
		{ "Laser_Shoot5.wav", SOUND_VOLUME / 4 },
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

static void loadMusic() {
	char* defs[] = {
		"tension.ogg",
		"intro-battle-3.ogg",
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

		Mix_VolumeMusic(MUSIC_VOLUME);

		//Add to register
		MusicAsset snd = {
			defs[i],
			chunk
		};
		music[i] = snd;
	}
}

void initAssets() {
	loadImages();
	loadSounds();
	loadMusic();
}
