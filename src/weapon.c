#include <time.h>
#include "renderer.h"
#include "player.h"
#include "assets.h"
#include "enemy.h"
#include "SDL2/SDL_mixer.h"

typedef enum {
	NORTH,
	NORTH_EAST,
	EAST,
	SOUTH_EAST,
	SOUTH,
	SOUTH_WEST,
	WEST,
	NORTH_WEST,
} ShotDir;

typedef struct {
	double speed;
	Coord coord;
	int animFrame;
	ShotDir direction;
} Shot;

#define MAX_SHOTS 200

static const int SHOT_HZ = 1000 / 30;
static const double SHOT_SPEED = 4.5;
static const Coord SHOT_FIRE_OFFSET = { 1, -5 };
static const double SHOT_DAMAGE = 1.0;

static Shot shots[MAX_SHOTS];
static int shotInc = 0;
static Sprite shotSprite;
static long lastShotTime;
static int maxFrames = 2;

//Used for checking whether Shot is valid in array.
static bool invalidShot(Shot *shot) {
	return
		shot->speed == 0 ||			//was never set.
		shot->coord.y <= -8;		//is out of range.
}

static Shot nullShot(void) {
	Shot shot = { };
	return shot;
}

void pew(void) {
	//Rate limit shots.
	if(!timer(&lastShotTime, SHOT_HZ)) {
		return;
	}

	play("Laser_Shoot18.wav");

	//Ensure we stick within the bounds of our Shot array.
	if(shotInc == sizeof(shots) / sizeof(Shot)) shotInc = 0;

	//Make the Shot.
	Shot shot1 = { SHOT_SPEED, makeCoord(
			playerOrigin.x - scalePixels(SHOT_FIRE_OFFSET.x + 3),
			playerOrigin.y + scalePixels(SHOT_FIRE_OFFSET.y + 3)
		), 1,
	   NORTH_WEST
	};
	Shot shot2 = { SHOT_SPEED, makeCoord(
			playerOrigin.x - scalePixels(SHOT_FIRE_OFFSET.x - 3),
			playerOrigin.y + scalePixels(SHOT_FIRE_OFFSET.y + 3)
		), 1,
	   NORTH_EAST
	};
	Shot shot3 = { SHOT_SPEED, makeCoord(
			playerOrigin.x - scalePixels(SHOT_FIRE_OFFSET.x),
			playerOrigin.y + scalePixels(SHOT_FIRE_OFFSET.y)
		), 1,
	   NORTH
	};
	Shot shot4 = { SHOT_SPEED, makeCoord(
			playerOrigin.x - scalePixels(SHOT_FIRE_OFFSET.x + 3),
			playerOrigin.y + scalePixels(SHOT_FIRE_OFFSET.y)
		), 1,
	   NORTH
	};
	Shot shot5 = { SHOT_SPEED, makeCoord(
			playerOrigin.x - scalePixels(SHOT_FIRE_OFFSET.x + 3),
			playerOrigin.y + scalePixels(SHOT_FIRE_OFFSET.y)
		), 1,
	   SOUTH
	};
	Shot shot6 = { SHOT_SPEED, makeCoord(
			playerOrigin.x - scalePixels(SHOT_FIRE_OFFSET.x + 3),
			playerOrigin.y + scalePixels(SHOT_FIRE_OFFSET.y)
		), 1,
	   SOUTH_WEST
	};
	Shot shot7 = { SHOT_SPEED, makeCoord(
			playerOrigin.x - scalePixels(SHOT_FIRE_OFFSET.x + 3),
			playerOrigin.y + scalePixels(SHOT_FIRE_OFFSET.y)
		), 1,
	   SOUTH_EAST
	};
	Shot shot8 = { SHOT_SPEED, makeCoord(
			playerOrigin.x - scalePixels(SHOT_FIRE_OFFSET.x),
			playerOrigin.y + scalePixels(SHOT_FIRE_OFFSET.y)
	), 1,
				   EAST
	};
	Shot shot9 = { SHOT_SPEED, makeCoord(
			playerOrigin.x - scalePixels(SHOT_FIRE_OFFSET.x),
			playerOrigin.y + scalePixels(SHOT_FIRE_OFFSET.y)
	), 1,
				   WEST
	};

	shots[shotInc++] = shot1;
	shots[shotInc++] = shot2;
	shots[shotInc++] = shot3;
//	shots[shotInc++] = shot4;
	shots[shotInc++] = shot5;
	shots[shotInc++] = shot6;
	shots[shotInc++] = shot7;
	shots[shotInc++] = shot8;
	shots[shotInc++] = shot9;
}

