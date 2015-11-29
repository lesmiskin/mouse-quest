#ifndef HUD_H
#define HUD_H

typedef enum {
	PLUME_SCORE,
	PLUME_LASER,
	PLUME_POWER
} PlumeType;

extern void spawnPlume(PlumeType type);
extern int score;
extern int topScore;
extern int coins;
extern void raiseScore(unsigned amount, bool plume);
extern void hudGameFrame(void);
extern void hudRenderFrame(void);
extern void hudInit(void);
extern void hudAnimateFrame(void);
extern void resetHud(void);

#endif
