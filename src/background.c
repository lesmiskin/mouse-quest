#include "myc.h"
#include <time.h>
#include "mysdl.h"
#include "renderer.h"
#include "assets.h"
#include "common.h"

typedef struct {
	Coord origin;
	Sprite sprite;
	double speed;
} Planet;

typedef struct {
	char map[10][20];							//Vertical rectangle (width / 2, height + 1/3)
	Coord origin;
	Sprite sprite;
} Platform;

typedef struct {
	Coord position;
	int layer;
	int brightness;
} Star;

typedef enum {
	TILE_NULL = 0,
	TILE_NORTH = 1,
	TILE_SOUTH = 2,
	TILE_WEST = 4,
	TILE_EAST = 8
} TileDirection;

double bgOffset;
bool showBackground;
bool staticBackground;
static const double SCROLL_SPEED = 0.5;			//TODO: Should be in FPS.

//PLANETS
#define MAX_PLANETS 10
static Planet planets[MAX_PLANETS];
static long lastPlanetTime;
static double nextPlanetSpawnSeconds;
static int PLANET_SPAWN_MIN_SECONDS = 5;
static int PLANET_SPAWN_MAX_SECONDS = 15;
static int planetInc;
static int PLANET_SPEED_MIN = 6;				//will /10
static int PLANET_SPEED_MAX = 6;
static int PLANET_BOUND = 32;

//PLATFORMS
#define MAX_PLATFORMS 3
static long lastPlatformTime;
static double nextPlatformSpawnSeconds;
static int PLATFORM_SPAWN_MIN_SECONDS = 10;
static int PLATFORM_SPAWN_MAX_SECONDS = 20;
static double PLATFORM_SCROLL_SPEED = 0.7;
static const int PLATFORM_SCALE = 2;
static Platform platforms[MAX_PLATFORMS];
static int platformInc;
static Sprite baseSprite;
static const int PLATFORM_TILE_SIZE = 20;
static const int PLATFORM_SEED_X = 3;
static const int PLATFORM_SEED_Y = 3;

//STARS
#define MAX_STARS 64
static bool starsBegun = false;
static int STAR_DELAY = 150;	//lower is greater.
static Star stars[MAX_STARS];
static long lastStarTime = 0;
static int starInc = 0;

static bool invalidPlanet(Planet* planet) {
	return
		planet->speed == 0 ||
		planet->origin.y - PLANET_BOUND > screenBounds.y;
}

static bool invalidPlatform(Platform* platform) {
	return
		platform->origin.x == 0 ||
		platform->origin.y - (PLATFORM_SEED_Y * PLATFORM_SCALE * PLATFORM_TILE_SIZE) > screenBounds.y;
}

SDL_Texture* createPlatformTexture() {
	return SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_UNKNOWN,
		SDL_TEXTUREACCESS_TARGET,
		PLATFORM_SEED_X * PLATFORM_SCALE * PLATFORM_TILE_SIZE,
		PLATFORM_SEED_Y * PLATFORM_SCALE * PLATFORM_TILE_SIZE
	);
}

bool skipClutter() {
    //Don't do planet, platform rendering, or star scrolling for static backgrounds.
    return staticBackground || (
        gameState != STATE_GAME &&
        gameState != STATE_GAME_OVER &&
        gameState != STATE_LEVEL_COMPLETE);
}

void foregroundRenderFrame() {
    if(skipClutter()) return;

	//Draw 'base' platforms.
	for(int i=0; i < MAX_PLATFORMS; i++) {
		//Skip null platforms.
		if(invalidPlatform(&platforms[i])) continue;

		Coord parallaxOrigin = parallax(platforms[i].origin, PARALLAX_PAN, PARALLAX_LAYER_PLATFORM, PARALLAX_X, PARALLAX_ADDITIVE);
		drawSpriteAbs(platforms[i].sprite, parallaxOrigin);
	}
}

