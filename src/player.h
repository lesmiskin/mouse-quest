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

extern bool pain;
extern bool isDying();
extern bool godMode;
extern void smile();
extern bool atFullhealth();
extern void restoreHealth();
extern PlayerState playerState;
extern bool useMike;
extern void hitPlayer(double damage);
extern double playerStrength;
extern double playerHealth;
extern Coord playerOrigin;
extern void playerInit();
extern void playerAnimate();
extern void playerShadowFrame();
extern void playerRenderFrame();
extern void playerGameFrame();
extern void resetPlayer();

#endif