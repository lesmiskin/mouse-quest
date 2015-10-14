#include <time.h>
#include "common.h"
#include "renderer.h"
#include "assets.h"
#include "player.h"
#include "hud.h"

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
static ScorePlume plumes[MAX_PLUMES];
static int plumeInc = 0;
static Sprite life, lifeHalf/*, lifeNone*/;
static Coord lifePositions[NUM_HEARTS];
static int noneAnimInc = 1;
static int noneMaxAnims = 2;
static const int BATTERY_BLINK_RATE = 500;
static long lastBlinkTime;

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

void raiseScore(unsigned amount) {
	score += amount;
	spawnScorePlume(PLUME_SCORE, amount);
}

ScorePlume nullPlume() {
	ScorePlume p = { };
	return p;
}

bool isNullPlume(ScorePlume *plume) {
	return plume->spawnTime == 0;
}

void hudGameFrame(void) {
	for(int i=0; i < MAX_PLUMES; i++) {
		if(isNullPlume(&plumes[i])) continue;

		//Hide once their display time has expired.
		if(due(plumes[i].spawnTime, 750)) {
			plumes[i] = nullPlume();
		}

		plumes[i].parallax.y -= 0.75;
//		plumes[i].parallax = parallax(plumes[i].origin, PARALLAX_PAN, PARALLAX_LAYER_FOREGROUND, PARALLAX_X, PARALLAX_ADDITIVE);
	}
}

void hudAnimateFrame(void) {
	if(!timer(&lastBlinkTime, BATTERY_BLINK_RATE)) {
		return;
	}

	if(noneAnimInc == noneMaxAnims) noneAnimInc = 0;

	noneAnimInc++;
}

void hudInit(void) {
	life = makeSprite(getTexture("battery.png"), zeroCoord(), SDL_FLIP_NONE);
	lifeHalf = makeSprite(getTexture("battery-half.png"), zeroCoord(), SDL_FLIP_NONE);
//	lifeNone = makeSprite(getTexture("battery-none.png"), zeroCoord(), SDL_FLIP_NONE);

	//Set drawing coordinates for heart icons - each is spaced out in the upper-left.
	for(int i=0; i < NUM_HEARTS; i++){
		lifePositions[i] = makeCoord(10 + (i * 12 ), 10);
	}
}

void hudRenderFrame(void) {
	//Only show if playing.
	if(gameState != STATE_GAME) return;

	//Loop through icons and draw them at the appropriate 'fullness' levels for each health bar.
	// This algorithm will automatically scale according to whatever we choose to set the player's total health to.
	float healthPerHeart = playerStrength / NUM_HEARTS;		//e.g. for 4 hearts, 1 heart = 25 hitpoints.
	for(int bar=0; bar < NUM_HEARTS; bar++) {
		//The total health represented by this bar. We do a bit of crazy magic here to make sure we calculate this
		// correctly for a left-to-right pass.
		float barHealth = playerStrength - ((NUM_HEARTS - (bar+1)) * healthPerHeart);

		//Full bar.
		if(playerHealth >= barHealth) {
			drawSpriteAbs(life, lifePositions[bar]);
		//Between half and full.
		//}else if(playerHealth >= barHealth - (healthPerHeart/2)) {
		}else{
			char noneName[50];
			sprintf(noneName, "battery-low-%02d.png", noneAnimInc);
			Sprite lifeNone = makeSprite(getTexture(noneName), zeroCoord(), SDL_FLIP_NONE);

			drawSpriteAbs(lifeNone, lifePositions[bar]);
		}
	}

	for(int i=0; i < MAX_PLUMES; i++) {
		if(isNullPlume(&plumes[i])) continue;

		switch(plumes[i].type) {
			case PLUME_SCORE: {
				Sprite score_0 = makeSprite(getTexture("font-00.png"), zeroCoord(), SDL_FLIP_NONE);
				Sprite score_1 = makeSprite(getTexture("font-01.png"), zeroCoord(), SDL_FLIP_NONE);
				Sprite score_5 = makeSprite(getTexture("font-05.png"), zeroCoord(), SDL_FLIP_NONE);

				if(plumes[i].score == 100) {
					drawSpriteAbs(score_1, deriveCoord(plumes[i].parallax, -4, 0));
				}else if(plumes[i].score == 500) {
					drawSpriteAbs(score_5, deriveCoord(plumes[i].parallax, -4, 0));
				}

				drawSpriteAbs(score_0, deriveCoord(plumes[i].parallax, 0, 0));
				drawSpriteAbs(score_0, deriveCoord(plumes[i].parallax, 4, 0));

				break;
			}
			case PLUME_LASER: {
				Sprite plume = makeSprite(getTexture("text-laser-upgraded.png"), zeroCoord(), SDL_FLIP_NONE);
				drawSpriteAbs(plume, plumes[i].parallax);
				break;
			}
			case PLUME_POWER: {
				Sprite plume = makeSprite(getTexture("text-full-power.png"), zeroCoord(), SDL_FLIP_NONE);
				drawSpriteAbs(plume, plumes[i].parallax);
				break;
			}
		}
	}
}
