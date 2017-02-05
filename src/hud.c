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
bool waveCompleteOn;
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

#define MAX_FETTI 300

typedef struct {
	Coord origin;
	int frame;
	int color;
	double speed;
} Fetti;

static bool fettiFrameSkip;
static Fetti fetti[MAX_FETTI];
static long fettiTime;
static int fettiInc;

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

	// Spawn fetti, and rain down.
	if(gameState == STATE_LEVEL_COMPLETE) {
		if(due(fettiTime, 100) && fettiInc < MAX_FETTI) {
			Fetti f = {
				makeCoord(randomMq(0, screenBounds.x), 0),
				randomMq(0, 1),
				randomMq(0, 3),
				randomMq(15, 25) / 10.0
			};
			fetti[fettiInc++] = f;
		}

		for(int i=0; i < MAX_FETTI; i++) {
			fetti[i].origin.y += fetti[i].speed;
		}
	}
}

void hudAnimateFrame() {
	// Animate the 'fetti (animated only every other frame for hacky slowness).
	if(gameState == STATE_LEVEL_COMPLETE && !fettiFrameSkip) {
		for(int i=0; i < MAX_FETTI; i++) {
			fetti[i].frame = fetti[i].frame == 0 ? 1 : 0;
		}
	}
	fettiFrameSkip = !fettiFrameSkip;

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

	resetHud(false);
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

void renderWaveComplete() {
	if(!waveCompleteOn) return;
	
	Sprite complete = makeSprite(getTexture("wave-complete.png"), zeroCoord(), SDL_FLIP_NONE);
	drawSpriteAbs(complete, makeCoord(screenBounds.x/2, screenBounds.y/3));
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

void hudRenderFrameBackground() {
	// Render fetti.
	if(gameState == STATE_LEVEL_COMPLETE) {
		for(int i=0; i < MAX_FETTI; i++) {
			char* filename = NULL;
			switch(fetti[i].color) {
				case 0:
					filename = "fetti-red.png";
					break;
				case 1:
					filename = "fetti-yellow.png";
					break;
				case 2:
					filename = "fetti-green.png";
					break;
				case 3:
					filename = "fetti-blue.png";
					break;
			}
			SDL_RendererFlip flip = fetti[i].frame == 0 ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
			Sprite s = makeSprite(getTexture(filename), zeroCoord(), flip);
			double angle = chance(50) ? 0 : 90;
			drawSpriteAbsRotated(s, fetti[i].origin, angle);
		}
	}
}

void hudRenderFrame() {

	Coord underScore = makeCoord(pixelGrid.x - 7, 26);

//	showDebugStats();

	//Show score and coin HUD during the game, and game over sequences.
	if(	gameState == STATE_GAME ||
		gameState == STATE_LEVEL_COMPLETE ||
		gameState == STATE_GAME_OVER
	) {
		//Score HUD
		writeText(score, makeCoord(pixelGrid.x - 5, 10));

		//Draw coin status
		Sprite coin = makeSprite(getTexture("coin-05.png"), zeroCoord(), SDL_FLIP_NONE);
		drawSpriteAbs(coin, underScore);
		Sprite x = makeSprite(getTexture("font-x.png"), zeroCoord(), SDL_FLIP_NONE);
		drawSpriteAbs(x, deriveCoord(underScore, -9, -1));
		writeText(coins, deriveCoord(underScore, -14, -1));
	}

	renderWaveComplete();

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

void resetHud(bool keepScore) {
	if(!keepScore) {
		score = 0;
	}
	memset(fetti, 0, sizeof(fetti));
	fettiInc = 0;
	coins = 0;
	coinInserting = false;
	coinX = 0;
	coinY = 0;
	coinThrowPower = 5.25;
	coinIn = false;
}