void pewGameFrame(void) {
	for(int i=0; i < MAX_SHOTS; i++) {
		//Skip zeroed shots.
		if (invalidShot(&shots[i])) continue;

		//Toggle hit animation on enemies if within range.
		for(int p=0; p < MAX_ENEMIES; p++) {
			//Skip if the enemy is already dying.
			if(enemies[p].dying) continue;

			//If he's within our projectile bounds.
			Rect enemyBound = makeSquareBounds(enemies[p].parallax, ENEMY_BOUND);
			if(inBounds(shots[i].coord, enemyBound)) {
				hitEnemy(&enemies[p], SHOT_DAMAGE);

				//Set shot as null, and stop any further hit detection.
				shots[i] = nullShot();
				break;
			}
		}

		//If we haven't hit anything - adjust shot for velocity and heading.
		if(!invalidShot(&shots[i])){
			switch(shots[i].direction) {
				case NORTH:
					shots[i].coord = deriveCoord(shots[i].coord, 0, -shots[i].speed);
					break;
				case SOUTH:
					shots[i].coord = deriveCoord(shots[i].coord, 0, shots[i].speed);
					break;
				case EAST:
					shots[i].coord = deriveCoord(shots[i].coord, +shots[i].speed, 0);
					break;
				case WEST:
					shots[i].coord = deriveCoord(shots[i].coord, -shots[i].speed, 0);
					break;
				case NORTH_EAST:
					shots[i].coord = deriveCoord(shots[i].coord, shots[i].speed, -shots[i].speed);
					break;
				case NORTH_WEST:
					shots[i].coord = deriveCoord(shots[i].coord, -shots[i].speed, -shots[i].speed);
					break;
				case SOUTH_EAST:
					shots[i].coord = deriveCoord(shots[i].coord, shots[i].speed, shots[i].speed);
					break;
				case SOUTH_WEST:
					shots[i].coord = deriveCoord(shots[i].coord, -shots[i].speed, shots[i].speed);
					break;
 			}
		}
	}
}

void pewRenderFrame(void) {
	//We loop through the projectile array, drawing any shots that are still initialised.
	for(int i=0; i < MAX_SHOTS; i++) {
		//Skip zeroed.
		if(invalidShot(&shots[i])) continue;

		//Choose frame.
		char frameFile[50];
		sprintf(frameFile, "shot-neon-%02d.png", shots[i].animFrame);
		SDL_Texture *shotTexture = getTexture(frameFile);
		SDL_Texture *shotShadowTexture = getTextureVersion(frameFile, ASSET_SHADOW);

		//Shadow.
		Sprite shotShadow = makeSprite(shotShadowTexture, zeroCoord(), SDL_FLIP_NONE);
		Coord shadowCoord = parallax(shots[i].coord, PARALLAX_SUN, PARALLAX_LAYER_SHADOW, PARALLAX_X, PARALLAX_SUBTRACTIVE);
		shadowCoord.y += STATIC_SHADOW_OFFSET;
		drawSpriteAbs(shotShadow, shadowCoord);

		SDL_RendererFlip flipDir = SDL_FLIP_NONE;
		double angle = 0;

		// Rotate/flip the sprite based on heading.
		switch(shots[i].direction) {
			case NORTH_EAST:
				angle = 45;
				break;
			case EAST:
				angle = 90;
				break;
			case SOUTH_EAST:
				angle = 135;
				break;
			case SOUTH:
				flipDir = SDL_FLIP_VERTICAL;
				break;
			case SOUTH_WEST:
				angle = 225;
				break;
			case WEST:
				angle = 270;
				break;
			case NORTH_WEST:
				angle = 315;
				break;
		}

		//Shot itself.
		shotSprite = makeSprite(shotTexture, zeroCoord(), flipDir);
		drawSpriteAbsRotated(shotSprite, shots[i].coord, angle);
	}
}

void pewAnimateFrame(){
	for(int i=0; i < MAX_SHOTS; i++) {
		if(invalidShot(&shots[i])) continue;
		if(shots[i].animFrame == maxFrames) shots[i].animFrame = 0;
		shots[i].animFrame++;
	}
}
