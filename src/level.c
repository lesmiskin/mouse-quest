#include <stdbool.h>
#include <time.h>
#include "common.h"
#include "enemy.h"
#include "hud.h"

typedef enum {
	W_COL,
	W_WARNING
} WaveType;

typedef struct {
	bool Warning;
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

#define MAX_WAVES 100
WaveTrigger triggers[MAX_WAVES];
long gameTime = 0;
int waveInc = 0;
int waveAddInc = 0;
const int NA = -50;
const int ENEMY_SPACE = 35;

bool invalidWave(WaveTrigger *wave) {
	return wave->Health == 0 && !wave->Pause && !wave->Warning;
}

void warning() {
	WaveTrigger e = {
		true, false, false, 0, W_WARNING, 0, 0, PATTERN_BOB, ENEMY_CD, COMBAT_IDLE, false, 0, 0, 0, 0
	};

	triggers[waveAddInc++] = e;
}

void pause(int spawnTime) {
	WaveTrigger e = {
		false, true, false, spawnTime, W_COL, 0, 0, PATTERN_BOB, ENEMY_CD, COMBAT_IDLE, false, 0, 0, 0, 0
	};

	triggers[waveAddInc++] = e;
}

void wave(int spawnTime, WaveType waveType, int x, int y, EnemyPattern movement, EnemyType type, EnemyCombat combat, bool async, double speed, double speedX, double health, int qty) {
	WaveTrigger e = {
		false, false, false, spawnTime, waveType, x, y, movement, type, combat, async, speed, speedX, health, qty
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

	if(	waveInc < MAX_WAVES && !invalidWave(&trigger) && due(gameTime, trigger.SpawnTime) ) {
		if(trigger.Pause) {
			if (due(gameTime, trigger.SpawnTime)) {
				gameTime = clock();
				waveInc++;
			}
		}else if(trigger.Warning) {
			toggleWarning();
			waveInc++;
		}else{
			w_column(trigger.x, trigger.y, trigger.Movement, trigger.Type, trigger.Combat, trigger.Async, trigger.Speed, trigger.SpeedX, trigger.Qty, trigger.Health);
			waveInc++;
		}
	}
}

void levelInit() {
	const int LEFT = 40;
	const int RIGHT = 230;
	const int CENTER = 135;
	const int C_LEFT = 120;
	const int C_RIGHT = 150;
	const int RIGHT_OFF = (int)screenBounds.x + 85;
	const int LEFT_OFF = -40;

//	pause(2000);
//    warning();
//    wave(0, W_COL, CENTER, 310, PATTERN_BOSS_INTRO, ENEMY_BOSS_INTRO, COMBAT_IDLE, false, 2, 0, 200, 1);
//    pause(3000);
//    wave(0, W_COL, CENTER, NA, PATTERN_BOSS, ENEMY_BOSS, COMBAT_HOMING, false, 0.5, 1, 1, 1);
//	return;

	pause(2000);

	// Two columns that split, and merge (NEEDS SINE)
	for(int i=0; i < 6; i++) {
		wave(i * 350, W_COL, C_LEFT, NA, P_CURVE_LEFT, ENEMY_MAGNET, COMBAT_IDLE, false, 1.4, 1, HEALTH_LIGHT, 1);
		wave(i * 350, W_COL, C_RIGHT, NA, P_CURVE_RIGHT, ENEMY_MAGNET, COMBAT_IDLE, false, 1.4, 1, HEALTH_LIGHT, 1);
	}
	pause(6000);


	// Snakes.
	for(int i=0; i < 8; i++) {
		wave(i * 350, W_COL, C_LEFT, NA, P_SNAKE_RIGHT, ENEMY_DISK, COMBAT_IDLE, false, 1.2, 0.05, HEALTH_LIGHT, 1);
	}
	pause(5000);
	for(int i=0; i < 8; i++) {
		wave(i * 350, W_COL, C_RIGHT, NA, P_SNAKE_LEFT, ENEMY_DISK_BLUE, COMBAT_IDLE, false, 1.2, 0.05, HEALTH_LIGHT, 1);
	}
	pause(6000);


	// Crossover.
	for(int i=0; i < 6; i++) {
		wave(i * 350, W_COL, LEFT, NA, P_CROSS_LEFT, ENEMY_DISK, COMBAT_IDLE, false, 1.2, 0.03, HEALTH_LIGHT, 1);
		wave(i * 350, W_COL, RIGHT, NA, P_CROSS_RIGHT, ENEMY_DISK_BLUE, COMBAT_IDLE, false, 1.2, 0.03, HEALTH_LIGHT, 1);
	}
	pause(6000);


	// Strafers coming from either side.
	for(int i=0; i < 3; i++) {
		wave(i * 750, W_COL, RIGHT_OFF, -40, P_STRAFE_LEFT, ENEMY_VIRUS, COMBAT_HOMING, false, 0.7, 0.004, HEALTH_LIGHT, 1);
	}
	pause(4000);
	for(int i=0; i < 3; i++) {
		wave(i * 750, W_COL, LEFT_OFF, -40, P_STRAFE_RIGHT, ENEMY_VIRUS, COMBAT_HOMING, false, 0.7, 0.004, HEALTH_LIGHT, 1);
	}
	pause(7000);

	// Peelers.
	for(int i=0; i < 7; i++) {
		wave(i * 300, W_COL, LEFT, NA, P_PEEL_RIGHT, ENEMY_BUG, COMBAT_HOMING, false, 2, 0.02, HEALTH_LIGHT, 1);
	}
	pause(3500);
	for(int i=0; i < 7; i++) {
		wave(i * 300, W_COL, RIGHT, NA, P_PEEL_LEFT, ENEMY_BUG, COMBAT_HOMING, false, 2, 0.02, HEALTH_LIGHT, 1);
	}
	pause(5000);

	// Swirlers.
	for(int i=0; i < 5; i++) {
		wave(i * 325, W_COL, LEFT + 50, NA, P_SWIRL_RIGHT, ENEMY_MAGNET, COMBAT_IDLE, false, 1.4, 0.09, HEALTH_LIGHT, 1);
		wave(150 + i * 325, W_COL, LEFT, NA, P_SWIRL_LEFT, ENEMY_MAGNET, i == 4 ? COMBAT_SHOOTER : COMBAT_IDLE, false, 1.4, 0.09, HEALTH_LIGHT, 1);
	}
	pause(5000);
	for(int i=0; i < 5; i++) {
		wave(i * 325, W_COL, RIGHT, NA, P_SWIRL_RIGHT, ENEMY_MAGNET, COMBAT_IDLE, false, 1.4, 0.09, HEALTH_LIGHT, 1);
		wave(150 + i * 325, W_COL, RIGHT - 50, NA, P_SWIRL_LEFT, ENEMY_MAGNET, i == 4 ? COMBAT_SHOOTER : COMBAT_IDLE, false, 1.4, 0.09, HEALTH_LIGHT, 1);
	}

	pause(6000);

    warning();
    wave(0, W_COL, CENTER, 310, PATTERN_BOSS_INTRO, ENEMY_BOSS_INTRO, COMBAT_IDLE, false, 2, 0, 200, 1);
    pause(3500);
    wave(0, W_COL, CENTER, NA, PATTERN_BOSS, ENEMY_BOSS, COMBAT_HOMING, false, 0.5, 1, 200, 1);
}

void resetLevel() {
	gameTime = clock();
	waveInc = 0;
}
