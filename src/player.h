#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"

typedef enum {
	PSTATE_NOT_PLAYING = 1,
	PSTATE_NORMAL = 2,
	PSTATE_WON = 4,
	PSTATE_DYING = 8,
	PSTATE_SMILING = 16,
	PSTATE_DEAD = 32
} PlayerState;

extern void smile(void);
extern bool atFullhealth(void);
extern void restoreHealth(void);
extern PlayerState playerState;
extern bool useMike;
extern bool playerDyingGame;
extern void hitPlayer(double damage);
extern double playerStrength;
extern double playerHealth;
extern Coord playerOrigin;
extern void playerInit(void);
extern void playerAnimate(void);
extern void playerShadowFrame(void);
extern void playerRenderFrame(void);
extern void playerGameFrame(void);
extern void resetPlayer(void);

#endif