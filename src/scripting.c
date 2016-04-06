#include <stdbool.h>
#include <time.h>
#include "scripting.h"
#include "scripts.h"
#include "common.h"

/*
 * Every game state is configured as a kind of script. Even the main game loop is defined this way.
 * This ensures we can use a consistent approach to initialising the beginning or middles of state
 * transitions or cinematics, and we can leverage fadein/fadeout as a means to prolong the current
 * state until the transition animation has completed.
 *
 * Each script has a set of 'scenes' defined, that delineate cinematic sequences. For states that
 * effecively have one continuous scene - these just have the one scene with a SCENE_INFINITE type,
 * that will continue looping until the game state is manually changed.
 *
 * The game state itself is kept in common.h, so any code file can easily change it.
 *
 * For simple states, you can use isScriptInitialised for one-time commands, otherwise use an array
 * you can switch through together with 'cue' scenes to choreograph your script more clearly.
 */

ScriptStatus scriptStatus;
Script scripts[MAX_SCRIPTS];

bool sceneInitialised() {
	return scriptStatus.sceneProgress > SCENE_UNINITIALISED;
}

Script getScript() {
	return scripts[gameState];
}

Scene getScene() {
	return getScript().scenes[scriptStatus.sceneNumber];
}

Scene newTimedStep(SceneType trigger, int milliseconds, FadeMode fadeMode) {
	Scene event = { trigger, milliseconds, fadeMode };
	return event;
}

Scene newCueStep() {
	return newTimedStep(SCENE_CUE, 0, FADE_NONE);
}

Scene newStateStep(GameState toState) {
	Scene event = { SCENE_STATE, 0, FADE_NONE, toState };
	return event;
}

void goToNextScene() {
	//Increment, or reset back to zero if we've completed all scenes.
	//NB: Resetting to zero also doubles as a script cycle, and kicking off a new state change.
	if(scriptStatus.sceneNumber < getScript().totalScenes-1){
		scriptStatus.sceneNumber++;
	}else{
		scriptStatus.sceneNumber = 0;
	}

	scriptStatus.sceneProgress = SCENE_UNINITIALISED;
	scriptStatus.sceneTimer = 0;
}

void resetScriptStatus() {
	scriptStatus.sceneNumber = 0;
	scriptStatus.sceneProgress = SCENE_UNINITIALISED;
	scriptStatus.sceneTimer = 0;
}

void endOfFrameTransition() {
	//It's the end of frame. Leverage whether we're uninitialised to do certain things here (e.g. fade in),
	// otherwise we ensure we flag ourselves as initialised before we move to the next scene / frame.
	Scene scene = getScene();

	switch(scene.type) {
		case SCENE_CUE:
			//When a cue frame completes, we want to immediately move to the next frame so that the
			// renderer doesn't have to deal with a single, redundant frame that could render an
			// unusual in-between state. Note that even if the next frame is a cue frame as well,
			// we will repeat this same check on the conclusion of that frame - since this is called
			// every time.
			goToNextScene();
			scriptGameFrame();
			break;
		case SCENE_STATE:
			//Changing to a new state? Reset our local progress, and run the first frame of the next
			// state immediately, as with SCENE_CUE.
			resetScriptStatus();
			triggerState(scene.toState);
			scriptGameFrame();
			break;
		case SCENE_LOOP:
			switch(scriptStatus.sceneProgress) {
				//Initialised, go to fade in or loop.
				case SCENE_UNINITIALISED:
					//Toggle fade in.
					if((scene.fadeMode & FADE_IN) > 0) {
						scriptStatus.sceneProgress = SCENE_FADE_IN;
						fadeIn();
					//Toggle main loop. IMPORTANT: We start our frame clock now, AFTER the fade.
					} else {
						scriptStatus.sceneProgress = SCENE_LOOPING;
						scriptStatus.sceneTimer = clock();
					}
					break;
				//Fading and complete? Go to loop.
				case SCENE_FADE_IN:
					if(!isFading()) {
						scriptStatus.sceneProgress = SCENE_LOOPING;
						scriptStatus.sceneTimer = clock();
					}
					break;
				//Looping and due to stop? Fade out, or go to next scene.
				case SCENE_LOOPING:
					if(due(scriptStatus.sceneTimer, scene.duration)) {
						//If we need to fade out first - do that.
						if((scene.fadeMode & FADE_OUT) > 0) {
							fadeOut();
							scriptStatus.sceneProgress = SCENE_FADE_OUT;
						//Otherwise, next scene.
						} else {
							goToNextScene();
						}
					}
					break;
				//Stopped fade out? Go to next scene.
				case SCENE_FADE_OUT:
					if(!isFading()) {
						goToNextScene();
					}
					break;
			}
			break;
		case SCENE_INFINITE:
			if(scriptStatus.sceneProgress == SCENE_UNINITIALISED) {
				scriptStatus.sceneProgress = SCENE_INITIALISED;
			}
			//Keep cycling 'till we manually trigger a state change.
			break;
	}
}
