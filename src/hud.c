#include <time.h>
#include "common.h"
#include "renderer.h"
#include "assets.h"
#include "player.h"
#include "hud.h"
#include "enemy.h"
#include "sound.h"
#include "myc.h"

#define NUM_HEARTS 3
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
int fruit = 0;
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

static bool warningOn;
static bool warningShowing;
static long warningStartTime;
static long lastWarningFlash;
static const int WARNING_TIME = 3000;
static const int WARNING_FLASH_TIME = 500;

static bool coinBoxFlash;
static long lastInsertCoinFlash;
static const int INSERT_COIN_FLASH_TIME = 250;
static const int INSERT_COIN_FLASH_TIME_FAST = 50;
bool coinInserting = false;
static float coinX = 0;
static int coinFrame = 1;
static const int COIN_FRAMES = 12;
static float coinY = 0;
static float coinThrowPower;
static bool coinIn = false;

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

void hudAnimateFrame() {
	// Insert coin flash
	if (gameState == STATE_COIN || gameState == STATE_TITLE || gameState == STATE_INTRO) {
		// Two flashing speeds depending on what mode we're in.
		if(timer(&lastInsertCoinFlash, INSERT_COIN_FLASH_TIME)) {
			coinBoxFlash = !coinBoxFlash;
		} else if(coinIn && timer(&lastInsertCoinFlash, INSERT_COIN_FLASH_TIME_FAST)) {
			coinBoxFlash = !coinBoxFlash;
		}
	}

	// coin rotation animation.
	if(coinInserting) {
		coinFrame = coinFrame < COIN_FRAMES-1 ? coinFrame + 1 : 1;
	}

	if(!timer(&lastBlinkTime, BATTERY_BLINK_RATE)) {
		return;
	}

	if(noneAnimInc == noneMaxAnims) noneAnimInc = 0;
	noneAnimInc++;
}

void persistentHudRenderFrame() {
	if(!(gameState == STATE_COIN || gameState == STATE_TITLE || gameState == STATE_INTRO)) return;

	// Animate the coin box.
	char coinBoxFile[50];
	sprintf(coinBoxFile,
		"insert-coin-%s%d.png",
		!coinIn ? "dim-" : "",
		coinBoxFlash ? 0 : 1
	);

	// Draw coin box.
	Sprite warning = makeSprite(getTexture(coinBoxFile), zeroCoord(), SDL_FLIP_NONE);
	drawSpriteAbs(warning, makeCoord(screenBounds.x - 20, screenBounds.y - 20));

	// Draw coin insertion animation.
	if(coinInserting) {
		if(coinX < 77) {
			// Throw the coin
			coinThrowPower -= 0.16;
			coinY -= coinThrowPower;

			char coinFile[50];
			sprintf(coinFile, "coin-%02d.png", coinFrame);
			Sprite coin = makeSprite(getTexture(coinFile), zeroCoord(), SDL_FLIP_NONE);

			drawSpriteAbsRotated(coin, makeCoord(
				screenBounds.x /2  + (coinX += 1.325),
				(screenBounds.y + 8) + coinY),
				90
			);
		}else {
			coinX = 0;
			coinInserting = false;
			coinIn = true;
			play("Powerup8.wav");
		}
	}
}

void hudInit() {
	lastInsertCoinFlash = clock();
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

	hudReset();
}

void hudReset() {
}

void writeText(int amount, Coord pos, bool doubleSize) {
	//Algorithm for digit iteration of an int (source: http://stackoverflow.com/a/8671947)
	//Note: The algorithm iterates from lowest to highest, so we print the digits in reverse to compensate.

	double scale = doubleSize ? 2 : 1;

	if(amount == 0) {
		drawSpriteAbsRotated2(letters[0], pos, 0, scale, scale);
	}else{
		while(amount != 0) {
			drawSpriteAbsRotated2(letters[amount % 10], pos, 0, scale, scale);
			amount /= 10;
			pos.x -= (LETTER_WIDTH * scale);
		}
	}
}

void showDebugStats() {
	Coord underLife = makeCoord(10, 27);

	//Show player X coordinate:
	writeText(playerOrigin.x, deriveCoord(underLife, 10, 20), false);

	if(enemies[0].health > 0) {
		writeText(enemies[0].origin.x, deriveCoord(underLife, 10, 30), false);
		writeText(enemies[0].parallax.x, deriveCoord(underLife, 10, 40), false);
	}
}

void toggleWarning() {
	warningOn = true;
	warningStartTime = clock();
	lastWarningFlash = clock();

	// Stop the music (drama!)
	Mix_FadeOutMusic(250);

	// Force initial display
	play("warning.wav");
	warningShowing = true;
}

