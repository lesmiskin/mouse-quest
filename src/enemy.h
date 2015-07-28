#ifndef ENEMY_H
#define ENEMY_H

#include "common.h"
#include "renderer.h"

typedef enum {
	ENEMY_ANIMATION_IDLE = 0,
	ENEMY_ANIMATION_DEATH = 1,
} EnemyAnimation;

typedef enum {
	ENEMY_DISK,
	ENEMY_DISK_BLUE,
	ENEMY_VIRUS,
	ENEMY_BUG,
	ENEMY_CD,
} EnemyType;

typedef struct {
	Coord origin;
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
} Enemy;

#define MAX_ENEMIES 20
extern void resetEnemies();
extern void spawnEnemy(int x, int y, EnemyType type);
extern void hitEnemy(Enemy* enemy, double damage);
extern const int ENEMY_BOUND;
extern void hit(Enemy p);
extern Enemy enemies[MAX_ENEMIES];
extern int enemyLimit;
extern void enemyInit(void);
extern void enemyShadowFrame(void);
extern void enemyRenderFrame(void);
extern void enemyGameFrame(void);
extern void animateEnemy(void);

#endif