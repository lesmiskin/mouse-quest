#include <stdbool.h>
#include <time.h>
#include "common.h"
#include "enemy.h"
#include "hud.h"
#include "myc.h"

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

typedef enum mapEnemy {
    LEFT_SNAKE,
    RIGHT_SNAKE,
    MAG_SPLIT,
    CROSSOVER,
    STRAFER_LEFT,
    STRAFER_RIGHT,
    PEELER_LEFT,
    PEELER_RIGHT,
    SWIRLER_LEFT,
    SWIRLER_RIGHT,
    WARNING,
    BOSS_INTRO,
    BOSS,
} MapEnemy;

typedef struct mapWave {
    MapEnemy enemy;
    long delay;
} MapWave;

#define MAX_WAVES 100
WaveTrigger triggers[MAX_WAVES];
long gameTime = 0;
int waveInc = 0;
int waveAddInc = 0;
const int NA = -50;
static MapWave waves[50];

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

MapEnemy getMapEnemy(char* str) {
    if(strcmp(str, "LEFT_SNAKE") == 0) {
        return LEFT_SNAKE;
    }else if(strcmp(str, "RIGHT_SNAKE") == 0) {
        return RIGHT_SNAKE;
    }else if(strcmp(str, "MAG_SPLIT") == 0) {
        return MAG_SPLIT;
    }else if(strcmp(str, "CROSSOVER") == 0) {
        return CROSSOVER;
    }else if(strcmp(str, "STRAFER_LEFT") == 0) {
        return STRAFER_LEFT;
    }else if(strcmp(str, "STRAFER_RIGHT") == 0) {
        return STRAFER_RIGHT;
    }else if(strcmp(str, "PEELER_LEFT") == 0) {
        return PEELER_LEFT;
    }else if(strcmp(str, "PEELER_RIGHT") == 0) {
        return PEELER_RIGHT;
    }else if(strcmp(str, "SWIRLER_LEFT") == 0) {
        return SWIRLER_LEFT;
    }else if(strcmp(str, "SWIRLER_RIGHT") == 0) {
        return SWIRLER_RIGHT;
    }else if(strcmp(str, "WARNING") == 0) {
        return WARNING;
    }else if(strcmp(str, "BOSS_INTRO") == 0) {
        return BOSS_INTRO;
    }else if(strcmp(str, "BOSS") == 0) {
        return BOSS;
    }
}

void loadLevel() {
	// Open the level file.
	char* fileName = "C:\\Users\\lxm\\dev\\c\\mouse-quest\\src\\LEVEL01.txt";
	FILE* file = fopen(fileName, "r");

	// Read each line.
	char line[256];
	while (fgets(line, sizeof(line), file)) {
		char *part;
		part = strtok (line, "|");
		int partInc = 0;

		// Go through each line.
		while (part != NULL) {
			switch(partInc) {
				case 0:
					// Choose the enemy.
					waves[waveInc].enemy = getMapEnemy(part);
					break;
				case 1: {
					// Choose the time.
					waves[waveInc].delay = (long)atoi(part);
					waveInc++;
					break;
				}
			}

			part = strtok (NULL, "|");
			partInc++;
		}
	}

	// Close the file.
	fclose(file);

}

