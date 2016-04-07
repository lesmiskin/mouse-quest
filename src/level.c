#include <stdbool.h>
#include <time.h>
#include "common.h"
#include "enemy.h"

typedef enum {
	W_LINE,
	W_COLUMN,
	W_COLUMN_SMALL,
	W_TRI,
	W_ANGLE_LEFT,
	W_ANGLE_RIGHT
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
	double Speed;
	double Health;
} WaveTrigger;

#define MAX_WAVES 50
WaveTrigger triggers[MAX_WAVES];
long gameTime = 0;
int waveInc = 0;
int waveAddInc = 0;
const int NA = -1;

bool invalidWave(WaveTrigger *wave) {
	return wave->SpawnTime == 0;
}

void wave(int spawnTime, WaveType waveType, int x, int y, EnemyMovement movement, EnemyType type, EnemyCombat combat, bool async, double speed, double health) {
	WaveTrigger e = {
		spawnTime, waveType, x, y, movement, type, combat, async, speed, health
	};

	triggers[waveAddInc++] = e;
}

const int ENEMY_SPACE = 35;

void w_tri(int x, EnemyType type, EnemyMovement movement, EnemyCombat combat, double speed) {
	int y = -50;

	spawnEnemy(x, y - 30, type, movement, combat, speed, 0, HEALTH_LIGHT);
	spawnEnemy(x + 20, y, type, movement, combat, speed, 0, HEALTH_LIGHT);
	spawnEnemy(x + 40, y - 30, type, movement, combat, speed, 0, HEALTH_LIGHT);
}

void w_line(EnemyMovement movement, EnemyType type, EnemyCombat combat, double speed) {
	int startY = -50;
	int startX = 50;

	for(int i=0; i < 4; i++) {
		spawnEnemy(startX, startY, type, movement, combat, speed, 0, HEALTH_LIGHT);
		startX += 50;
	}
}

void w_angle(EnemyMovement movement, EnemyType type, EnemyCombat combat, double speed, bool angleDir) {
	int startY = angleDir ? -50 : -50 + (-40 * 3);
	int startX = 70;

	for(int i=0; i < 4; i++) {
		spawnEnemy(startX, startY, type, movement, combat, speed, 0, HEALTH_LIGHT);
		startX += 40;
		startY = angleDir ? startY - 40 : startY + 40;
	}
}

void w_column(int x, EnemyMovement movement, EnemyType type, EnemyCombat combat, bool async, double speed, int qty, double health) {
	int startY = -50;
	int sineInc = 0;

	//Ensure "swayiness" is distributed throughout total quantity.
	double sineIncInc = qty / 3.14;

	for(int i=0; i < qty; i++) {
		spawnEnemy(x, startY, type, movement, combat, speed, sineInc, health);
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
			case W_ANGLE_LEFT:
				w_angle(trigger.Movement, trigger.Type, trigger.Combat, trigger.Speed, false);
				break;
			case W_ANGLE_RIGHT:
				w_angle(trigger.Movement, trigger.Type, trigger.Combat, trigger.Speed, true);
				break;
			case W_LINE:
				w_line(trigger.Movement, trigger.Type, trigger.Combat, trigger.Speed);
				break;
			case W_COLUMN_SMALL:
				w_column(trigger.x, trigger.Movement, trigger.Type, trigger.Combat, trigger.Async, trigger.Speed, 2, trigger.Health);
				break;
			case W_COLUMN:
				w_column(trigger.x, trigger.Movement, trigger.Type, trigger.Combat, trigger.Async, trigger.Speed, 4, trigger.Health);
				break;
			case W_TRI:
				w_tri(trigger.x, trigger.Type, trigger.Movement, trigger.Combat, trigger.Speed);
				break;
		}
		waveInc++;
	}
}

void levelInit() {
	//Start of level - 2 columns, and space invaders.

	wave(1000, W_COLUMN, 50, NA, MOVEMENT_SNAKE, ENEMY_DISK, COMBAT_IDLE, true, 1, HEALTH_LIGHT);
	wave(1000, W_COLUMN, 200, NA, MOVEMENT_SNAKE, ENEMY_DISK_BLUE, COMBAT_IDLE, true, 1, HEALTH_LIGHT);

	wave(6000, W_LINE, NA, NA, MOVEMENT_SNAKE, ENEMY_BUG, COMBAT_IDLE, false, 0.7, HEALTH_LIGHT);
	wave(7000, W_LINE, NA, NA, MOVEMENT_SNAKE, ENEMY_VIRUS, COMBAT_IDLE, false, 0.7, HEALTH_LIGHT);

	//10 seconds in - triads.

	wave(12000, W_TRI, 50, NA, MOVEMENT_STRAIGHT, ENEMY_DISK, COMBAT_IDLE, false, 1.7, HEALTH_LIGHT);
	wave(14000, W_TRI, 200, NA, MOVEMENT_STRAIGHT, ENEMY_DISK_BLUE, COMBAT_IDLE, false, 1.7, HEALTH_LIGHT);

	wave(17000, W_TRI, 50, NA, MOVEMENT_SNAKE, ENEMY_CD, COMBAT_IDLE, false, 1.7, HEALTH_LIGHT);
	wave(18000, W_TRI, 200, NA, MOVEMENT_SNAKE, ENEMY_CD, COMBAT_IDLE, false, 1.7, HEALTH_LIGHT);

	//20 seconds in - Diagonal magnets.

	wave(22000, W_ANGLE_LEFT, 50, NA, MOVEMENT_STRAIGHT, ENEMY_MAGNET, COMBAT_IDLE, false, 2, HEALTH_LIGHT);
	wave(25000, W_ANGLE_RIGHT, 50, NA, MOVEMENT_STRAIGHT, ENEMY_MAGNET, COMBAT_IDLE, false, 2, HEALTH_LIGHT);

	wave(30000, W_COLUMN, 50, NA, MOVEMENT_STRAIGHT, ENEMY_VIRUS, COMBAT_SHOOTER, true, 1, HEALTH_HEAVY);
	wave(33000, W_COLUMN, 200, NA, MOVEMENT_STRAIGHT, ENEMY_BUG, COMBAT_SHOOTER, true, 1, HEALTH_HEAVY);
	wave(36000, W_COLUMN, 50, NA, MOVEMENT_SNAKE, ENEMY_VIRUS, COMBAT_SHOOTER, true, 1, HEALTH_HEAVY);
	wave(39000, W_COLUMN, 200, NA, MOVEMENT_SNAKE, ENEMY_BUG, COMBAT_SHOOTER, true, 1, HEALTH_HEAVY);

	//Spirals...

	//Tough (health) guys that come down, then peel offscreen.

	//Guys that come in from the sides, then peel off to the other side.

	//Spirals with shooters inside them.

	//Guys that plunge in from the sides.

	//Up/down bobbing enemies in blocks.

	//WARNING WARNING || SHOOT THE CORE || WARNING WARNING.
}

void resetLevel() {
	gameTime = clock();
	waveInc = 0;
}
