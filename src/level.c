#include <stdbool.h>
#include <time.h>
#include <mem.h>
#include "common.h"
#include "enemy.h"
#include "hud.h"
#include "item.h"

typedef enum {
	W_COL,
	W_WARNING,
	W_LEVEL_CHANGE,
} WaveType;

typedef struct {
	bool Pause;
	bool PauseFinished;
	int SpawnTime;
	WaveType WaveType;
	int x;
	int y;
	EnemyPattern Movement;
	EnemyType Type;
	EnemyCombat Combat;
	bool Async;
	double Speed;
	double SpeedX;
	double Health;
	int Qty;
} WaveTrigger;

#define MAX_WAVES 1000
WaveTrigger triggers[MAX_WAVES];
long gameTime = 0;
int waveInc = 0;
int waveAddInc = 0;
const int NA = -50;
const int ENEMY_SPACE = 35;
long levelTime = 0;
int level = 0;

bool invalidWave(WaveTrigger *wave) {
	return wave->Health == 0 && !wave->Pause && !wave->WaveType == W_WARNING;
}

void levelChange() {
	WaveTrigger e = {
		false, false, 0, W_LEVEL_CHANGE, 0, 0, PATTERN_NONE, ENEMY_CD, COMBAT_IDLE, false, 0, 0, 0, 0
	};
	
	triggers[waveAddInc++] = e;
}

void warning() {
	WaveTrigger e = {
		false, false, 0, W_WARNING, 0, 0, PATTERN_NONE, ENEMY_CD, COMBAT_IDLE, false, 0, 0, 0, 0
	};

	triggers[waveAddInc++] = e;
}

void pause(int spawnTime) {
	WaveTrigger e = {
		true, false, spawnTime, W_COL, 0, 0, PATTERN_BOB, ENEMY_CD, COMBAT_IDLE, false, 0, 0, 0, 0
	};

	triggers[waveAddInc++] = e;
}

void wave(int spawnTime, WaveType waveType, int x, int y, EnemyPattern movement, EnemyType type, EnemyCombat combat, bool async, double speed, double speedX, double health, int qty) {
	WaveTrigger e = {
		false, false, spawnTime, waveType, x, y, movement, type, combat, async, speed, speedX, health, qty
	};

	triggers[waveAddInc++] = e;
}

void w_column(int x, int y, EnemyPattern movement, EnemyType type, EnemyCombat combat, bool async, double speed, double speedX, int qty, double health) {
	int startY = y > NA ? y : NA; //respect Y if given, otherwise normal offscreen pos.
	int sineInc = 0;

	//Ensure "swayiness" is distributed throughout total quantity.
	double sineIncInc = qty / 3.14;

	for(int i=0; i < qty; i++) {
		spawnEnemy(x, startY, type, movement, combat, speed, speedX, sineInc, health);
		startY -= 35;
		if(async) sineInc += sineIncInc;
	}
}

//Wires up wave spawners with their triggers.
void levelGameFrame() {
	if(gameState != STATE_GAME) return;

	WaveTrigger trigger = triggers[waveInc];

	if(waveInc < MAX_WAVES && !invalidWave(&trigger) && due(gameTime, trigger.SpawnTime) ) {
		if(trigger.Pause) {
			if (due(gameTime, trigger.SpawnTime)) {
				gameTime = clock();
				waveInc++;
			}
		}else if(trigger.WaveType == W_WARNING) {
			toggleWarning();
			waveInc++;
		}else if(trigger.WaveType == W_LEVEL_CHANGE) {
			
			// Wait until there are no enemies left, before ending.
			if(noEnemiesLeft() && noItemsLeft()) {
				triggerState(STATE_LEVEL_COMPLETE);
				waveInc++;
			}
		}
		else{
			w_column(trigger.x, trigger.y, trigger.Movement, trigger.Type, trigger.Combat, trigger.Async, trigger.Speed, trigger.SpeedX, trigger.Qty, trigger.Health);
			waveInc++;
		}
	}
}

void levelInit() {
	const int CENTER = 135;
	int wavesPerLevel = 2;
	
	// LEVEL X message.
	// Fruit spawns on conclusion of wave destruction (four or more).
	// Variables tweak up with each level.
	// When hurt, your powerup should drop off at 50% transparency, with a chance to pickup.
	
	// BUG: Mike is invisible sometimes when ending (e.g. when in pain).
	// BUG: Enemy shots stay onscreen during ending.

	// Restore shot icon for current weapon.
	// Reward icon flashes when powerup is due.
	// Level one: singles, columns, swirlers.
	// Level two: singles, columns, swirlers, snakes.
	// Level three: singles, columns, swirlers, snakes, peelers.

	for(int i=0; i < wavesPerLevel; i++) {
		// Position across the screen.
		int xPos = randomMq(16, (int)screenBounds.x - 16);
		EnemyType type = (EnemyType)randomMq(0, ENEMY_TYPES);
		EnemyCombat shoots = chance(5) ? COMBAT_HOMING : COMBAT_IDLE;
		int qty = randomMq(1, 4);
		int delay = qty == 1 ? 1000 : 2000;
		double speed = randomMq(100, 150) / 100.0;

		// Fast column of enemies crop up occasionally.
		double fast = 2.4;
		double useSpeed = chance(5) ? fast : speed;
		if(useSpeed == fast) {
			qty = 4;
			shoots = COMBAT_IDLE;
		}

		// wave(int spawnTime, WaveType waveType, int x, int y, EnemyPattern movement,
		// 		EnemyType type, EnemyCombat combat, bool async, double speed, double speedX,
		// 		double health, int qty) {

		wave(0, W_COL, xPos, NA, PATTERN_NONE, type, shoots, false, useSpeed, 1, HEALTH_LIGHT, qty);

		// Boss trigger.
		if(i == wavesPerLevel-1) {
			
			// Last level = spawn boss.
			if(level == 2) {
				pause(6000);
				warning();
				wave(0, W_COL, CENTER, 310, PATTERN_BOSS_INTRO, ENEMY_BOSS_INTRO, COMBAT_IDLE, false, 2, 0, 200, 1);
				pause(3500);
				wave(0, W_COL, CENTER, NA, PATTERN_BOSS, ENEMY_BOSS, COMBAT_HOMING, false, 0.5, 1, 200, 1);

			// Otherwise, change level.
			}else {
				levelChange();
			}
			
		}else{
			pause(delay);
		}
	}
}

void resetLevel() {
	gameTime = clock();
	waveInc = 0;
	waveAddInc = 0;
	levelTime = clock();
}