static void runLevel() {
    const int LEFT = 40;
    const int RIGHT = 230;
    const int CENTER = 135;
    const int C_LEFT = 120;
    const int C_RIGHT = 150;
    const int RIGHT_OFF = (int)screenBounds.x + 85;
    const int LEFT_OFF = -40;

    for(int w=0; w < waveInc; w++) {
        switch(waves[w].enemy) {

            case MAG_SPLIT:
                for(int i=0; i < 6; i++) {
                    wave(i * 350, W_COL, C_LEFT, NA, P_CURVE_LEFT, ENEMY_MAGNET, COMBAT_IDLE, false, 1.4, 1, HEALTH_LIGHT, 1);
                    wave(i * 350, W_COL, C_RIGHT, NA, P_CURVE_RIGHT, ENEMY_MAGNET, COMBAT_IDLE, false, 1.4, 1, HEALTH_LIGHT, 1);
                }
                break;

            case CROSSOVER:
                for(int i=0; i < 6; i++) {
                    wave(i * 350, W_COL, LEFT, NA, P_CROSS_LEFT, ENEMY_DISK, COMBAT_IDLE, false, 1.2, 0.03, HEALTH_LIGHT, 1);
                    wave(i * 350, W_COL, RIGHT, NA, P_CROSS_RIGHT, ENEMY_DISK_BLUE, COMBAT_IDLE, false, 1.2, 0.03, HEALTH_LIGHT, 1);
                }
                break;

            case STRAFER_LEFT:
                for(int i=0; i < 3; i++) {
                    wave(i * 750, W_COL, RIGHT_OFF, -40, P_STRAFE_LEFT, ENEMY_VIRUS, COMBAT_HOMING, false, 0.7, 0.004, HEALTH_LIGHT, 1);
                }
                break;

            case STRAFER_RIGHT:
                for(int i=0; i < 3; i++) {
                    wave(i * 750, W_COL, LEFT_OFF, -40, P_STRAFE_RIGHT, ENEMY_VIRUS, COMBAT_HOMING, false, 0.7, 0.004, HEALTH_LIGHT, 1);
                }
                break;

            case PEELER_LEFT:
                for(int i=0; i < 7; i++) {
                    wave(i * 300, W_COL, LEFT, NA, P_PEEL_RIGHT, ENEMY_BUG, COMBAT_HOMING, false, 2, 0.02, HEALTH_LIGHT, 1);
                }
                break;

            case PEELER_RIGHT:
                for(int i=0; i < 7; i++) {
                    wave(i * 300, W_COL, RIGHT, NA, P_PEEL_LEFT, ENEMY_BUG, COMBAT_HOMING, false, 2, 0.02, HEALTH_LIGHT, 1);
                }
                break;

            case SWIRLER_LEFT:
                for(int i=0; i < 5; i++) {
                    wave(i * 325, W_COL, LEFT + 50, NA, P_SWIRL_RIGHT, ENEMY_MAGNET, COMBAT_IDLE, false, 1.4, 0.09, HEALTH_LIGHT, 1);
                    wave(150 + i * 325, W_COL, LEFT, NA, P_SWIRL_LEFT, ENEMY_MAGNET, i == 4 ? COMBAT_SHOOTER : COMBAT_IDLE, false, 1.4, 0.09, HEALTH_LIGHT, 1);
                }
                break;

            case SWIRLER_RIGHT:
                for(int i=0; i < 5; i++) {
                    wave(i * 325, W_COL, RIGHT, NA, P_SWIRL_RIGHT, ENEMY_MAGNET, COMBAT_IDLE, false, 1.4, 0.09, HEALTH_LIGHT, 1);
                    wave(150 + i * 325, W_COL, RIGHT - 50, NA, P_SWIRL_LEFT, ENEMY_MAGNET, i == 4 ? COMBAT_SHOOTER : COMBAT_IDLE, false, 1.4, 0.09, HEALTH_LIGHT, 1);
                }
                break;

            case WARNING:
                warning();
                break;

            case BOSS_INTRO:
                wave(0, W_COL, CENTER, 310, PATTERN_BOSS_INTRO, ENEMY_BOSS_INTRO, COMBAT_IDLE, false, 2, 0, 200, 1);
                break;

            case BOSS:
                wave(0, W_COL, CENTER, NA, PATTERN_BOSS, ENEMY_BOSS, COMBAT_HOMING, false, 0.5, 1, 200, 1);
                break;

            case LEFT_SNAKE:
                for(int i=0; i < 8; i++) {
                    wave(i * 350, W_COL, C_LEFT, NA, P_SNAKE_RIGHT, ENEMY_DISK, COMBAT_IDLE, false, 1.2, 0.05, HEALTH_LIGHT, 1);
                }
                break;

            case RIGHT_SNAKE:
                for(int i=0; i < 8; i++) {
                    wave(i * 350, W_COL, C_RIGHT, NA, P_SNAKE_LEFT, ENEMY_DISK_BLUE, COMBAT_IDLE, false, 1.2, 0.05, HEALTH_LIGHT, 1);
                }
                break;
        }

        if(waves[w].delay > 0) {
            pause(waves[w].delay);
        }
    }
}

void levelInit() {
	loadLevel();
    runLevel();
}

void resetLevel() {
	gameTime = clock();
	waveInc = 0;
}
