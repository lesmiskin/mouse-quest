#include <time.h>
#include "renderer.h"
#include "assets.h"
#include "input.h"
#include "player.h"
#include "enemy.h"
#include "background.h"
#include "input.h"

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

//TODO: Should we queue up the 'desired' state change, so we can fade out from GAME to TITLE?
//TODO: Fade to white.
//TODO: Stop one-frame of initial graphics before fade.
//TODO: Fade in/out of title screen during attract loop (make title part of attract mode?)
//TODO: In intro script, time should mean the time between fades, not including fades.

typedef enum {
	SCENE_CUE,
	SCENE_LOOP,
	SCENE_INFINITE
} SceneType;

typedef enum {
	SCENE_UNINITIALISED = 0,
	SCENE_INITIALISED = 1,		//Cue scenes will always end here.
	SCENE_FADE_IN = 2,
	SCENE_LOOPING = 3,
	SCENE_FADE_OUT = 4
} SceneProgress;

typedef struct {
	SceneType type;
	int duration;
	FadeMode fadeMode;
} SceneDef;

typedef struct {
	int sceneNumber;
	long sceneStartTime;
	SceneProgress sceneProgress;
	bool scriptStarted;
} ScriptStatus;

static ScriptStatus scriptStatus;

bool sceneInitialised() {
	return scriptStatus.sceneProgress > SCENE_UNINITIALISED;
}

//Script-specific vars.
static double intro_StrafeInc;
static SceneDef intro_script[15];
static SceneDef title_script[3];
static char intro_mikeStafeDir;
static Sprite title_logoSprite;
static Coord title_logoLocation;
static bool game_showLevelMessage;
static const double GAME_MESSAGE_DURATION = 1.5;
static long game_messageTime;


static SceneDef newTimedStep(SceneType trigger, int milliseconds, FadeMode fadeMode) {
	SceneDef event = { trigger, milliseconds, fadeMode };
	return event;
}

static SceneDef newCueStep() {
	return newTimedStep(SCENE_CUE, 0, FADE_NONE);
}

typedef enum titleCues {
	TITLE_CUE = 0,
	TITLE_LOOP = 1,
	TITLE_INTRO_CUE = 2
} TitleCues;

typedef enum introCues {
	INTRO_CUE = 0,
	INTRO_LOGO = 1,
	INTRO_BATTLE_CUE = 2,
	INTRO_BATTLE_PRELUDE = 3,
	INTRO_BATTLE_MIKE_ENTER = 4,
	INTRO_BATTLE_MIKE_PAUSE = 5,
	INTRO_BATTLE_MIKE_FIRE = 6,
	INTRO_BATTLE_MIKE_GLOAT = 7,
	INTRO_BATTLE_MIKE_DEPART = 8,
	INTRO_TITLE_CUE = 9
} IntroCues;

void goToNextScene(SceneType type, ScriptStatus *status) {
	status->sceneNumber++;
	status->sceneProgress = SCENE_UNINITIALISED;
}

void endOfFrameTransition(SceneDef scene, ScriptStatus *status) {
	switch(scene.type) {
		case SCENE_CUE:
			goToNextScene(scene.type, status);
			break;
		case SCENE_LOOP:
			//Initialised, go to fade in or loop.
			if(status->sceneProgress == SCENE_UNINITIALISED) {
				if((scene.fadeMode&FADE_IN) > 0) {
					status->sceneProgress = SCENE_FADE_IN;
					fadeIn();
				}else{
					status->sceneProgress = SCENE_LOOPING;
				}
			//Faded in, go to loop.
			}else if(status->sceneProgress == SCENE_FADE_IN && !isFading()) {
				status->sceneProgress = SCENE_LOOPING;

			//Loop finished, go to fade out or next frame.
			} else if(due(status->sceneStartTime, scene.duration)) {
				//Begin fade out.
				if(status->sceneProgress != SCENE_FADE_OUT && (scene.fadeMode&FADE_OUT) > 0) {
					fadeOut();
					status->sceneProgress = SCENE_FADE_OUT;
					return;

				//Still fading? Wait until we're done before changing scenes.
				}else if(isFading()) {
					return;
				}

				goToNextScene(scene.type, status);
			}
			break;
		case SCENE_INFINITE:
			//Keep cycling 'till we manually trigger a state change.
			break;
	}
}

