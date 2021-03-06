#ifndef ENEMY_H
#define ENEMY_H

#include "common.h"
#include "renderer.h"

#define MAX_ENEMIES 200

typedef enum {
	ENEMY_ANIMATION_IDLE = 0,
	ENEMY_ANIMATION_DEATH = 1,
} EnemyAnimation;

typedef enum {
	ENEMY_MAGNET = 0,
	ENEMY_DISK = 1,
	ENEMY_DISK_BLUE = 2,
	ENEMY_VIRUS = 3,
	ENEMY_BUG = 4,
	ENEMY_CD = 5,
	ENEMY_BOSS = 6,
	ENEMY_BOSS_INTRO = 7,
	ENEMY_CONE = 8
} EnemyType;

typedef enum {
	PATTERN_NONE,
	PATTERN_BOSS,
	PATTERN_SNAKE,
	PATTERN_SNAKE_REV,
	PATTERN_BOB,
	PATTERN_CIRCLE,
	PATTERN_BOSS_INTRO,

	P_CURVE_LEFT,
	P_CURVE_RIGHT,

	P_STRAFE_LEFT,
	P_STRAFE_RIGHT,

	P_SNAKE_LEFT,
	P_SNAKE_RIGHT,

	P_CROSS_LEFT,
	P_CROSS_RIGHT,

	P_PEEL_RIGHT,
	P_PEEL_LEFT,

	P_SWIRL_RIGHT,
	P_SWIRL_LEFT,
} EnemyPattern;

typedef enum {
	COMBAT_IDLE,
	COMBAT_SHOOTER,
	COMBAT_HOMING
} EnemyCombat;

typedef struct {
	Coord origin;
	Coord formationOrigin;
	Coord parallax;
	Sprite sprite;
	bool hitAnimate;
	double health;
	double strength;		//how resistent enemy is to knockbacks.
	int animFrame;
	EnemyAnimation animSequence;
	bool dying;
	EnemyType type;
	long lastShotTime;
	double speed;
	double speedX;
	char frameName[50];
	bool wasHitLastFrame;
	bool initialFrameChosen;
	EnemyPattern movement;
	EnemyCombat combat;
	double swayIncX;
	double swayIncY;
	Coord offset;
	bool collided;
	long spawnTime;
	long lastBlastTime;
	bool blasting;
	int scriptInc;
	bool scrollDir;
	double collisionDamage;
	long boomTime;
	long fatalTime;
	bool nonInteractive;
	bool inBackground;
	double frequency;
	double ampMult;
} Enemy;

extern double bossHealth;
extern bool bossOnscreen;
extern void spawnBoom(Coord origin, double scale);
extern const double HEALTH_LIGHT;
extern bool invalidEnemy(Enemy* enemy);
extern void resetEnemies();
extern void spawnEnemy(int x, int y, EnemyType type, EnemyPattern movement, EnemyCombat combat, double speed, double speedX, double swayInc, double health, double frequency, double ampMult);
extern void hitEnemy(Enemy* enemy, double damage, bool collision);
extern const int ENEMY_BOUND;
extern Enemy enemies[MAX_ENEMIES];
extern void enemyInit();
extern void enemyShadowFrame();
extern void enemyBackgroundRenderFrame();
extern void enemyRenderFrame();
extern void enemyGameFrame();
extern void animateEnemy();

#endif