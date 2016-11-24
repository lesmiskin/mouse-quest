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

typedef enum {
    SNAKE,
    MAG_SPLIT,
    SNAKE_REV,
    STRAFER,
    PEELER,
    SWIRLER,
    WARNING,
    BOSS_INTRO,
    BOSS,
	COLUMN
} EnemyPatternDef;

typedef enum {
	POS_LL = 20,
    POS_L = 50,
	POS_LC = 90,
	POS_C = 135,
	POS_CR = 178,
    POS_R = 220,
    POS_RR = 250,
} EnemyPosition;

typedef struct {
    EnemyPatternDef pattern;
    int delay;
    int position;
    EnemyType enemyType;
    EnemyCombat combat;
	int qty;
    double speed;
	double frequency;
	double ampMult;
} MapWave;

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
	bool Finished;
	long StartedPausing;
    bool Started;
	double Frequency;
	double AmpMult;
    long FinishedPausing;
} WaveTrigger;

#define MAX_WAVES 200

static const int NA = -50;

WaveTrigger triggers[MAX_WAVES];
static MapWave mapWaves[100];
static int mapWaveInc = 0;
static int waveAddInc = 0;
static bool pausing = false;
static long gameStartTime;

// -------------------------------------------------------------------------

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
		false, true, false, spawnTime, W_COL, 0, 0, PATTERN_BOB, ENEMY_CD, COMBAT_IDLE, false, 0, 0, 0, 0, false, 0, false, 0, 0, 0
	};

	triggers[waveAddInc++] = e;
}

void wave(int spawnTime, WaveType waveType, int x, int y, EnemyPattern movement, EnemyType type, EnemyCombat combat, bool async, double speed, double speedX, double health, int qty, double frequency, double ampMult) {
	WaveTrigger e = {
		false, false, false, spawnTime, waveType, x, y, movement, type, combat, async, speed, speedX, health, qty, false, 0, false, frequency, ampMult, 0
	};

	triggers[waveAddInc++] = e;
}

void w_column(int x, int y, EnemyPattern movement, EnemyType type, EnemyCombat combat, bool async, double speed, double speedX, int qty, double health, double frequency, double ampMult) {
	int startY = y > NA ? y : NA; //respect Y if given, otherwise normal offscreen pos.
	int sineInc = 0;

	//Ensure "swayiness" is distributed throughout total quantity.
	double sineIncInc = qty / 3.14;

	for(int i=0; i < qty; i++) {
		spawnEnemy(x, startY, type, movement, combat, speed, speedX, sineInc, health, frequency, ampMult);
		startY -= 35;
		if(async) sineInc += sineIncInc;
	}
}

//Wires up wave spawners with their triggers.
void levelGameFrame() {
	if(gameState != STATE_GAME) return;

    // Cycle through all triggers per-frame.
	for(int i=0; i < waveAddInc; i++) {
		WaveTrigger trigger = triggers[i];

		// Ignore finished or unset triggers.
        if(trigger.Finished || invalidWave(&trigger)) continue;

        // We've encountered a pause trigger.
        if(trigger.Pause) {
            // Start pausing if we're not already (NB: We don't bother about many pauses at this point, because
            // we stop looping at the first active or candidate trigger).
            if(!triggers[i].Started) {
                triggers[i].StartedPausing = clock();
                triggers[i].Started = true;
                break;

            // Stop pausing if we've reached the delay time.
            } else if (due(triggers[i].StartedPausing, trigger.SpawnTime)) {
				triggers[i].Finished = true;
                triggers[i].FinishedPausing = clock();

            // Don't cycle past a pause if we're still on it.
			}else{
                break;
            }

        // Warnings.
		}else if(trigger.Warning) {
			toggleWarning();
			triggers[i].Finished = true;

		// Enemies
        }else{
            // If our spawn time is due, SINCE the last pause time, fire away!
            long pauseBeforeWave = gameStartTime;
            for(int j=i; j > 0; j--) {
                if(triggers[j].Pause && triggers[j].Finished) {
                    pauseBeforeWave = triggers[j].FinishedPausing;
                    break;
                }
            }
			if(due(pauseBeforeWave, trigger.SpawnTime)) {
				w_column(trigger.x, trigger.y, trigger.Movement, trigger.Type, trigger.Combat, trigger.Async, trigger.Speed, trigger.SpeedX, trigger.Qty, trigger.Health, trigger.Frequency, trigger.AmpMult);
				triggers[i].Finished = true;
			}
		}
	}
}

int getEnemyPosition(char* str) {
	if(strcmp(str, "LC") == 0) {
		return POS_LC;
	}else if(strcmp(str, "CR") == 0) {
		return POS_CR;
	}else if(strcmp(str, "LL") == 0) {
		return POS_LL;
	}else if(strcmp(str, "RR") == 0) {
		return POS_RR;
	}else if(strcmp(str, "L") == 0) {
		return POS_L;
	}else if(strcmp(str, "R") == 0) {
		return POS_R;
	}else if(strcmp(str, "C") == 0) {
		return POS_C;
	}else{
		if(!isdigit(str[0])) fatalError("Unrecognised level position", str);
		return atoi(str);
	}
}

