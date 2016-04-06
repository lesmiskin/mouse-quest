#include <time.h>
#include "common.h"
#include "renderer.h"
#include "assets.h"
#include "player.h"
#include "hud.h"
#include "weapon.h"
#include "enemy.h"

#define NUM_HEARTS 4
#define MAX_PLUMES 10

typedef struct {
	PlumeType type;
	int score;
	Coord origin;
	Coord parallax;
	long spawnTime;
} ScorePlume;

int score;
int topScore;
int coins = 0;
bool weaponChanging = false;
static ScorePlume plumes[MAX_PLUMES];
static int plumeInc = 0;
static Sprite life, lifeHalf/*, lifeNone*/;
static Coord lifePositions[NUM_HEARTS];
static int noneAnimInc = 1;
static int noneMaxAnims = 2;
static const int BATTERY_BLINK_RATE = 500;
static long lastBlinkTime;
static Sprite letters[10];
static const int LETTER_WIDTH = 4;
static double weapSweepInc = 0;

void spawnScorePlume(PlumeType type, int score) {
	if(plumeInc+1 == MAX_PLUMES) plumeInc = 0;

	ScorePlume plume = {
		type,
		score,
		deriveCoord(playerOrigin, 0, -10),
		deriveCoord(playerOrigin, 0, -10),
		clock()
	};

	plumes[plumeInc++] = plume;
}

void spawnPlume(PlumeType type) {
	spawnScorePlume(type, 0);
}

void raiseScore(unsigned amount, bool plume) {
	score += amount;
	if(plume) {
		spawnScorePlume(PLUME_SCORE, amount);
	}
}

ScorePlume nullPlume() {
	ScorePlume p = { };
	return p;
}

bool isNullPlume(ScorePlume *plume) {
	return plume->spawnTime == 0;
}

void hudGameFrame() {
	for(int i=0; i < MAX_PLUMES; i++) {
		if(isNullPlume(&plumes[i])) continue;

		//Hide once their display time has expired.
		if(due(plumes[i].spawnTime, 750)) {
			plumes[i] = nullPlume();
		}

		plumes[i].parallax.y -= 0.75;
	}
}

void resetHud() {
	score = 0;
	coins = 0;
}

void hudAnimateFrame() {
	if(!timer(&lastBlinkTime, BATTERY_BLINK_RATE)) {
		return;
	}

	if(noneAnimInc == noneMaxAnims) noneAnimInc = 0;

	noneAnimInc++;
}

void hudInit() {
	life = makeSprite(getTexture("battery.png"), zeroCoord(), SDL_FLIP_NONE);
	lifeHalf = makeSprite(getTexture("battery-half.png"), zeroCoord(), SDL_FLIP_NONE);
//	lifeNone = makeSprite(getTexture("battery-none.png"), zeroCoord(), SDL_FLIP_NONE);

	//Set drawing coordinates for heart icons - each is spaced out in the upper-left.
	for(int i=0; i < NUM_HEARTS; i++){
		lifePositions[i] = makeCoord(10 + (i * 12 ), 10);
	}

	//Pre-load font sprites.
	for(int i=0; i < 10; i++) {
		char textureName[50];
		sprintf(textureName, "font-%02d.png", i);
		letters[i] = makeSimpleSprite(textureName);
	}
}

void writeText(int amount, Coord pos) {
	//Algorithm for digit iteration of an int (source: http://stackoverflow.com/a/8671947)
	//Note: The algorithm iterates from lowest to highest, so we print the digits in reverse to compensate.

	if(amount == 0) {
		drawSpriteAbs(letters[0], pos);
	}else{
		while(amount != 0) {
			drawSpriteAbs(letters[amount % 10], pos);
			amount /= 10;
			pos.x -= LETTER_WIDTH;
		}
	}
}

void showDebugStats() {
	Coord underLife = makeCoord(10, 27);

	//Show player X coordinate:
	writeText(playerOrigin.x, deriveCoord(underLife, 10, 20));

	if(enemies[0].health > 0) {
		writeText(enemies[0].origin.x, deriveCoord(underLife, 10, 30));
		writeText(enemies[0].parallax.x, deriveCoord(underLife, 10, 40));
	}
}

//FIXME: Hack... :p
Coord underLife = { 10, 27 };
Coord sweepPos = { 10, 27 };
bool sweeping = false;
bool sweepDir = false;