void requestStateChange(GameState newState) {
	//This is to cleanly reset our script status before the actual game state change.
	scriptStatus.scriptStarted = false;
	triggerState(STATE_TITLE);
}

void scriptGameFrame() {
	if(!scriptStatus.scriptStarted) {
		scriptStatus.sceneNumber = 0;
		scriptStatus.scriptStarted = true;
	}

	//Reset scene timer to current time for best accuracy.
	if(!sceneInitialised()) {
		scriptStatus.sceneStartTime = clock();
	}

	switch(gameState) {
		case STATE_INTRO:
			//Skip to titlescreen if fire button pressed.
			if(checkCommand(CMD_PLAYER_SKIP_CUTSCENE)) {
				requestStateChange(STATE_TITLE);
			}

			switch(scriptStatus.sceneNumber) {
				case INTRO_CUE:
					resetPlayer();
					resetEnemies();
					resetBackground();
					showMike = false;
					staticBackground = true;
					intro_StrafeInc = 0;
					intro_mikeStafeDir = -1;
					break;

				case INTRO_BATTLE_CUE:
					//Spawn random assortment of enemies.
					spawnEnemy(80,  random(40, 65), random(0, sizeof(EnemyType)));
					spawnEnemy(110, random(40, 65), random(0, sizeof(EnemyType)));
					spawnEnemy(140, random(40, 65), random(0, sizeof(EnemyType)));
					playerOrigin.y = screenBounds.y + 16;
					showMike = true;
					showBackground = false;
					break;
				case INTRO_BATTLE_MIKE_ENTER:
					scriptCommand(CMD_PLAYER_UP);
					break;
				case INTRO_BATTLE_MIKE_FIRE:
					scriptCommand(CMD_PLAYER_FIRE);

					//Strafe left and right.
					if(intro_mikeStafeDir < 0) {
						scriptCommand(CMD_PLAYER_LEFT);
					}else{
						scriptCommand(CMD_PLAYER_RIGHT);
					}
					if(playerOrigin.x < (screenBounds.x/2) - 9) {
						intro_mikeStafeDir = 1;
					}else if(playerOrigin.x > (screenBounds.x/2) + 9){
						intro_mikeStafeDir = -1;
					}

					break;
				case INTRO_BATTLE_MIKE_DEPART:
					playerOrigin.y -= 4.0;
					break;
				case INTRO_TITLE_CUE:
					requestStateChange(STATE_TITLE);
					break;
			}
			break;

		case STATE_TITLE:
			switch(scriptStatus.sceneNumber) {
				case 0:
					//General initialisation
					resetPlayer();
					resetEnemies();
					resetBackground();
					showMike = true;
					staticBackground = true;
					game_messageTime = clock();
					title_logoLocation = makeCoord((screenBounds.x/2) - 3, screenBounds.y/4);
					title_logoSprite = makeSprite(getTexture("title.png"), zeroCoord(), SDL_FLIP_NONE);
					showBackground = true;

					//Enemy roll call.
					int spacer = -10;
					EnemyType roll[] = { ENEMY_BUG, ENEMY_DISK, ENEMY_VIRUS, ENEMY_DISK_BLUE, ENEMY_CD };
					for(int i=0; i < sizeof(roll) / sizeof(EnemyType); i++) {
						spawnEnemy(spacer += 40, 135, roll[i]);
					}
					break;
				case 1:
					//Begin game when fire button is pressed.
					if(checkCommand(CMD_PLAYER_FIRE)) {
						requestStateChange(STATE_GAME);
					}

				case 2:
					//Loop back to intro.
					requestStateChange(STATE_INTRO);
					break;
			}
			break;

		case STATE_GAME:
			if(!sceneInitialised()) {
				resetPlayer();
				resetEnemies();
				resetBackground();
				showMike = true;
			}
			break;

	}

	endOfFrameTransition(intro_script[scriptStatus.sceneNumber], &scriptStatus);
}