EnemyCombat getMapShoots(char *str) {
	return strcmp(str, "SHOOTS") == 0 ? COMBAT_HOMING : COMBAT_IDLE;
}

EnemyType getMapEnemy(char *str) {
	if(strcmp(str, "DISK") == 0) {
		return ENEMY_DISK;
	}else if(strcmp(str, "DISK_BLUE") == 0) {
		return ENEMY_DISK_BLUE;
	}else if(strcmp(str, "CONE") == 0) {
		return ENEMY_CONE;
	}else if(strcmp(str, "VIRUS") == 0) {
		return ENEMY_VIRUS;
	}else if(strcmp(str, "MAGNET") == 0) {
		return ENEMY_MAGNET;
	}else if(strcmp(str, "BUG") == 0) {
		return ENEMY_BUG;
	}else if(strcmp(str, "CD") == 0) {
		return ENEMY_CD;
	}else if(strcmp(str, "BOSS_INTRO") == 0) {
		return ENEMY_BOSS_INTRO;
	}else if(strcmp(str, "BOSS") == 0) {
		return ENEMY_BOSS;
	}else{
		fatalError("Unrecognised level enemy", str);
	}
}

double getMapSpeed(char *str) {
	if(strcmp(str, "SLOW") == 0) {
		return 1.2;
	}else if(strcmp(str, "NORMAL") == 0) {
		return 1.7;
	}else if(strcmp(str, "FAST") == 0) {
		return 2.2;
	}else{
		if(!isdigit(str[0])) fatalError("Unrecognised level enemy speed", str);
		return atof(str);
	}
}

EnemyPatternDef getMapPattern(char *str) {
    if(strcmp(str, "SNAKE") == 0) {
        return SNAKE;
	}else if(strcmp(str, "SNAKE_REV") == 0) {
		return SNAKE_REV;
	}else if(strcmp(str, "COLUMN") == 0) {
		return COLUMN;
    }else if(strcmp(str, "MAG_SPLIT") == 0) {
        return MAG_SPLIT;
    }else if(strcmp(str, "STRAFER") == 0) {
        return STRAFER;
    }else if(strcmp(str, "PEELER") == 0) {
        return PEELER;
    }else if(strcmp(str, "SWIRLER") == 0) {
        return SWIRLER;
    }else if(strcmp(str, "WARNING") == 0) {
        return WARNING;
    }else if(strcmp(str, "BOSS_INTRO") == 0) {
        return BOSS_INTRO;
    }else if(strcmp(str, "BOSS") == 0) {
        return BOSS;
    }else{
		fatalError("Unrecognised level pattern", str);
	}
}

void loadLevel() {
	// Open the level file.
#ifdef _WIN32
//	char* fileName = "C:\\Users\\lxm\\dev\\mq\\src\\LEVEL01.csv";
	char* fileName = "C:\\Users\\Les\\dev\\mouse-quest\\src\\LEVEL01.csv";
#elif __linux__
	char* fileName = "/home/les/dev/c/mouse-quest/src/LEVEL01.csv";
#endif
	FILE* file = fopen(fileName, "r");

	// Read each line.
	char line[256];
	while (fgets(line, sizeof(line), file)) {
		// Skip blank and commented-out lines.
		if(strlen(line) == 0 || strcmp(line, "\n") == 0 || line[0] == '#') continue;

		char *part;
		part = strtok (line, ",");
		int partInc = 0;

		// Go through each line.
		while (part != NULL) {
			switch(partInc) {
				case 0:
					// Choose the enemy.
					mapWaves[mapWaveInc].enemyType = getMapEnemy(part);
					break;
				case 1:
					// Shooting.
					mapWaves[mapWaveInc].combat = getMapShoots(part);
					break;
				case 2:
					// Quantity.
					mapWaves[mapWaveInc].qty = atoi(part);
					break;
				case 3:
					// Speed (and spacing).
					mapWaves[mapWaveInc].speed = getMapSpeed(part);
					break;
				case 4:
					// Pattern.
					mapWaves[mapWaveInc].pattern = getMapPattern(part);
					break;
				case 5:
					// Choose the position.
					mapWaves[mapWaveInc].position = getEnemyPosition(part);
					break;
				case 6:
					mapWaves[mapWaveInc].frequency = atof(part);
					break;
				case 7:
					mapWaves[mapWaveInc].ampMult = atoi(part);
					break;
				case 8: {
					// Choose the time.
					mapWaves[mapWaveInc].delay = atoi(part);
					mapWaveInc++;
					break;
				}
			}

			part = strtok (NULL, ",");
			partInc++;
		}
	}

	// Close the file.
	fclose(file);
}

