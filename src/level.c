#include <stdbool.h>
#include <time.h>
#include "common.h"
#include "enemy.h"

typedef enum {
	W_LINE,
	W_COL,
	W_TRI,
	W_ANGLE_LEFT,
	W_ANGLE_RIGHT,
	W_DELTA_DOWN,
	W_DELTA_UP,
} WaveType;

typedef struct {
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

#define MAX_WAVES 50
WaveTrigger triggers[MAX_WAVES];
long gameTime = 0;
int waveInc = 0;
int waveAddInc = 0;
const int NA = -1;
const int ENEMY_SPACE = 35;

bool invalidWave(WaveTrigger *wave) {
	return wave->SpawnTime == 0;
}

void wave(int spawnTime, WaveType waveType, int x, int y, EnemyPattern movement, EnemyType type, EnemyCombat combat, bool async, double speed, double speedX, double health, int qty) {
	WaveTrigger e = {
		spawnTime, waveType, x, y, movement, type, combat, async, speed, speedX, health, qty
	};

	triggers[waveAddInc++] = e;
}

//void w_tri(int x, EnemyType type, EnemyPattern movement, EnemyCombat combat, double speed) {
//	int y = -50;
//
//	spawnEnemy(x, y - 30, type, movement, combat, speed, 0, HEALTH_LIGHT);
//	spawnEnemy(x + 20, y, type, movement, combat, speed, 0, HEALTH_LIGHT);
//	spawnEnemy(x + 40, y - 30, type, movement, combat, speed, 0, HEALTH_LIGHT);
//}
//
//void w_delta(EnemyPattern movement, EnemyType type, EnemyCombat combat, double speed, bool dir) {
//	int startY = dir ? 0 -50 : -100;
//	int startX = 45;
//
//	for(int i=0; i < 5; i++) {
//		if(startY == 2) {
//			startY = 0;
//			continue;
//		}
//
//		if(dir) {
//			startY = i <= 2 ? startY - 20 : startY + 20;
//		}else{
//			startY = i <= 2 ? startY + 20 : startY - 20;
//		}
//
//		spawnEnemy(startX, startY, type, movement, combat, speed, 0, HEALTH_LIGHT);
//		startX += 45;
//	}
//}
//
//void w_line(EnemyPattern movement, EnemyType type, EnemyCombat combat, double speed) {
//	int startY = -50;
//	int startX = 50;
//
//	for(int i=0; i < 4; i++) {
//		spawnEnemy(startX, startY, type, movement, combat, speed, 0, HEALTH_LIGHT);
//		startX += 50;
//	}
//}
//
//void w_angle(EnemyPattern movement, EnemyType type, EnemyCombat combat, double speed, bool angleDir) {
//	int startY = angleDir ? -50 : -50 + (-40 * 3);
//	int startX = 70;
//
//	for(int i=0; i < 4; i++) {
//		spawnEnemy(startX, startY, type, movement, combat, speed, 0, HEALTH_LIGHT);
//		startX += 40;
//		startY = angleDir ? startY - 40 : startY + 40;
//	}
//}
//

void w_column(int x, EnemyPattern movement, EnemyType type, EnemyCombat combat, bool async, double speed, double speedX, int qty, double health) {
	int startY = -50;
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
		w_column(trigger.x, trigger.Movement, trigger.Type, trigger.Combat, trigger.Async, trigger.Speed, trigger.SpeedX, trigger.Qty, trigger.Health);
		waveInc++;
	}
}

