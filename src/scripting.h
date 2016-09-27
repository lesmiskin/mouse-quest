#ifndef SCRIPTING_H
#define SCRIPTING_H

#include "renderer.h"

#define MAX_SCRIPTS 10
#define MAX_SCENES 25

typedef enum {
	SCENE_CUE,
	SCENE_LOOP,
	SCENE_INFINITE,
	SCENE_STATE
} SceneType;

typedef enum {
	SCENE_UNINITIALISED = 0,
	SCENE_INITIALISED = 1,		//Cue scenes will always end here.
	SCENE_FADE_IN = 2,
	SCENE_LOOPING = 3,
	SCENE_FADE_OUT = 4
} SceneProgress;

typedef struct {
	int sceneNumber;
	long sceneTimer;
	SceneProgress sceneProgress;

} ScriptStatus;

typedef struct {
	SceneType type;
	int duration;
	FadeMode fadeMode;
	GameState toState;
} Scene;

typedef struct {
	Scene scenes[MAX_SCENES];
	int totalScenes;
} Script;

extern Script scripts[MAX_SCRIPTS];
extern ScriptStatus scriptStatus;

extern bool sceneInitialised();
extern Scene newTimedStep(SceneType trigger, int milliseconds, FadeMode fadeMode);
extern Scene newCueStep();
extern Scene newStateStep(GameState toState);
extern void endOfFrameTransition();
extern void resetScriptStatus();

#endif
