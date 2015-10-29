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

typedef struct {
	char map[10][20];							//Vertical rectangle (width / 2, height + 1/3)
	Coord origin;
	Sprite sprite;
} Platform;

double bgOffset;
bool showBackground;
bool staticBackground;

#define MAX_PLANETS 10
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

#define MAX_PLATFORMS 3
static long lastPlatformTime;
static double nextPlatformSpawnSeconds;
static int PLATFORM_SPAWN_MIN_SECONDS = 5;
static int PLATFORM_SPAWN_MAX_SECONDS = 10;
static double PLATFORM_SCROLL_SPEED = 0.7;
static const int PLATFORM_SCALE = 2;
static Platform platforms[MAX_PLATFORMS];
static int platformInc;
static Sprite baseSprite;
static SDL_Texture* platformCanvas;

static const int PLATFORM_TILE_SIZE = 20;
static const int PLATFORM_SEED_X = 3;
static const int PLATFORM_SEED_Y = 3;
static const int PLATFORM_SEED_DENSITY = 75;

static bool isNullPlanet(Planet* planet) {
	return planet->speed == 0;
}

SDL_Texture* createPlatformTexture(void) {
	return SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_UNKNOWN,
		SDL_TEXTUREACCESS_TARGET,
		(int)PLATFORM_SEED_X * PLATFORM_SCALE * PLATFORM_TILE_SIZE,
		(int)PLATFORM_SEED_Y * PLATFORM_SCALE * PLATFORM_TILE_SIZE
	);
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
		(int)screenBounds.x, (int)screenBounds.y
	};
	SDL_RenderCopy(renderer, tileMap, NULL, &destination);

	//Blit out our second texture, offset by the first texture accordingly.
	SDL_Rect destination2 = {
		0, (int)bgOffset - (int)screenBounds.y,
		(int)screenBounds.x, (int)screenBounds.y
	};
	SDL_RenderCopy(renderer, tileMap, NULL, &destination2);

	//Blit the fader overlay to the screen.
	SDL_Rect faderDestination = {
			0, 0,
			(int)screenBounds.x, (int)screenBounds.y
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

Platform makePlatform(Coord origin) {
	bool seedMap[PLATFORM_SEED_X][PLATFORM_SEED_Y];
	bool seedMap2[PLATFORM_SEED_X * PLATFORM_SCALE][PLATFORM_SEED_Y * PLATFORM_SCALE];

	//Create an initial 'seed map' to base eventual platform appearance on. This can scale based on a constant.
	// (taking care to set false cases, since these aren't automatically set).
	for(int x=0; x < PLATFORM_SEED_X; x++){
		for(int y=0; y < PLATFORM_SEED_Y; y++) {
			seedMap[x][y] = chance(PLATFORM_SEED_DENSITY);
		}
	}

	//NB: FOUND ISSUE - DON'T ITERATE ON BOTH X AND Y WITH S! JUST ONE!!!

	for(int x=0; x < PLATFORM_SEED_X * PLATFORM_SCALE; x+=PLATFORM_SCALE){
		for(int y=0; y < PLATFORM_SEED_Y * PLATFORM_SCALE; y+=PLATFORM_SCALE) {
			bool chanceResult = chance(PLATFORM_SEED_DENSITY);
			for(int s=0; s < PLATFORM_SCALE; s++) {
				seedMap[x+s][y+s] = chanceResult;
			}
		}
	}

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

	//TODO: Randomly-blended (128 alpha) tiles to give depth look.

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
//					int chance = random(1, 3);
//					if(chance == 1){
						filename = "base-large-n.png";
//					}else if(chance == 2) {
//						filename = "base-n-resistor.png";
//					}else if(chance == 3){
//						filename = "base-n-resistor-2.png";
//					}
					break;
				}case TILE_SOUTH:{
//					int chance = random(1, 3);
//					if(chance == 1){
						filename = "base-large-s.png";
//					}else if(chance == 2) {
//						filename = "base-s-resistor.png";
//					}else if(chance == 3){
//						filename = "base-s-resistor-2.png";
//					}
					break;
				}case TILE_EAST:
					filename = "base-large-e.png";
//					filename = (chance(50) ? "base-e-resistor-2.png" : "base-e.png");
					break;
				case TILE_WEST: {
//					int chance = random(1, 3);
//					if(chance == 1){
						filename = "base-large-w.png";
//					}else if(chance == 2) {
//						filename = "base-w-resistor.png";
//					}else if(chance == 3){
//						filename = "base-w-resistor-2.png";
//					}
					break;
				}
				default:
					filename = chance(90) ? "base-large-chip.png" : "base-large.png";
//					filename = chance(50) ?
//					   (chance(50) ? "base-resistor.png" : "base-resistor-2.png") :
//					   (chance(50) ? "base-chip.png" : "base.png");
					break;
			}

			SDL_Texture *baseTexture = getTexture(filename);
			baseSprite = makeSprite(baseTexture, zeroCoord(), SDL_FLIP_NONE);

			SDL_RenderCopy(renderer, baseSprite.texture, NULL, &destination);
		}
	}

	//Restore renderer context back to the window canvas.
	SDL_SetRenderTarget(renderer, NULL);

	p.sprite = makeSprite(canvas, zeroCoord(), SDL_FLIP_NONE);

	return p;
}

void backgroundGameFrame(void) {

	if(!showBackground || staticBackground) return;

	//Chunk the textures downwards on each frame, or reset if outside view.
	bgOffset = bgOffset > screenBounds.y ? 0 : bgOffset + SCROLL_SPEED;

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

	if(timer(&lastPlatformTime, toMilliseconds(nextPlatformSpawnSeconds))) {
		if(platformInc == MAX_PLATFORMS-1) platformInc = 0;

		//TODO: Algorithm to remove free-floating, or otherwise corner-hinged tiles from platform geometry.
		//TODO: Decorate borders with edge sprites.

		Coord origin = makeCoord(random(0, (int)screenBounds.x), -(PLATFORM_SEED_Y * PLATFORM_SCALE * PLATFORM_TILE_SIZE) / 2);
		Platform platform = makePlatform(origin);
		platforms[++platformInc] = platform;

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
		char filename[15];
//		sprintf(filename, "title.png", i+1);
//		sprintf(filename, "base.png", i+1);
		sprintf(filename, "space-%02d.png", i+1);
		tileMaps[i] = initBackgroundFrame(filename);
	}
}
