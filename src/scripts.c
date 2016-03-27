#include <time.h>
#include "renderer.h"
#include "assets.h"
#include "input.h"
#include "player.h"
#include "enemy.h"
#include "background.h"
#include "input.h"
#include "scripting.h"
#include "weapon.h"
#include "item.h"
#include "hud.h"
#include "sound.h"

//BUG: Opaque PCB edges *sometimes* (something to do with faders?)
//TODO: There's no fading when entering the game(?)
//TODO: When pressing [FIRE], fade out the current scene first, don't jump to the next.
//TODO: Move Mike's Titlescreen speech bubble to this file.
//TODO: Move Mike's level start 'here I go' speech bubble to this file.
//TODO: Should we queue up the 'desired' state change, so we can fade out from GAME to TITLE?

//Script-specific vars.
static char intro_mikeStafeDir;

static Sprite title_logoSprite;
static Coord title_logoLocation;

static bool game_showLevelMessage;
static const double GAME_MESSAGE_DURATION = 1.5;
static long game_messageTime;

typedef enum {
	TITLE_CUE,
	TITLE_LOOP,
	TITLE_INTRO_CUE
} TitleCues;

typedef enum {
	INTRO_CUE,
	INTRO_LOGO,
	INTRO_BATTLE_CUE,
	INTRO_BATTLE_PRELUDE,
	INTRO_BATTLE_MIKE_ENTER,
	INTRO_BATTLE_MIKE_PAUSE,
	INTRO_BATTLE_MIKE_SMILING_CUE,
	INTRO_BATTLE_MIKE_SMILING,
	INTRO_BATTLE_MIKE_FIRE,
	INTRO_BATTLE_MIKE_GLOAT_CUE,
	INTRO_BATTLE_MIKE_GLOAT,
	INTRO_BATTLE_MIKE_DEPART_CUE,
	INTRO_BATTLE_MIKE_DEPART,
	INTRO_TITLE_CUE
} IntroCues;

void scriptGameFrame(void) {

	if(!stateInitialised) {
		resetScriptStatus();
		stateInitialised = true;
	}

	switch(gameState) {
		case STATE_GAME_OVER:
			//Skip to titlescreen if fire button pressed.
			if(checkCommand(CMD_PLAYER_SKIP_TO_TITLE)) {
				fadeState(STATE_TITLE);
			}
			break;

		case STATE_INTRO:
			//Skip to titlescreen if fire button pressed.
			if(checkCommand(CMD_PLAYER_SKIP_TO_TITLE)) {
				fadeState(STATE_TITLE);
			}

			switch(scriptStatus.sceneNumber) {
				case INTRO_CUE:
					resetPlayer();
					resetEnemies();
					resetBackground();
					useMike = false;
//					staticBackground = true;
					intro_mikeStafeDir = -1;
					play("intro-presents.wav");
					break;

				case INTRO_BATTLE_CUE:
					//Spawn randomMq assortment of enemies.
					spawnEnemy(80,  randomMq(40, 65), randomMq(0, sizeof(EnemyType)), MOVEMENT_STRAIGHT, COMBAT_IDLE, 0, 0);
					spawnEnemy(110, randomMq(40, 65), randomMq(0, sizeof(EnemyType)), MOVEMENT_STRAIGHT, COMBAT_IDLE, 0, 0);
					spawnEnemy(140, randomMq(40, 65), randomMq(0, sizeof(EnemyType)), MOVEMENT_STRAIGHT, COMBAT_IDLE, 0, 0);
					playerOrigin.y = screenBounds.y + 16;
					useMike = true;
					playMusic("intro-battle-3.ogg", 1);
					break;
				case INTRO_BATTLE_MIKE_ENTER:
					scriptCommand(CMD_PLAYER_UP);
					break;
				case INTRO_BATTLE_MIKE_SMILING_CUE:
					smile();
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
				case INTRO_BATTLE_MIKE_DEPART_CUE:
					play("warp.wav");
					break;
				case INTRO_BATTLE_MIKE_DEPART:
					playerOrigin.y -= 4.0;
					break;
			}
			break;

		case STATE_TITLE:
			switch(scriptStatus.sceneNumber) {
				case TITLE_CUE:
					//General initialisation
					resetPew();
					resetPlayer();
					resetEnemies();
					resetBackground();
					resetItems();
					resetHud();
					useMike = true;
//					staticBackground = true;
					game_messageTime = clock();
					title_logoLocation = makeCoord((screenBounds.x/2) - 3, screenBounds.y/4);
					title_logoSprite = makeSprite(getTexture("title.png"), zeroCoord(), SDL_FLIP_NONE);
					showBackground = true;

					//Enemy roll call.
					int spacer = -10;
					EnemyType roll[] = { ENEMY_BUG, ENEMY_DISK, ENEMY_VIRUS, ENEMY_MAGNET, ENEMY_CD };
					for(int i=0; i < sizeof(roll) / sizeof(EnemyType); i++) {
						spawnEnemy(spacer += 40, 135, roll[i], MOVEMENT_CIRCLE, COMBAT_IDLE, 0, 0);
					}

					playMusic("title.ogg", 1);
					break;
				case TITLE_LOOP:
					//Begin game when fire button is pressed.
					if(checkCommand(CMD_PLAYER_FIRE)) {
						fadeState(STATE_GAME);
					}
					break;
			}
			break;

		case STATE_GAME:
			if(!sceneInitialised()) {
				resetPlayer();
				resetEnemies();
				resetBackground();
				resetItems();
				useMike = true;
				playMusic("level-01c.ogg", -1);
			}
			//Skip to titlescreen if fire button pressed.
			if(checkCommand(CMD_PLAYER_SKIP_TO_TITLE)) {
				fadeState(STATE_TITLE);
			}
			break;

	}

	endOfFrameTransition();
}