void backgroundRenderFrame() {

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	if(!showBackground) {
		return;
	}

	//Draw nebulous background.
//	Sprite s = makeSprite(getTexture("bg.png"), zeroCoord(), SDL_FLIP_VERTICAL);
//	drawSpriteAbsRotated2(s, zeroCoord(), 0, 2);

	//Display an initial screen of stars.
	if(!starsBegun) {
		for(int i=0; i < 40; i++) {
			Star star = {
				makeCoord(
					randomMq(0, screenBounds.x),
					randomMq(0, screenBounds.y)
				),
				randomMq(0, 2),		//layer
				randomMq(0, 2),		//brightness
			};
			stars[i++] = star;
		}
		starsBegun = true;
	}

	//Spawn stars based on designated density.
	if(timer(&lastStarTime, STAR_DELAY)) {
		Star star = {
			makeCoord(
				randomMq(0, screenBounds.x),  //spawn across the width of the screen
				0
			),
			randomMq(0, 2),		//layer
			randomMq(0, 2),		//brightness
		};

		starInc = starInc == MAX_STARS ? 0 : starInc++;
		stars[starInc++] = star;
	}

	//Render stars.
	for(int i=0; i < MAX_STARS; i++) {
		Sprite sprite = makeSprite(getTexture(
			stars[i].layer == 0 ? "star-dark.png" : stars[i].layer == 1 ? "star-dim.png" : "star-bright.png"
		), zeroCoord(), SDL_FLIP_NONE);

		Coord parallaxOrigin = parallax(stars[i].position, PARALLAX_PAN, PARALLAX_LAYER_STAR, PARALLAX_X, PARALLAX_ADDITIVE);

		drawSprite(sprite, parallaxOrigin);
	}

    if(skipClutter()) return;

    //We show two large textures, one after the other, to create a seamless scroll. Once the first texture
	// moves out of the viewport, however, we snap it back to the top.

	//Render planets.
	for(int i=0; i < MAX_PLANETS; i++) {
		//Skip null planets.
		if(invalidPlanet(&planets[i])) continue;
		Coord parallaxOrigin = parallax(planets[i].origin, PARALLAX_PAN, PARALLAX_LAYER_PLANET, PARALLAX_X, PARALLAX_ADDITIVE);
		drawSpriteAbs(planets[i].sprite, parallaxOrigin);
	}
}

Platform makePlatform(Coord origin) {
	//Hardcoded seedmap
	int seedMap[3][3] = {
			{ 1, 1, 1 },
			{ 1, 1, 1 },
			{ 1, 1, 1 }
	};

	//Randomised seedmap
//	for(int x=0; x < PLATFORM_SEED_X; x++){
//		for(int y=0; y < PLATFORM_SEED_Y; y++) {
//			seedMap[x][y] = chance(PLATFORM_SEED_DENSITY);
//		}
//	}

	//Create tile map canvas texture.
	SDL_Texture* canvas = createPlatformTexture();

	//Change renderer context to output onto the tilemap.
	SDL_SetRenderTarget(renderer, canvas);

	//Make transparent (initially)
	SDL_SetTextureBlendMode(canvas, SDL_BLENDMODE_BLEND);
	SDL_RenderFillRect(renderer, NULL);

	Platform p;
	p.origin = origin;

	//Local variables.
	Coord tileSize = makeCoord(PLATFORM_TILE_SIZE, PLATFORM_TILE_SIZE);

	//Get base asset.

	//Replicate the tile across the screen area, operating on seed map to scale.
	int xTile = 0;
	for(double x=0; x < PLATFORM_SEED_X * PLATFORM_SCALE; x += 1 / (double)PLATFORM_SCALE, xTile += tileSize.x){
		int yTile = 0;
		for(double y=0; y < PLATFORM_SEED_Y * PLATFORM_SCALE; y += 1 / (double)PLATFORM_SCALE, yTile += tileSize.y) {
			if(!seedMap[(int)floor(x)][(int)floor(y)]) continue;
			SDL_Rect destination  = {
					xTile, yTile,
					(int)tileSize.x, (int)tileSize.y
			};

			int ix = (int)floor(x);
			int iy = (int)floor(y);

			TileDirection result = TILE_NULL;

			char* filename = NULL;
			if(floor(x) != x && (ix+1 == PLATFORM_SEED_X || !seedMap[ix+1][iy])) {
				result |= TILE_EAST;
			}else if(x == 0 || (floor(x) == x && !seedMap[ix-1][iy])) {
				result |= TILE_WEST;
			}

			if(y == 0 || (floor(y) == y && !seedMap[ix][iy-1])) {
				result |= TILE_NORTH;
			}else if(floor(y) != y && (iy+1 == PLATFORM_SEED_Y || (!seedMap[ix][iy+1]))) {
				result |= TILE_SOUTH;
			}

			switch(result) {
				case TILE_NORTH | TILE_EAST:
					filename = "base-large-ne.png";
					break;
				case TILE_NORTH | TILE_WEST:
					filename = "base-large-nw.png";
					break;
				case TILE_SOUTH | TILE_EAST:
					filename = "base-large-se.png";
					break;
				case TILE_SOUTH | TILE_WEST:
					filename = "base-large-sw.png";
					break;
				case TILE_NORTH:{
					filename = "base-large-n.png";
					break;
				}case TILE_SOUTH:{
					filename = "base-large-s.png";
					break;
				}case TILE_EAST:
					filename = "base-large-e.png";
					break;
				case TILE_WEST: {
					filename = "base-large-w.png";
					break;
				}
				default: {
					filename = chance(50) ?
					   "base-large-chip.png" :
					   "base-large-resistor.png";
					break;
				}
			}

			SDL_Texture *baseTexture = getTexture(filename);
			baseSprite = makeSprite(baseTexture, zeroCoord(), SDL_FLIP_NONE);

			SDL_RenderCopy(renderer, baseSprite.texture, NULL, &destination);
		}
	}

	//Restore renderer context back to the window canvas.
	SDL_SetRenderTarget(renderer, renderBuffer);
	p.sprite = makeSprite(canvas, zeroCoord(), SDL_FLIP_NONE);

	return p;
}

