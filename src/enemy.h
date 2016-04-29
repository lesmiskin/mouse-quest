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
	ENEMY_MAGNET,
	ENEMY_DISK,
	ENEMY_DISK_BLUE,
	ENEMY_VIRUS,
	ENEMY_BUG,
	ENEMY_CD,
} EnemyType;

typedef enum {
	PATTERN_NONE,
	PATTERN_SNAKE,
	PATTERN_BOB,
	PATTERN_CIRCLE,
	PATTERN_PEEL_LEFT,
	PATTERN_PEEL_RIGHT,

	PATTERN_STRAFE_LEFT,
	PATTERN_STRAFE_RIGHT,

	PATTERN_PEEL_FAR_LEFT,
	PATTERN_PEEL_FAR_RIGHT,


//	PATTERN_ZIP,
//	PATTERN_CROSS_OVER,
//	PATTERN_FUNNEL,
//	PATTERN_INVERT_FUNNEL,
//	PATTERN_Z,
//	PATTERN_Z_CROSS,
//	PATTERN_Z_EASY,
} EnemyPattern;

typedef enum {
	COMBAT_IDLE,
	COMBAT_SHOOTER
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
} Enemy;

extern const double HEALTH_LIGHT, HEALTH_HEAVY;
extern bool invalidEnemy(Enemy* enemy);
extern void resetEnemies();
extern void spawnEnemy(int x, int y, EnemyType type, EnemyPattern movement, EnemyCombat combat, double speed, double swayInc, double health);
extern void hitEnemy(Enemy* enemy, double damage, bool collision);
extern const int ENEMY_BOUND;
extern Enemy enemies[MAX_ENEMIES];
extern void enemyInit();
extern void enemyShadowFrame();
extern void enemyRenderFrame();
extern void enemyGameFrame();
extern void animateEnemy();

#endif