void superRenderFrame() {
	//Main blue background.
	Sprite streak = makeSprite(getTexture("super-streak.png"), zeroCoord(), SDL_FLIP_NONE);
	drawSpriteAbs(streak, makeCoord(screenBounds.x/2, screenBounds.y/2));

	//Spawn a bunch of random white streaks to give the impression of fast motion.
	Sprite fleck = makeSprite(getTexture("super-fleck.png"), zeroCoord(), SDL_FLIP_NONE);
	for(int i=0; i < 16; i++) {
		drawSpriteAbs(
			fleck,
			makeCoord(
				random((screenBounds.x/2) - 55, (screenBounds.x/2) + 56),
				random(0, screenBounds.y)
			)
		);
	}
}

void scriptRenderFrame() {

	//Wait until the current state has initialised before attempting to render.
	if(!sceneInitialised()) return;

	switch(gameState) {
		case STATE_INTRO:
			//Use super background for the battle scenes only.
			if(scriptStatus.sceneNumber > INTRO_BATTLE_CUE && scriptStatus.sceneNumber < INTRO_TITLE_CUE) {
				superRenderFrame();
			}

			switch(scriptStatus.sceneNumber) {
				//Show "Les Miskin presents"
				case INTRO_LOGO: {
					Sprite presents = makeSprite(getTexture("lm-presents.png"), zeroCoord(), SDL_FLIP_NONE);
					drawSpriteAbs(presents, makeCoord(screenBounds.x/2 - 3, 98));
					break;
				}
				//Add special blue trailing shadows to Mike as he flies off.
				case INTRO_BATTLE_MIKE_DEPART: {
					for(int i = 0; i < playerOrigin.y + 10; i += 8) {
						SDL_Texture *superTex = getTextureVersion("mike-01.png", ASSET_SUPER);
						Sprite superSprite = makeSprite(superTex, zeroCoord(), SDL_FLIP_NONE);
						drawSpriteAbs(superSprite, deriveCoord(playerOrigin, 0, i));
					}
					break;
				}
			}

			break;

		case STATE_TITLE:
			switch(scriptStatus.sceneNumber) {
				case 1:
					//Draw the game logo.
					drawSpriteAbs(title_logoSprite, title_logoLocation);
					break;
			}
			break;

		case STATE_GAME:
			//Show level entry message.
			if(game_showLevelMessage) {
				if(!timer(&game_messageTime, toMilliseconds(GAME_MESSAGE_DURATION))) {
					Sprite levelSprite = makeSprite(getTexture("level-1.png"), zeroCoord(), SDL_FLIP_NONE);
					drawSpriteAbs(levelSprite, makeCoord((screenBounds.x / 2) - 3, 100));
				} else {
					game_showLevelMessage = false;
				}
			}
			break;
	}
}

void initScript(void) {
	//Introduction script.
	intro_script[INTRO_CUE] = 					newCueStep();
	intro_script[INTRO_LOGO] = 					newTimedStep(SCENE_LOOP, 3000, FADE_BOTH);
	intro_script[INTRO_BATTLE_CUE] = 			newCueStep();
	intro_script[INTRO_BATTLE_PRELUDE] = 		newTimedStep(SCENE_LOOP, 2500, FADE_IN);
	intro_script[INTRO_BATTLE_MIKE_ENTER] = 	newTimedStep(SCENE_LOOP, 200, FADE_NONE);
	intro_script[INTRO_BATTLE_MIKE_PAUSE] = 	newTimedStep(SCENE_LOOP, 2000, FADE_NONE);
	intro_script[INTRO_BATTLE_MIKE_FIRE] = 		newTimedStep(SCENE_LOOP, 1360, FADE_NONE);
	intro_script[INTRO_BATTLE_MIKE_GLOAT] = 	newTimedStep(SCENE_LOOP, 1500, FADE_NONE);
	intro_script[INTRO_BATTLE_MIKE_DEPART] = 	newTimedStep(SCENE_LOOP, 1500, FADE_OUT);
	intro_script[INTRO_TITLE_CUE] = 			newCueStep();

	title_script[TITLE_CUE] = 					newCueStep();
	intro_script[TITLE_LOOP] = 					newTimedStep(SCENE_LOOP, FADE_BOTH, 10000);
	intro_script[TITLE_INTRO_CUE] = 			newCueStep();
}