void backgroundGameFrame() {

	if(!showBackground || staticBackground) return;

	//Chunk the textures downwards on each frame, or reset if outside view.
	bgOffset = bgOffset > screenBounds.y ? 0 : bgOffset + SCROLL_SPEED;

	//Spawn planets.
	if(timer(&lastPlanetTime, toMilliseconds(nextPlanetSpawnSeconds))) {
		if(planetInc == MAX_PLANETS-1) planetInc = 0;

		//Choose random planet type.
		char planetFile[50];
		int randPlanet = randomMq(1, 4);
		sprintf(planetFile, "planet-%02d.png", randPlanet);

		SDL_Texture *planetTexture = getTexture(planetFile);
		Sprite planetSprite = makeSprite(planetTexture, zeroCoord(), SDL_FLIP_NONE);

		Planet planet = {
			makeCoord(randomMq(0, (int)screenBounds.x), -PLANET_BOUND),
			planetSprite,
			(randomMq(PLANET_SPEED_MIN, PLANET_SPEED_MAX)) * 0.1
		};
		planets[++planetInc] = planet;

		//Spawn next planet at a random time.
		nextPlanetSpawnSeconds = randomMq(PLANET_SPAWN_MIN_SECONDS, PLANET_SPAWN_MAX_SECONDS);
	}

	//Scroll stars.
	for(int i=0; i < MAX_STARS; i++) {
		//Different star 'distances' scroll at different speeds.
		stars[i].position.y +=
			stars[i].brightness == 0 ? 0.75 : stars[i].brightness == 1 ? 1 : 1.25;
	}

	//Scroll planets.
	for(int i=0; i < MAX_PLANETS; i++) {
		if(invalidPlanet(&planets[i])) continue;
		planets[i].origin.y += planets[i].speed;
	}

	if(timer(&lastPlatformTime, toMilliseconds(nextPlatformSpawnSeconds))) {
		if(platformInc == MAX_PLATFORMS-1) platformInc = 0;

		Coord origin = makeCoord(randomMq(0, (int)screenBounds.x), -(PLATFORM_SEED_Y * PLATFORM_SCALE * PLATFORM_TILE_SIZE) / 2);
		Platform platform = makePlatform(origin);
		platforms[++platformInc] = platform;

		//Spawn next planet at a random time.
		nextPlatformSpawnSeconds = randomMq(PLATFORM_SPAWN_MIN_SECONDS, PLATFORM_SPAWN_MAX_SECONDS);
	}

	//Scroll platforms.
	for(int i=0; i < MAX_PLATFORMS; i++) {
		if(invalidPlatform(&platforms[i])) continue;
		platforms[i].origin.y += PLATFORM_SCROLL_SPEED;
	}
}

void resetBackground() {
	memset(planets, 0, sizeof(planets));
	memset(platforms, 0, sizeof(platforms));
	showBackground = true;
	staticBackground = false;
}

void initBackground() {
	resetBackground();
}

