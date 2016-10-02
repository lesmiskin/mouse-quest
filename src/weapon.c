#include <time.h>
#include "renderer.h"
#include "player.h"
#include "assets.h"
#include "enemy.h"
#include "mysdl.h"
#include "weapon.h"
#include "input.h"
#include "sound.h"
#include "hud.h"
#include "myc.h"

#define MAX_SHOTS 50

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
	double angle;
} Shot;

typedef enum {
	SPEED_NORMAL = 1000 / 12,
	SPEED_FAST = 1000 / 14,
} WeaponSpeed;

typedef enum {
	PATTERN_MINI = 0,
	PATTERN_MINI_2 = 1,
	PATTERN_SINGLE = 2,
	PATTERN_DUAL = 3,
	PATTERN_TRIAD = 4,
	PATTERN_FAN = 5,
} WeaponPattern;

typedef struct {
	WeaponSpeed speed;
	WeaponPattern pattern;
} Weapon;

bool canFireInLevel = false;
int weaponInc = 0;
static Weapon weapons[MAX_WEAPONS];
//static const int SHOT_HZ = 1000 / 11 ;
static const double SHOT_SPEED = 7;
static const double SHOT_DAMAGE = 0.7;
static Shot shots[MAX_SHOTS];
static int shotInc = 0;
static Sprite shotSprite;
static long lastShotTime;
static int maxFrames = 2;

static short minigunLastSide = 0;

//Used for checking whether Shot is valid in array.
static bool invalidShot(Shot *shot) {
	return
			shot->speed == 0 ||				//was never set (i.e. when all are NULL at start of game)
			!inScreenBounds(shot->coord);	//if out of range in any screen boundary (important for diag and fan patterns).
}

static Shot nullShot() {
	Shot shot = { };
	return shot;
}

static void spawnPew(int xOffset, int yOffset, ShotDir direction) {
	//Set angle based on direction
	double angle = 0;
	switch(direction) {
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
			angle = 180;
			break;
		case SOUTH_WEST:
			angle = 215;
			break;
		case WEST:
			angle = 270;
			break;
		case NORTH_WEST:
			angle = 315;
			break;
	}

	//Make the shot.
	Shot shot = {
		SHOT_SPEED,
		deriveCoord(playerOrigin, xOffset, yOffset),
		1,
		direction,
		angle
	};

    //Ensure we stick within the bounds of our Shot array.
    shotInc+1 > MAX_SHOTS ? shotInc = 0 : shotInc++;

    //Add to the shot list.
	shots[shotInc] = shot;
}

bool atMaxWeapon() {
	return weaponInc + 1 == MAX_WEAPONS;
}

void changeWeapon(int newWeapon) {
	//This is the 'official' place to change the weapon, since we trigger things like the HUD sweep here.

	//Don't change if the same.
	if(newWeapon == weaponInc) return;

	weaponInc = newWeapon;
}

void upgradeWeapon() {
	if(atMaxWeapon()) return;
	changeWeapon(weaponInc+1);
}

void pew() {
	//Rate-limiter, and preventing initial shots post-menu.
	if(!timer(&lastShotTime, weapons[weaponInc].speed))
		return;
	if(!canFireInLevel && gameState != STATE_INTRO)
		return;

//	SDL_HapticRumblePlay(haptic, 0.4, 100);
	play("Laser_Shoot18.wav");

	//The different shot patterns, based on our current weapon.
	switch(weapons[weaponInc].pattern) {
		case PATTERN_SINGLE:
			spawnPew(-1, -5, NORTH);
			break;
		case PATTERN_DUAL:
			spawnPew(2, -5, NORTH);
			spawnPew(-4, -5, NORTH);
			break;
		case PATTERN_TRIAD:
			spawnPew(-1, -5, NORTH);
			spawnPew(-3, -2, NORTH_EAST);
			spawnPew(1, -2, NORTH_WEST);
			break;
		case PATTERN_MINI:
			if(minigunLastSide == 0) {
				spawnPew(-2, -5, NORTH);
				minigunLastSide = 1;
			}else{
				spawnPew(2, -5, NORTH);
				minigunLastSide = 0;
			}
			break;
		case PATTERN_MINI_2:
			switch(minigunLastSide) {
				case 0:
					spawnPew(-6, -5, NORTH);
					minigunLastSide = 1;
					break;
				case 1:
					spawnPew(0, -5, NORTH);
					minigunLastSide = 2;
					break;
				case 2:
					spawnPew(6, -5, NORTH);
					minigunLastSide = 3;
					break;
				case 3:
					spawnPew(0, -5, NORTH);
					minigunLastSide = 0;
					break;
			}
			break;
		case PATTERN_FAN:
            spawnPew(-1, -5, NORTH);
            spawnPew(-1, -2, NORTH_EAST);
            spawnPew(-1, 0, EAST);
            spawnPew(-1, 2, SOUTH_EAST);
            spawnPew(-1, 2, SOUTH);
            spawnPew(1, 2, SOUTH_WEST);
            spawnPew(5, 0, WEST);
            spawnPew(1, -2, NORTH_WEST);
			break;
	}
}