void levelInit() {

	const int LEFT = 20;
	const int RIGHT = 250;
	const int C_LEFT = 120;
	const int C_RIGHT = 150;


	// Two columns that split, and merge.
	for(int i=0; i < 6; i++) {
		wave(1000 + (i * 350), W_COL, C_LEFT, NA, P_CURVE_LEFT, ENEMY_MAGNET, COMBAT_IDLE, false, 1.5, 1, HEALTH_LIGHT, 1);
		wave(1000 + (i * 350), W_COL, C_RIGHT, NA, P_CURVE_RIGHT, ENEMY_MAGNET, COMBAT_IDLE, false, 1.5, 1, HEALTH_LIGHT, 1);
	}



//	// Two columns that cross over mid-way down.
//	for(int i=0; i < 6; i++) {
//		wave(6000 + (i * 350), W_COL, RIGHT, NA, P_CROSSOVER_LEFT, ENEMY_DISK, COMBAT_IDLE, false, 1, 1.9, HEALTH_LIGHT, 1);
//		wave(6000 + (i * 350), W_COL, LEFT, NA, P_CROSSOVER_RIGHT, ENEMY_DISK_BLUE, COMBAT_IDLE, false, 1, 1.9, HEALTH_LIGHT, 1);
//	}
//
//	// Strafers coming from either side.
//	for(int i=0; i < 3; i++)
//		wave(11000 + (i * 800), W_COL, 250 + 48, NA, P_STRAFE_LEFT, ENEMY_VIRUS, COMBAT_SHOOTER, false, 0.8, 1.8, HEALTH_LIGHT, 1);
//	for(int i=0; i < 3; i++)
//		wave(14500 + (i * 800), W_COL, 0 - 48, NA, P_STRAFE_RIGHT, ENEMY_BUG, COMBAT_SHOOTER, false, 0.8, 1.8, HEALTH_LIGHT, 1);










//	 Things get biffed in rapidly from either side.
//	for(int i=0; i < 4; i++) {
//		wave(11000 + (i * 500), W_COL, 250 + 48, NA, P_STRAFE_LEFT, ENEMY_CD, COMBAT_IDLE, false, 1.4, 1, HEALTH_LIGHT, 1);
//		wave(11250 + (i * 500), W_COL, 0 - 48, NA, P_STRAFE_RIGHT, ENEMY_CD, COMBAT_IDLE, false, 1.4, 1, HEALTH_LIGHT, 1);
//	}

//	wave(1000, W_COL, C_LEFT, NA, P_CURVE_LEFT, ENEMY_CD, COMBAT_IDLE, false, 1.7, HEALTH_LIGHT, 1);
//	wave(1000, W_COL, C_RIGHT, NA, PATTERN_PEEL_RIGHT, ENEMY_CD, COMBAT_IDLE, false, 1.7, HEALTH_LIGHT, 1);
//
//	wave(1300, W_COL, C_LEFT, NA, P_CURVE_LEFT, ENEMY_CD, COMBAT_IDLE, false, 1.7, HEALTH_LIGHT, 1);
//	wave(1300, W_COL, C_RIGHT, NA, PATTERN_PEEL_RIGHT, ENEMY_CD, COMBAT_IDLE, false, 1.7, HEALTH_LIGHT, 1);
//
//	wave(1600, W_COL, C_LEFT, NA, P_CURVE_LEFT, ENEMY_CD, COMBAT_IDLE, false, 1.7, HEALTH_LIGHT, 1);
//	wave(1600, W_COL, C_RIGHT, NA, PATTERN_PEEL_RIGHT, ENEMY_CD, COMBAT_IDLE, false, 1.7, HEALTH_LIGHT, 1);



	//Start of level - 2 columns, and space invaders.
//
//	wave(1000, W_COL, 50, NA, PATTERN_SNAKE, ENEMY_DISK, COMBAT_IDLE, true, 1, HEALTH_LIGHT);
//	wave(1000, W_COL, 200, NA, PATTERN_SNAKE, ENEMY_DISK_BLUE, COMBAT_IDLE, true, 1, HEALTH_LIGHT);
//
//	wave(6000, W_LINE, NA, NA, PATTERN_SNAKE, ENEMY_BUG, COMBAT_IDLE, false, 0.7, HEALTH_LIGHT);
//	wave(7000, W_LINE, NA, NA, PATTERN_SNAKE, ENEMY_VIRUS, COMBAT_IDLE, false, 0.7, HEALTH_LIGHT);
//
//	//10 seconds in - triads.
//
//	wave(12000, W_TRI, 50, NA, PATTERN_NONE, ENEMY_DISK, COMBAT_IDLE, false, 1.7, HEALTH_LIGHT);
//	wave(14000, W_TRI, 200, NA, PATTERN_NONE, ENEMY_DISK_BLUE, COMBAT_IDLE, false, 1.7, HEALTH_LIGHT);
//
//	wave(17000, W_TRI, 50, NA, PATTERN_SNAKE, ENEMY_CD, COMBAT_IDLE, false, 1.7, HEALTH_LIGHT);
//	wave(18000, W_TRI, 200, NA, PATTERN_SNAKE, ENEMY_CD, COMBAT_IDLE, false, 1.7, HEALTH_LIGHT);
//
//	//20 seconds in - Diagonal magnets.
//
//	wave(22000, W_ANGLE_LEFT, 50, NA, PATTERN_NONE, ENEMY_MAGNET, COMBAT_IDLE, false, 2, HEALTH_LIGHT);
//	wave(25000, W_ANGLE_RIGHT, 50, NA, PATTERN_NONE, ENEMY_MAGNET, COMBAT_IDLE, false, 2, HEALTH_LIGHT);
//
//	//Introduce the shooters, first in simple columns, then waving.
//
//	wave(30000, W_COL, 50, NA, PATTERN_NONE, ENEMY_VIRUS, COMBAT_SHOOTER, true, 1, HEALTH_HEAVY);
//	wave(33000, W_COL, 200, NA, PATTERN_NONE, ENEMY_BUG, COMBAT_SHOOTER, true, 1, HEALTH_HEAVY);
//	wave(36000, W_COL, 50, NA, PATTERN_SNAKE, ENEMY_VIRUS, COMBAT_SHOOTER, true, 1, HEALTH_HEAVY);
//	wave(39000, W_COL, 200, NA, PATTERN_SNAKE, ENEMY_BUG, COMBAT_SHOOTER, true, 1, HEALTH_HEAVY);

	// A few deltas...

//	wave(44000, W_DELTA_DOWN, 30, NA, MOVEMENT_STRAIGHT, ENEMY_DISK, COMBAT_IDLE, true, 1.3, HEALTH_LIGHT);
//	wave(46000, W_DELTA_DOWN, 30, NA, MOVEMENT_TITLE_BOB, ENEMY_DISK_BLUE, COMBAT_SHOOTER, true, 1, HEALTH_LIGHT);


	//Column options which can peel off to the sides.
	//Spirals.
	//Carrier columns (floppy line flanked with bugs).
	//Super carrier (blue floppy line flanked with viruses).
	//Spirals with shooters inside them.
	//Tough (health) guys that come down, then peel offscreen.
	//Guys that come in from the sides, then peel off to the other side.
	//?? Guys that plunge in from the sides.
	//Delta lines, Up/down bobbing enemies in blocks.
	//Guys that cruise in form the sides, and shoot while doing it.






	//Little darters down the screen *COMBINED* with side crusers.
	//WARNING WARNING || SHOOT THE CORE || WARNING WARNING.
}

void resetLevel() {
	gameTime = clock();
	waveInc = 0;
}
