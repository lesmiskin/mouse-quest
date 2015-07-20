#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"

typedef enum {
	PSTATE_NOT_PLAYING = 1,
	PSTATE_NORMAL = 2,
	PSTATE_WON = 4,
	PSTATE_DYING = 8,
	PSTATE_DEAD = 16
} PlayerState;

extern PlayerState playerState;
extern bool showMike;
extern bool playerDying;
extern void collidePlayer(double strength);
extern void hitPlayer(double damage);
extern double playerStrength;
extern Coord playerOrigin;
extern void playerInit(void);
extern void playerAnimate(void);
extern void playerShadowFrame(void);
extern void playerRenderFrame(void);
extern void playerGameFrame(void);
extern void resetPlayer(void);

#endif