void runLevel() {
    const int LEFT = 40;
    const int RIGHT = 230;
    const int CENTER = 120;
    const int C_LEFT = 120;
    const int C_RIGHT = 150;
    const int RIGHT_OFF = (int)screenBounds.x + 85;
    const int LEFT_OFF = -40;

    for(int w=0; w < mapWaveInc; w++) {
        int offscreenPos = mapWaves[w].position == POS_L ? LEFT_OFF : RIGHT_OFF;
		MapWave map = mapWaves[w];

		// TODO: Leaving column blank should default to not shooting.
		// TODO: Leading pause.
		// TODO: Slow/medium/fast speed affects X speed, too.
		// TODO: Proper sine method.

		// Proportional spacing (based on 350 separation @ 1.7 speed).
		int spacing = (int)(300 * (1.7 / map.speed));

        switch(map.pattern) {

			case COLUMN:
				for(int i=0; i < map.qty; i++)
					wave(i * spacing, W_COL, map.position, NA, PATTERN_NONE, map.enemyType, map.combat, false, map.speed, 0.05, HEALTH_LIGHT, 1, map.frequency, map.ampMult);

				break;

			case SNAKE:
                for(int i=0; i < map.qty; i++)
                    wave(i * spacing, W_COL, map.position, NA, PATTERN_SNAKE, map.enemyType, map.combat, false, map.speed, 0.05, HEALTH_LIGHT, 1, map.frequency, map.ampMult);

                break;

			case SNAKE_REV:
				for(int i=0; i < map.qty; i++)
					wave(i * spacing, W_COL, map.position, NA, PATTERN_SNAKE_REV, map.enemyType, map.combat, false, map.speed, 0.05, HEALTH_LIGHT, 1, map.frequency, map.ampMult);

				break;

			case MAG_SPLIT:
                for(int i=0; i < map.qty; i++) {
                    wave(i * spacing, W_COL, C_LEFT, NA, P_CURVE_LEFT, map.enemyType, map.combat, false, map.speed, 1, HEALTH_LIGHT, 1, map.frequency, map.ampMult);
                    wave(i * spacing, W_COL, C_RIGHT, NA, P_CURVE_RIGHT, map.enemyType, map.combat, false, map.speed, 1, HEALTH_LIGHT, 1, map.frequency, map.ampMult);
                }
                break;

            case STRAFER:
                for(int i=0; i < map.qty; i++) {
                    wave(ceil(i * spacing * 1.5), W_COL, offscreenPos, -40,
                         mapWaves[w].position == POS_L ? P_STRAFE_RIGHT : P_STRAFE_LEFT,
						 map.enemyType, COMBAT_HOMING, false, map.speed, 1.5, HEALTH_LIGHT, 1, map.frequency, map.ampMult);
                }
                break;

            case PEELER:
                for(int i=0; i < map.qty; i++) {
                    wave(i * spacing, W_COL, map.position, NA,
                         mapWaves[w].position == POS_LL ? P_PEEL_RIGHT : P_PEEL_LEFT,
						 map.enemyType, map.combat, false, map.speed, 0.008, HEALTH_LIGHT, 1, map.frequency, map.ampMult);
                }
                break;

            case SWIRLER:
                for(int i=0; i < map.qty; i++) {
                    wave(i * spacing, W_COL, mapWaves[w].position + 50, NA, P_SWIRL_RIGHT, map.enemyType, map.combat, false, map.speed, 0.09, HEALTH_LIGHT, 1, map.frequency, map.ampMult);
                    wave((spacing/2) + i * spacing, W_COL, mapWaves[w].position, NA, P_SWIRL_LEFT, map.enemyType, i == 4 ? map.combat : map.combat, false, map.speed, 0.09, HEALTH_LIGHT, 1, map.frequency, map.ampMult);
                }
                break;

            case WARNING:
                warning();
                break;

            case BOSS_INTRO:
                wave(0, W_COL, map.position, 310, PATTERN_BOSS_INTRO, map.enemyType, COMBAT_IDLE, false, map.speed, 0, 200, 1, map.frequency, map.ampMult);
                break;

            case BOSS:
				wave(0, W_COL, map.position, NA, PATTERN_BOSS, map.enemyType, COMBAT_HOMING, false, map.speed, 1, 200, 1, map.frequency, map.ampMult);
//                wave(0, W_COL, map.position, NA, PATTERN_BOSS, map.enemyType, COMBAT_HOMING, false, map.speed, 1, 1, 1, map.frequency, map.ampMult);
                break;
        }

        if(mapWaves[w].delay > 0) {
            pause(mapWaves[w].delay);
        }
    }
}

void resetLevel() {
    gameStartTime = clock();
	waveAddInc = 0;
}

void levelInit() {
	loadLevel();
	resetLevel();
}