void renderWarning() {
	if(!warningOn) return;

	// Halt the warning, and start special boss music.
	if(warningOn && due(warningStartTime, WARNING_TIME)) {
		playMusic("tension.ogg", -1);
		warningOn = false;
		return;
	}

	// Toggle message on/off, and play sound.
	if(timer(&lastWarningFlash, WARNING_FLASH_TIME)) {
		warningShowing = !warningShowing;
		if(warningShowing) play("warning.wav");
	}

	// Render the message
	if(warningShowing) {
		Sprite warning = makeSprite(getTexture("warning.png"), zeroCoord(), SDL_FLIP_NONE);
		drawSpriteAbs(warning, makeCoord(pixelGrid.x/2, pixelGrid.y/3));
	}
}

void insertCoin() {
	coinInserting = true;
	Mix_PauseMusic();
	play("Powerup9.wav");
//	play("Pickup_Coin34b.wav");
}

int coinInc = 0;
int fruitInc = 0;
int scoreInc = 0;
int statsInc = 0;

void hudRenderFrame() {
	Coord underScore = makeCoord(pixelGrid.x - 7, 26);

//	showDebugStats();

	if(gameState == STATE_STATS) {
		drawSpriteAbsRotated2(makeSimpleSprite("text-coins.png"), makeCoord(120, 50), 0, 1, 1);
		drawSpriteAbsRotated2(makeSimpleSprite("font-x.png"), makeCoord(100, 50), 0, 1, 1);
		writeText(coinInc, makeCoord(93, 50), false);

		if(coinInc == coins) {
			statsInc++;
		}else{
			coinInc++;
		}

		if(statsInc >= 1) {
			drawSpriteAbsRotated2(makeSimpleSprite("text-treats.png"), makeCoord(123, 60), 0, 1, 1);
			drawSpriteAbsRotated2(makeSimpleSprite("font-x.png"), makeCoord(100, 60), 0, 1, 1);
			writeText(fruitInc, makeCoord(93, 60), false);

			if(fruitInc == fruit) {
				statsInc++;
			}else{
				fruitInc++;
			}
		}
		if(statsInc >= 1) {
			drawSpriteAbsRotated2(makeSimpleSprite("text-score.png"), makeCoord(121, 75), 0, 1, 1);
			writeText(scoreInc, makeCoord(93, 75), false);

			if(scoreInc == score) {
				statsInc++;
			}else{
				scoreInc++;
			}
		}

		return;
	}

	//Show score and coin HUD during the game, and game over sequences.
	if(	gameState == STATE_GAME ||
		gameState == STATE_LEVEL_COMPLETE ||
		gameState == STATE_GAME_OVER
	) {
		//Score HUD
		writeText(score, makeCoord(pixelGrid.x - 5, 10), false);

		//Draw coin status
		Sprite coin = makeSprite(getTexture("coin-05.png"), zeroCoord(), SDL_FLIP_NONE);
		drawSpriteAbs(coin, underScore);
		Sprite x = makeSprite(getTexture("font-x.png"), zeroCoord(), SDL_FLIP_NONE);
		drawSpriteAbs(x, deriveCoord(underScore, -9, -1));
		writeText(coins, deriveCoord(underScore, -14, -1), false);
	}

	//Only show if playing, and *hide* if dying.
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
			char noneName[50];
			sprintf(noneName, "battery-low-%02d.png", noneAnimInc);
			Sprite lifeNone = makeSprite(getTexture(noneName), zeroCoord(), SDL_FLIP_NONE);

			drawSpriteAbs(lifeNone, lifePositions[bar]);
		}
	}

	//Plumes
	for(int i=0; i < MAX_PLUMES; i++) {
		if(isNullPlume(&plumes[i])) continue;

		switch(plumes[i].type) {
			case PLUME_SCORE: {
				writeText(plumes[i].score, plumes[i].parallax, false);
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

	renderWarning();

	// Boss health bar.
	if(bossOnscreen) {
		const double BAR_LENGTH = 60;
		Sprite bossBarBg = makeSprite(getTexture("health-bar-bg.png"), zeroCoord(), SDL_FLIP_NONE);
		drawSpriteAbsRotated2(bossBarBg, makeCoord(50, 9), 0, BAR_LENGTH*2, 1);
		Sprite bossBar = makeSprite(getTexture("health-bar.png"), zeroCoord(), SDL_FLIP_NONE);
		drawSpriteAbsRotated2(bossBar, makeCoord(50, 9), 0, (bossHealth / 100) * BAR_LENGTH, 1);
		Sprite bossName = makeSprite(getTexture("text-keyface.png"), zeroCoord(), SDL_FLIP_NONE);
		drawSprite(bossName, makeCoord(112, 9));
	}
}

void resetHud() {
	score = 0;
	coins = 0;
	fruit = 0;
	coinInserting = false;
	coinX = 0;
	coinY = 0;
	coinThrowPower = 5.25;
	coinIn = false;
}
