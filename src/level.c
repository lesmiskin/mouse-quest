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
	return wave->Health == 0 && !wave->Pause;
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
			if(due(gameTime, trigger.SpawnTime)) {
				gameTime = clock();
				waveInc++;
			}
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

	// BUG: Homing doesn't work on peelers.
	// Peelers are *WAY* too fast.




	// X guys are boring. Cross over at different times.
	// Need some sine-ed snakes.
	// Fix it so you can't sit in one place and shoot ("U" enemies should collide).
	// Introduce some homing shots.
	// Hold a button for "rail" shot? / Powerup?

	// Column of PCB on either side, with chips.
	// "Castle" PCB in center of screen.
	// Donut PCB with a single "thing" (enemy?) in the middle.
	// When you blow up a chip: you get 4 coins.
	// When you destroy a wave sequence, you get fruit.

	// When a certain amout of time has elapsed, a powerup will spawn.
	// Powerups won't spawn until X time has elapsed.

	// Carrier columns (floppy line flanked with bugs).
	// Space invader rows and sine left and right.
	// Random spirals.
	// Random spirals with shooters inside.

	// Keys look dumb.
	// Any extra animation we can add to him?

	// Bosses: CRT monitor (face flashes up), Joystick, Keyboard, Floppy drive, CD Drive.

	// WARNING message.
	// Music stops.
	// Massive explosion.
	// Bunch of keys fly out.
	// Secondary attack animation (splits apart and reveals board).


//	wave(0, W_COL, CENTER, NA, PATTERN_BOSS, ENEMY_BOSS, COMBAT_HOMING, false, 0.5, 1, 200, 1);
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
		wave(i * 750, W_COL, RIGHT_OFF, -40, P_STRAFE_LEFT, ENEMY_VIRUS, COMBAT_SHOOTER, false, 0.7, 0.004, HEALTH_LIGHT, 1);
	}
	pause(4000);
	for(int i=0; i < 3; i++) {
		wave(i * 750, W_COL, LEFT_OFF, -40, P_STRAFE_RIGHT, ENEMY_VIRUS, COMBAT_SHOOTER, false, 0.7, 0.004, HEALTH_LIGHT, 1);
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
		wave(150 + i * 325, W_COL, LEFT, NA, P_SWIRL_LEFT, ENEMY_MAGNET, COMBAT_IDLE, false, 1.4, 0.09, HEALTH_LIGHT, 1);
	}
	pause(5000);
	for(int i=0; i < 5; i++) {
		wave(i * 325, W_COL, RIGHT, NA, P_SWIRL_RIGHT, ENEMY_MAGNET, COMBAT_IDLE, false, 1.4, 0.09, HEALTH_LIGHT, 1);
		wave(150 + i * 325, W_COL, RIGHT - 50, NA, P_SWIRL_LEFT, ENEMY_MAGNET, COMBAT_IDLE, false, 1.4, 0.09, HEALTH_LIGHT, 1);
	}
	pause(10000);

	wave(0, W_COL, CENTER, NA, PATTERN_BOSS, ENEMY_BOSS, COMBAT_HOMING, false, 0.5, 1, 200, 1);

	return;
	// Column goes down screen that peels offscreen to the right.
	// Column streamers (fast, go down the screen, pop up in quasi-random locations).
	// Delta (big wide delta of enemies, sines backwards so the triangle is inverted).
	// Wide 45' angles across the screen.
}

void resetLevel() {
	gameTime = clock();
	waveInc = 0;
}