void superRenderFrame(void) {
	//Main blue background.
//	Sprite streak = makeSprite(getTexture("super-streak.png"), zeroCoord(), SDL_FLIP_NONE);
//	drawSpriteAbs(streak, makeCoord(screenBounds.x/2, screenBounds.y/2));
//
//	//Spawn a bunch of randomMq white streaks to give the impression of fast motion.
//	Sprite fleck = makeSprite(getTexture("super-fleck.png"), zeroCoord(), SDL_FLIP_NONE);
//	for(int i=0; i < 16; i++) {
//		drawSpriteAbs(
//			fleck,
//			makeCoord(
//				randomMq((screenBounds.x/2) - 55, (screenBounds.x/2) + 56),
//				randomMq(0, screenBounds.y)
//			)
//		);
//	}
}

void scriptRenderFrame(void) {

	//Only ever ensure we process a scripted render frame if the script itself has been initialised.
	// e.g. If we got here from a user-initiated state change which has not yet been initialised, then skip
	// rendering until this has taken place. We may rely on game-specific stuff to do our rendering, such as
	// logo loading.

	if(!stateInitialised) return;

	switch(gameState) {
		case STATE_GAME_OVER: {
			switch(scriptStatus.sceneNumber) {
				case 0: {
					Sprite gameOver = makeSprite(getTexture("game-over.png"), zeroCoord(), SDL_FLIP_NONE);
					drawSpriteAbs(gameOver, makeCoord(screenBounds.x/2 - 3, 98));
					break;
				}
				case 1:
					fadeState(STATE_TITLE);
					break;
			}
			break;
		}
		case STATE_INTRO:
			//Use super background for the battle scenes only.
			if(scriptStatus.sceneNumber > INTRO_BATTLE_CUE && scriptStatus.sceneNumber < INTRO_TITLE_CUE) {
				superRenderFrame();
			}

			switch(scriptStatus.sceneNumber) {
				//Show "Les Miskin presents"
				case INTRO_LOGO: {
					Sprite presents = makeSprite(getTexture("lm-presents.png"), zeroCoord(), SDL_FLIP_NONE);
					drawSpriteAbs(presents, /*step*/makeCoord(screenBounds.x/2 - 3, 98));
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
				case TITLE_LOOP:
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

void initScripts(void) {
	Script intro, title, game, gameOver;

	//Introduction script.
	intro.scenes[INTRO_CUE] = 						newCueStep();
	intro.scenes[INTRO_LOGO] = 						newTimedStep(SCENE_LOOP, 2000, FADE_BOTH);
	intro.scenes[INTRO_BATTLE_CUE] = 				newCueStep();
	intro.scenes[INTRO_BATTLE_PRELUDE] = 			newTimedStep(SCENE_LOOP, 3000, FADE_IN);
	intro.scenes[INTRO_BATTLE_MIKE_ENTER] = 		newTimedStep(SCENE_LOOP, 200, FADE_NONE);
	intro.scenes[INTRO_BATTLE_MIKE_PAUSE] = 		newTimedStep(SCENE_LOOP, 1000, FADE_NONE);
	intro.scenes[INTRO_BATTLE_MIKE_SMILING_CUE] = 	newCueStep();
	intro.scenes[INTRO_BATTLE_MIKE_SMILING] = 		newTimedStep(SCENE_LOOP, 1150, FADE_NONE);
	intro.scenes[INTRO_BATTLE_MIKE_FIRE] = 			newTimedStep(SCENE_LOOP, 1600, FADE_NONE);
	intro.scenes[INTRO_BATTLE_MIKE_GLOAT_CUE] =		newCueStep();
	intro.scenes[INTRO_BATTLE_MIKE_GLOAT] = 		newTimedStep(SCENE_LOOP, 1600, FADE_NONE);
	intro.scenes[INTRO_BATTLE_MIKE_DEPART_CUE] = 	newCueStep();
	intro.scenes[INTRO_BATTLE_MIKE_DEPART] = 		newTimedStep(SCENE_LOOP, 800, FADE_OUT);
	intro.scenes[INTRO_TITLE_CUE] = 				newStateStep(STATE_TITLE);
	intro.totalScenes = INTRO_TITLE_CUE+1;
	scripts[STATE_INTRO] = intro;

	title.scenes[TITLE_CUE] = 						newCueStep();
	title.scenes[TITLE_LOOP] = 						newTimedStep(SCENE_LOOP, 7500, FADE_BOTH);
	title.scenes[TITLE_INTRO_CUE] = 				newStateStep(STATE_INTRO);
	title.totalScenes = TITLE_INTRO_CUE+1;
	scripts[STATE_TITLE] = title;

	game.scenes[0] = 								newTimedStep(SCENE_INFINITE, 0, FADE_BOTH);
	game.totalScenes = 1;
	scripts[STATE_GAME] = game;

	gameOver.scenes[0] = 							newTimedStep(SCENE_LOOP, 10000, FADE_OUT);
	gameOver.scenes[1] = 							newCueStep();
	gameOver.totalScenes = 2;
	scripts[STATE_GAME_OVER] = gameOver;
}
