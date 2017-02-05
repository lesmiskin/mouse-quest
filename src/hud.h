#ifndef HUD_H
#define HUD_H

typedef enum {
	PLUME_SCORE,
	PLUME_LASER,
	PLUME_POWER
} PlumeType;

extern void hudRenderFrameBackground();
extern bool waveCompleteOn;
extern void insertCoin();
extern void persistentHudRenderFrame();
extern void toggleWarning();
extern void spawnPlume(PlumeType type);
extern int score;
extern int topScore;
extern int coins;
extern void raiseScore(unsigned amount, bool plume);
extern void hudGameFrame();
extern void hudRenderFrame();
extern void hudInit();
extern void hudAnimateFrame();
extern void resetHud(bool keepScore);

#endif
