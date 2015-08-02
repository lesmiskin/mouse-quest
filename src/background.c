#include <time.h>
#include "SDL.h"
#include "renderer.h"
#include "assets.h"
#include "common.h"
#include "player.h"

typedef struct {
	Coord origin;
	Sprite sprite;
	double speed;
} Planet;

typedef enum {
	TILE_NONE,
	TILE_BASE,
	TILE_CHIP
} TileType;

#define PLATFORM_TILE_X 6
#define PLATFORM_TILE_Y 6

typedef struct {
	TileType tiles[PLATFORM_TILE_X][PLATFORM_TILE_Y];
	Coord origin;
	Sprite sprite;  
} Platform;

double bgOffset;
bool showBackground;
bool staticBackground;

static const int BASE_SIZE = 20;

#define MAX_PLANETS 4
static Sprite spaceSprite;
static const double SCROLL_SPEED = 0.5;			//TODO: Should be in FPS.
static Planet planets[MAX_PLANETS];
static int tileFrame;

static SDL_Texture* tileMaps[4];
static const int TILE_FRAMES = 4;
static double TILE_HERTZ_SECONDS = 100;
static long lastTileFrameTime;

static long lastPlanetTime;
static double nextPlanetSpawnSeconds;
static int PLANET_SPAWN_MIN_SECONDS = 5;
static int PLANET_SPAWN_MAX_SECONDS = 15;
static int planetInc;
static int PLANET_SPEED_MIN = 6;				//will /10
static int PLANET_SPEED_MAX = 6;
static int PLANET_BOUND = 32;

#define MAX_PLATFORMS 10
static long lastPlatformTime;
static double nextPlatformSpawnSeconds;
static int PLATFORM_SPAWN_MIN_SECONDS = 5;
static int PLATFORM_SPAWN_MAX_SECONDS = 5;
static double PLATFORM_SCROLL_SPEED = 0.8;
static int platformInc;
static Sprite baseSprite;
static SDL_Texture* platformCanvas;
static Platform platforms[MAX_PLATFORMS];

static bool isNullPlanet(Planet* planet) {
	return planet->speed == 0;
}

SDL_Texture* initBackgroundFrame(char* filename) {
	//We render out the tile map to a big texture, that we blit onto the screen on each frame.
	// This saves a lot of processing time drawing out individual tiles on each frame.
	//Note that we render out to a 1:1 ratio (i.e. smaller than the actual screen, then scale
	// this up via the renderScale factor, so we only need a small physical image in memory, i.e.
	// scaling is done when applying the texture.

	//Load the spaceTexture
	SDL_Texture *spaceTexture = getTexture(filename);
	spaceSprite = makeSprite(spaceTexture, zeroCoord(), SDL_FLIP_NONE);

	//We want to scale the sprite_t up to our pixel grid.
	Coord tileSize = makeCoord(
		spaceSprite.size.x,
		spaceSprite.size.y
	);

	//Create tile map canvas texture.
	SDL_Texture* tileMap = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_UNKNOWN,
		SDL_TEXTUREACCESS_TARGET,
		(int)pixelGrid.x, (int)pixelGrid.y
	);

	//Change renderer context to output onto the tilemap.
	SDL_SetRenderTarget(renderer, tileMap);

	//Replicate the tile across the screen area.
	for(int x=0; x < pixelGrid.x; x += tileSize.x){
		for(int y=0; y < pixelGrid.y; y += tileSize.y) {
			SDL_Rect destination  = {
				x, y,
				(int)tileSize.x, (int)tileSize.y
			};

			SDL_RenderCopy(renderer, spaceSprite.texture, NULL, &destination);
		}
	}

	//Restore renderer context back to the window canvas.
	SDL_SetRenderTarget(renderer, NULL);

	return tileMap;
}

void backgroundRenderFrame(void) {

	//If we're not showing the background, at least clear it on every frame so we don't keep previously-
	// rendered sprites around on it (think: Doom noclip).
	if(!showBackground) {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);
		return;
	}

	//We show two large textures, one after the other, to create a seamless scroll. Once the first texture
	// moves out of the viewport, however, we snap it back to the top.

	//Animate.
	if(timer(&lastTileFrameTime, TILE_HERTZ_SECONDS)) {
		if(tileFrame == TILE_FRAMES-1){
			tileFrame = 0;
		}else{
			tileFrame++;
		}
	}
	SDL_Texture *tileMap = tileMaps[tileFrame];

	//Blit out our first texture
	SDL_Rect destination  = {
		0, (int)bgOffset,
		(int)windowSize.x, (int)windowSize.y
	};
	SDL_RenderCopy(renderer, tileMap, NULL, &destination);

	//Blit out our second texture, offset by the first texture accordingly.
	SDL_Rect destination2 = {
		0, (int)bgOffset - (int)windowSize.y,
		(int)windowSize.x, (int)windowSize.y
	};
	SDL_RenderCopy(renderer, tileMap, NULL, &destination2);

	//Blit the fader overlay to the screen.
	SDL_Rect faderDestination = {
		0, 0,
		(int)windowSize.x, (int)windowSize.y
	};

	//Don't do planet or platform rendering for static backgrounds.
	if(staticBackground) return;

	//Render planets.
	for(int i=0; i < MAX_PLANETS; i++) {
		//Skip null planets.
		if(isNullPlanet(&planets[i])) continue;
		Coord parallaxOrigin = parallax(planets[i].origin, PARALLAX_PAN, PARALLAX_LAYER_PLANET, PARALLAX_X, PARALLAX_ADDITIVE);
		drawSpriteAbs(planets[i].sprite, parallaxOrigin);
	}

	//Draw 'base' platforms.
	for(int i=0; i < MAX_PLATFORMS; i++) {
		//Skip null platforms.
		if(platforms[i].origin.x == 0) continue;

		Coord parallaxOrigin = parallax(platforms[i].origin, PARALLAX_PAN, PARALLAX_LAYER_PLATFORM, PARALLAX_X, PARALLAX_ADDITIVE);
		drawSpriteAbs(platforms[i].sprite, parallaxOrigin);
	}
}