void hudRenderFrame() {
	Coord underScore = makeCoord(pixelGrid.x - 7, 26);

//	showDebugStats();

	//Show score and coin HUD during the game, and game over sequences.
	if(gameState == STATE_GAME || gameState == STATE_GAME_OVER) {
		//Score HUD
		writeText(score, makeCoord(pixelGrid.x - 5, 10));

		//Draw coin status
		Sprite coin = makeSprite(getTexture("coin-05.png"), zeroCoord(), SDL_FLIP_NONE);
		drawSpriteAbs(coin, underScore);
		Sprite x = makeSprite(getTexture("font-x.png"), zeroCoord(), SDL_FLIP_NONE);
		drawSpriteAbs(x, deriveCoord(underScore, -9, -1));
		writeText(coins, deriveCoord(underScore, -14, -1));
	}

	//Only show if playing, and *hide* if dying.
	if(gameState != STATE_GAME) return;

	//Animate the weapon change.
	if(weaponChanging) {
		if(!sweeping || sweepPos.x < underLife.x) {
			sweepPos.x = sineInc(sweepPos.x, &weapSweepInc, 0.2, 2.3);
			if(!sweeping) sweeping = true;
			if(sweepPos.x < -10) sweepDir = true;		//toggle direction when offscreen
		} else {
			sweepPos.x = underLife.x;
			weaponChanging = false;
			sweeping = false;
			weapSweepInc = 0;
			sweepDir = false;
		}
	}

	//Draw sprite of current/changing weapon.
	char* weaponTexture = NULL;
	int weaponToDraw = weaponChanging && !sweepDir ? lastWeapon : weaponInc;
	switch(weaponToDraw) {
		case 0:
			weaponTexture = "hud-powerup.png";
			break;
		case 1:
			weaponTexture = "hud-powerup-double.png";
			break;
		case 2:
			weaponTexture = "hud-powerup-triple.png";
			break;
		case 3:
			weaponTexture = "hud-powerup-fan.png";
			break;
	}

	//Draw the weapon sprite.
	Sprite weapon = makeSprite(getTexture(weaponTexture), zeroCoord(), SDL_FLIP_NONE);
	drawSpriteAbs(weapon, sweepPos);

	//Loop through icons and draw them at the appropriate 'fullness' levels for each health bar.
	// This algorithm will automatically scale according to whatever we choose to set the player's total health to.
	float healthPerHeart = playerStrength / NUM_HEARTS;		//e.g. for 4 hearts, 1 heart = 25 hitpoints.
	for(int bar=0; bar < NUM_HEARTS; bar++) {
		//The total health represented by this bar. We do a bit of crazy magic here to make sure we calculate this
		// correctly for a left-to-right pass.
		float barHealth = playerStrength - ((NUM_HEARTS - (bar+1)) * healthPerHeart);

		//Full bar.
		if(playerHealth >= barHealth) {
			AssetVersion version = godMode ? ASSET_SUPER : ASSET_DEFAULT;
			Sprite lifeGod = makeSprite(getTextureVersion("battery.png", version), zeroCoord(), SDL_FLIP_NONE);

			drawSpriteAbs(lifeGod, lifePositions[bar]);
		//Between half and full.
		}else if(playerHealth >= barHealth - (healthPerHeart/2)) {
			char noneName[50];
			sprintf(noneName, "battery-low-%02d.png", noneAnimInc);
			Sprite lifeNone = makeSprite(getTexture(noneName), zeroCoord(), SDL_FLIP_NONE);

			drawSpriteAbs(lifeNone, lifePositions[bar]);
		}else{
//			char noneName[50];
//			sprintf(noneName, "battery-low-%02d.png", noneAnimInc);
//			Sprite lifeNone = makeSprite(getTexture(noneName), zeroCoord(), SDL_FLIP_NONE);
//
//			drawSpriteAbs(lifeNone, lifePositions[bar]);
		}
	}

	//Plumes
	for(int i=0; i < MAX_PLUMES; i++) {
		if(isNullPlume(&plumes[i])) continue;

		switch(plumes[i].type) {
			case PLUME_SCORE: {
				writeText(plumes[i].score, plumes[i].parallax);
				break;
			}
			case PLUME_LASER: {
				Sprite plume = makeSimpleSprite("text-laser-upgraded.png");
				drawSpriteAbs(plume, plumes[i].parallax);
				break;
			}
			case PLUME_POWER: {
				Sprite plume = makeSimpleSprite("text-full-power.png");
				drawSpriteAbs(plume, plumes[i].parallax);
				break;
			}
		}
	}
}
