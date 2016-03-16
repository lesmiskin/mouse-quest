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
	MOVEMENT_STRAIGHT,
	MOVEMENT_SNAKE,
	MOVEMENT_CIRCLE,
	MOVEMENT_TITLE_BOB
} EnemyMovement;

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
	EnemyMovement movement;
	EnemyCombat combat;
	double swayIncX;
	double swayIncY;
	Coord offset;
	bool collided;
} Enemy;

extern bool invalidEnemy(Enemy* enemy);
extern void resetEnemies();
extern void spawnEnemy(int x, int y, EnemyType type, EnemyMovement movement, EnemyCombat combat, double speed, double swayInc);
extern void hitEnemy(Enemy* enemy, double damage, bool collision);
extern const int ENEMY_BOUND;
extern Enemy enemies[MAX_ENEMIES];
extern void enemyInit(void);
extern void enemyShadowFrame(void);
extern void enemyRenderFrame(void);
extern void enemyGameFrame(void);
extern void animateEnemy(void);

#endif