typedef enum {
	TILE_NULL = 0,
	TILE_NORTH = 1,
	TILE_SOUTH = 2,
	TILE_WEST = 4,
	TILE_EAST = 8
} TileDirection;

Platform makePlatform(Platform *platform) {
	//Create tile map canvas texture.
	SDL_Texture* canvas = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_UNKNOWN,
		SDL_TEXTUREACCESS_TARGET,
		PLATFORM_TILE_X * BASE_SIZE,
		PLATFORM_TILE_Y * BASE_SIZE
	);	bool seedMap[PLATFORM_TILE_X][PLATFORM_TILE_Y];

	//Change renderer context to output onto the tilemap.
	SDL_SetRenderTarget(renderer, canvas);

	//Make transparent (initially)
	SDL_RenderClear(renderer);		//clear previous drawing data (e.g. from fader).
	SDL_SetTextureBlendMode(canvas, SDL_BLENDMODE_BLEND);

	//Replicate the tile across the screen area, operating on seed map to scale.
	int xTile = 0;
	for(int x=0; x < PLATFORM_TILE_X; x++, xTile += BASE_SIZE){
		int yTile = 0;
		for(int y=0; y < PLATFORM_TILE_Y; y++, yTile += BASE_SIZE) {

			char* filename;

			switch(platform->tiles[y][x]) {
				case TILE_NONE:
					continue;
				case TILE_BASE:
					filename = "base-large.png";
					break;
				case TILE_CHIP:
					filename = "base-large-chip.png";
					break;
			}

			SDL_Rect destination  = {
				xTile, yTile,
				BASE_SIZE, BASE_SIZE
			};

			//Grab texture
			SDL_Texture *baseTexture = getTexture(filename);
			baseSprite = makeSprite(baseTexture, zeroCoord(), SDL_FLIP_NONE);

			//Draw it onto the platform canvas
			SDL_RenderCopy(renderer, baseSprite.texture, NULL, &destination);
		}
	}

	//Restore renderer context back to the window canvas.
	SDL_SetRenderTarget(renderer, NULL);
	
	platform->sprite = makeSprite(canvas, zeroCoord(), SDL_FLIP_NONE);
}

void backgroundGameFrame(void) {

	if(!showBackground || staticBackground) return;

	//Chunk the textures downwards on each frame, or reset if outside view.
	bgOffset = bgOffset > windowSize.y ? 0 : bgOffset + SCROLL_SPEED;

	//Spawn planets.
	if(timer(&lastPlanetTime, toMilliseconds(nextPlanetSpawnSeconds))) {
		if(planetInc == MAX_PLANETS-1) planetInc = 0;

		//Choose random planet type.
		char planetFile[50];
		int randPlanet = random(1, 4);
		sprintf(planetFile, "planet-%02d.png", randPlanet);

		SDL_Texture *planetTexture = getTexture(planetFile);
		Sprite planetSprite = makeSprite(planetTexture, zeroCoord(), SDL_FLIP_NONE);

		Planet planet = {
			makeCoord(random(0, (int)screenBounds.x), -PLANET_BOUND),
			planetSprite,
			(random(PLANET_SPEED_MIN, PLANET_SPEED_MAX)) * 0.1
		};
		planets[++planetInc] = planet;

		//Spawn next planet at a random time.
		nextPlanetSpawnSeconds = random(PLANET_SPAWN_MIN_SECONDS, PLANET_SPAWN_MAX_SECONDS);
	}

	//Scroll planets.
	for(int i=0; i < MAX_PLANETS; i++) {
		planets[i].origin.y += planets[i].speed;
	}

	//Spawn platforms.
	if(timer(&lastPlatformTime, toMilliseconds(nextPlatformSpawnSeconds))) {
		if(platformInc == MAX_PLATFORMS-1) platformInc = 0;

		Platform p = {
			{
			{ 1, 1, 1, 1, 1, 1 },
			{ 1, 2, 2, 2, 2, 1 },
			{ 1, 1, 2, 2, 1, 1 },
			{ 0, 0, 1, 1, 0, 0 },
			{ 0, 0, 0, 0, 0, 0 },
			{ 0, 0, 0, 0, 0, 0 }
			},
			makeCoord(
				random(0, (int)screenBounds.x),
				-(PLATFORM_TILE_Y * BASE_SIZE) / 2
			)
		};
//		makePlatform(&p);
//		platforms[++platformInc] = p;

		//Spawn next planet at a random time.
		nextPlatformSpawnSeconds = random(PLATFORM_SPAWN_MIN_SECONDS, PLATFORM_SPAWN_MAX_SECONDS);
	}

	//Scroll planets.
	for(int i=0; i < MAX_PLATFORMS; i++) {
		platforms[i].origin.y += PLATFORM_SCROLL_SPEED;
	}
}

void resetBackground(void) {
	memset(planets, 0, sizeof(planets));
	memset(platforms, 0, sizeof(platforms));
	showBackground = true;
	staticBackground = false;
}

void initBackground(void) {
	resetBackground();

	//Pre-render background tile animation frames, and load into array.
	for(int i=0; i < TILE_FRAMES; i++) {
		char filename[10];
		sprintf(filename, "space-%02d.png", i+1);
		tileMaps[i] = initBackgroundFrame(filename);
	}
}
