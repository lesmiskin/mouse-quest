#include <stdbool.h>
#include <time.h>
#include "common.h"
#include "enemy.h"

typedef enum {
	W_LINE,
	W_COLUMN,
} WaveType;

typedef struct {
	int SpawnTime;
	WaveType WaveType;
	int x;
	int y;
	EnemyMovement Movement;
	EnemyType Type;
	EnemyCombat Combat;
	bool Async;
} WaveTrigger;

#define MAX_WAVES 4
WaveTrigger triggers[MAX_WAVES];
long gameTime = 0;
int waveInc = 0;
int waveAddInc = 0;
const int NA = -1;

bool invalidWave(WaveTrigger *wave) {
	return wave->SpawnTime == 0;
}

void wave(int spawnTime, WaveType waveType, int x, int y, EnemyMovement movement, EnemyType type, EnemyCombat combat, bool async) {
	WaveTrigger e = {
		spawnTime, waveType, x, y, movement, type, combat, async
	};

	triggers[waveAddInc++] = e;
}

void w_line(EnemyMovement movement, EnemyType type, EnemyCombat combat) {
	int startY = -50;
	int startX = 50;
	for(int i=0; i < 4; i++) {
		spawnEnemy(startX, startY, type, movement, combat, 1, 0);
		startX += 50;
	}
}

void w_column(int x, EnemyMovement movement, EnemyType type, EnemyCombat combat, bool async) {
	int startY = -50;
	int sineInc = 0;
	int qty = 4;
	double sineIncInc = qty / 3.14;

	for(int i=0; i < qty; i++) {
		spawnEnemy(x, startY, type, movement, combat, 1, sineInc);
		startY -= 35;
		if(async) sineInc += sineIncInc;
	}
}

//Wires up wave spawners with their triggers.
void levelGameFrame() {
	if(gameState != STATE_GAME) return;

	WaveTrigger trigger = triggers[waveInc];

	if(	waveInc < MAX_WAVES && !invalidWave(&trigger) && due(gameTime, trigger.SpawnTime) ) {
		switch(trigger.WaveType) {
			case W_LINE:
				w_line(trigger.Movement, trigger.Type, trigger.Combat);
				break;
			case W_COLUMN:
				w_column(trigger.x, trigger.Movement, trigger.Type, trigger.Combat, trigger.Async);
				break;
		}
		waveInc++;
	}
}

void levelInit() {
	wave(1000, W_COLUMN, 50, NA, MOVEMENT_SNAKE, ENEMY_DISK, COMBAT_IDLE, true);
	wave(1000, W_COLUMN, 200, NA, MOVEMENT_SNAKE, ENEMY_DISK, COMBAT_IDLE, true);

	wave(4000, W_LINE, NA, NA, MOVEMENT_SNAKE, ENEMY_BUG, COMBAT_SHOOTER, false);
	wave(5000, W_LINE, NA, NA, MOVEMENT_SNAKE, ENEMY_VIRUS, COMBAT_SHOOTER, false);

}

void resetLevel() {
	gameTime = clock();
	waveInc = 0;
}
