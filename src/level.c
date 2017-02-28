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

	// Set the swaying property if our pattern dictates this.
	// NB: Ensure "swayiness" is distributed throughout total quantity (done through += in the inner loop)
	double sineIncInc = 0;
	if(movement == PATTERN_SNAKE) {
		sineIncInc = qty / 3.14;
	}

	for(int i=0; i < qty; i++) {
		spawnEnemy(x, startY, type, movement, combat, speed, speedX, sineInc, health);
		startY -= 35;
		/*if(async) */sineInc += sineIncInc;
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

typedef struct {
	int waves;
	Pair density;
	bool useBoss;
	int shootChance;
	int singleChance;
	int columnChance;
	int snakeChance;
	Pair columnRange;
	Pair snakeRange;
} Level;

#define MAX_LEVELS 5

Level levels[MAX_LEVELS];

void loadLevels() {
	Level level1 = {
		30,
		makePair(1000, 2000),
		false,
		0,
		0,
		0,
		100,
		makePair(2, 3),
		makePair(3, 6)
	};
	levels[0] = level1;
	
	Level level2 = {
		50,
		makePair(1000, 1500),
		false,
		0,
		40,
		40,
		20,
		makePair(3, 6),
		makePair(4, 5)
	};
	levels[1] = level2;
	
	Level level3 = {
		75,
		makePair(750, 1250),
		true,
		0,
		33,
		33,
		33,
		makePair(4, 7),
		makePair(6, 8)
	};
	levels[2] = level3;
}

void levelInit() {
	const int CENTER = 135;

	loadLevels();

	// Fruit spawns on conclusion of wave destruction (four or more).
	// Wave complete message on boss death.
	// Reintroduce boss white-out on death.
	// Swap confetti for coins on boss death.

	Level thisLevel = levels[level];

	for(int i=0; i < thisLevel.waves; i++) {
		// Position across the screen.
		int xPos = randomMq(16, (int)screenBounds.x - 16);
		EnemyType type = (EnemyType)randomMq(0, ENEMY_TYPES);

		// Delay between waves.
		int delay = chance(50) ? thisLevel.density.first : thisLevel.density.second;

		// Speed.
		double speed = randomMq(100, 150) / 100.0;

		// Configuration chances.
		int qty;
		EnemyPattern pattern;
		if(chance(thisLevel.singleChance)) {
			qty = 1;
			pattern = PATTERN_NONE;
		}else if(chance(thisLevel.columnChance)) {
			qty = randomMq(thisLevel.columnRange.first, thisLevel.columnRange.second);
			pattern = PATTERN_NONE;
		}else if(chance(thisLevel.snakeChance)) {
			qty = randomMq(thisLevel.snakeRange.first, thisLevel.snakeRange.second);
			pattern = PATTERN_SNAKE;
		}

		// Shoot chance.
		EnemyCombat shoots = chance(thisLevel.shootChance) ? COMBAT_HOMING : COMBAT_IDLE;

		// Spawn it.
		wave(0, W_COL, xPos, NA, pattern, type, shoots, false, speed, 1, HEALTH_LIGHT, qty);

		// End of level trigger.
		if(i == thisLevel.waves-1) {
			// Last level = spawn boss.
			if(level == MAX_LEVELS-1) {
				pause(6000);
				warning();
				wave(0, W_COL, CENTER, 310, PATTERN_BOSS_INTRO, ENEMY_BOSS_INTRO, COMBAT_IDLE, false, 2, 0, 200, 1);
				pause(3500);
				wave(0, W_COL, CENTER, NA, PATTERN_BOSS, ENEMY_BOSS, COMBAT_HOMING, false, 0.5, 1, 200, 1);

			// Otherwise, change level.
			} else {
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