void pewGameFrame() {
	for(int i=0; i < MAX_SHOTS; i++) {
		//Skip zeroed shots.
		if (invalidShot(&shots[i])) continue;

		//Toggle hit animation on enemies if within range.
		for(int p=0; p < MAX_ENEMIES; p++) {
			//Skip if the enemy is already dying.
            if(invalidEnemy(&enemies[p]) || enemies[p].dying) continue;

			//If he's within our projectile bounds.
			Rect enemyBound;
			if(enemies[p].type == ENEMY_BOSS) {  //HACK!
				enemyBound = makeBounds(enemies[p].parallax, 72, 32);
			}else{
				enemyBound = makeSquareBounds(enemies[p].parallax, ENEMY_BOUND);
			}

			if(inBounds(shots[i].coord, enemyBound)) {
				hitEnemy(&enemies[p], SHOT_DAMAGE, false);

				//Set shot as null, and cancel out of this loop (since this shot is now finished with).
				shots[i] = nullShot();
				break;
			}
		}

		//If we left the above loop early (e.g. hit something) continue onto the next shot.
		if(invalidShot(&shots[i])) {
			continue;
		}

		//If we haven't hit anything - adjust shot for velocity and heading.
		switch(shots[i].direction) {
			case NORTH:
				shots[i].coord = deriveCoord(shots[i].coord, 0, -shots[i].speed);
				break;
			case SOUTH:
				shots[i].coord = deriveCoord(shots[i].coord, 0, shots[i].speed);
				shots[i].angle = 180;
				break;
			case EAST:
				shots[i].coord = deriveCoord(shots[i].coord, +shots[i].speed, 0);
				shots[i].angle = 90;
				break;
			case WEST:
				shots[i].coord = deriveCoord(shots[i].coord, -shots[i].speed, 0);
				shots[i].angle = 270;
				break;
			case NORTH_EAST:
				shots[i].coord = deriveCoord(shots[i].coord, shots[i].speed, -shots[i].speed);
				shots[i].angle = 45;
				break;
			case NORTH_WEST:
				shots[i].coord = deriveCoord(shots[i].coord, -shots[i].speed, -shots[i].speed);
				shots[i].angle = 315;
				break;
			case SOUTH_EAST:
				shots[i].angle = 135;
				shots[i].coord = deriveCoord(shots[i].coord, shots[i].speed, shots[i].speed);
				break;
			case SOUTH_WEST:
				shots[i].angle = 225;
				shots[i].coord = deriveCoord(shots[i].coord, -shots[i].speed, shots[i].speed);
				break;
		}
	}
}

void pewShadowFrame() {
	//Draw the shadows first (so we don't shadow on top of other shots)
	for(int i=0; i < MAX_SHOTS; i++) {
		//Skip zeroed.
		if (invalidShot(&shots[i])) continue;

		//Choose frame.
		char frameFile[20];
		sprintf(frameFile, "shot-neon-%02d.png", shots[i].animFrame);
		SDL_Texture *shotShadowTexture = getTextureVersion(frameFile, ASSET_SHADOW);

		//Shadow.
		Sprite shotShadow = makeSprite(shotShadowTexture, zeroCoord(), SDL_FLIP_NONE);
		Coord shadowCoord = parallax(shots[i].coord, PARALLAX_SUN, PARALLAX_LAYER_SHADOW, PARALLAX_X, PARALLAX_SUBTRACTIVE);
		shadowCoord.y += STATIC_SHADOW_OFFSET;
		drawSpriteAbsRotated(shotShadow, shadowCoord, shots[i].angle);
	}
}

void pewRenderFrame() {
	//We loop through the projectile array, drawing any shots that are still initialised.
	for(int i=0; i < MAX_SHOTS; i++) {
		//Skip zeroed.
		if(invalidShot(&shots[i])) continue;

		//Choose frame.
		char frameFile[20];
		sprintf(frameFile, "shot-neon-%02d.png", shots[i].animFrame);
		SDL_Texture *shotTexture = getTexture(frameFile);

		//Shot itself.
		shotSprite = makeSprite(shotTexture, zeroCoord(), SDL_FLIP_NONE);
		drawSpriteAbsRotated(shotSprite, shots[i].coord, shots[i].angle);
	}
}

void pewAnimateFrame(){
	for(int i=0; i < MAX_SHOTS; i++) {
		if(invalidShot(&shots[i])) continue;
		if(shots[i].animFrame == maxFrames) shots[i].animFrame = 0;
		shots[i].animFrame++;
	}
}

void pewInit() {
	Weapon w1 = { SPEED_NORMAL, PATTERN_DUAL };
//	Weapon w1 = { SPEED_FAST, PATTERN_MINI };
	Weapon w2 = { SPEED_FAST, PATTERN_DUAL };
	Weapon w3 = { SPEED_FAST, PATTERN_TRIAD };
	Weapon w4 = { SPEED_FAST, PATTERN_FAN };

	weapons[0] = w1;
	weapons[1] = w2;
	weapons[2] = w3;
	weapons[3] = w4;

	resetPew();
}

void resetPew() {
	weaponInc = 0;
	shotInc = 0;
	canFireInLevel = false;
}