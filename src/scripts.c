#include <time.h>
#include <stdlib.h>
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
#include "level.h"

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
	COIN_CUE,
	COIN_PLAY,
	COIN_END_CUE
} CoinCues;

typedef enum {
	END_PAUSE_CUE,
	END_PAUSE,
	END_SMILE_CUE,
	END_SMILE,
	END_WARP_CUE,
	END_WARP,
	END_FINISH
} EndCues;

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

void scriptGameFrame() {

	if(!stateInitialised) {
		resetScriptStatus();
		stateInitialised = true;
	}

	switch(gameState) {
		case STATE_LEVEL_COMPLETE:
		case STATE_END_GAME:
			switch(scriptStatus.sceneNumber){
				case END_PAUSE_CUE:
					pain = false;	// make sure Mike is visible during end sequence.
					Mix_PauseMusic();
					play("intro-presents.wav");
					break;
				case END_SMILE_CUE:
					smile();
					break;
				case END_WARP_CUE:
					play("warp.wav");
					break;
				case END_WARP:
					playerOrigin.y -= 4.0;
					break;
			}
			break;

		case STATE_GAME_OVER:
			//Skip to titlescreen if fire button pressed.
			if(checkCommand(CMD_PLAYER_SKIP_TO_TITLE)) {
				triggerState(STATE_TITLE);
			}
			break;

		case STATE_COIN:
			switch(scriptStatus.sceneNumber) {
				case COIN_CUE:
					resetEnemies();
					useMike = false;
					insertCoin();
					break;
				case COIN_PLAY:
					break;
				case COIN_END_CUE:
					triggerState(STATE_GAME);
					break;
			}
			break;

		case STATE_INTRO:
			// Insert coin if fire button is pressed.
			if(checkCommand(CMD_PLAYER_SKIP_TO_TITLE)) {
				triggerState(STATE_COIN);
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
					// Spawn randomMq assortment of enemies.
					// NB: 0-5 enemy index excludes the boss (6).
					spawnEnemy(80, randomMq(40, 65), (EnemyType)randomMq(0, 5), PATTERN_NONE, COMBAT_IDLE, 0, 0, 0, HEALTH_LIGHT);
					spawnEnemy(110, randomMq(40, 65), (EnemyType)randomMq(0, 5), PATTERN_NONE, COMBAT_IDLE, 0, 0, 0, HEALTH_LIGHT);
					spawnEnemy(140, randomMq(40, 65), (EnemyType)randomMq(0, 5), PATTERN_NONE, COMBAT_IDLE, 0, 0, 0, HEALTH_LIGHT);
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
					resetHud(false);
					useMike = true;
//					staticBackground = true;
					game_messageTime = clock();
					title_logoLocation = makeCoord((screenBounds.x/2) - 3, screenBounds.y/4);
					title_logoSprite = makeSprite(getTexture("title.png"), zeroCoord(), SDL_FLIP_NONE);
					showBackground = true;
					waveCompleteOn = false;
					level = 0;

					//Enemy roll call.
					int spacer = -10;
					EnemyType roll[] = { ENEMY_BUG, ENEMY_DISK, ENEMY_VIRUS, ENEMY_MAGNET, ENEMY_CD };
					for(int i=0; i < sizeof(roll) / sizeof(EnemyType); i++) {
						spawnEnemy(spacer += 40, 135, roll[i], PATTERN_CIRCLE, COMBAT_IDLE, 0, 0, 0, HEALTH_LIGHT);
					}

					playMusic("title.ogg", 1);
					break;
				case TITLE_LOOP:
					//Begin game when fire button is pressed.
					if(checkCommand(CMD_PLAYER_FIRE)) {
//						insertCoin();
						triggerState(STATE_COIN);
					}
					break;
			}
			break;

		case STATE_GAME:
			if(!sceneInitialised()) {
				
				// Remember that we may have come in from a level change, so reset coins (but not score!)
				resetHud(true);
				game_messageTime = clock();
				resetPlayer();
				resetEnemies();
				resetBackground();
				resetItems();
				
				resetLevel();
				levelInit();

				level++;

				waveCompleteOn = false;
				useMike = true;
				playMusic("level-01c.ogg", -1);
			}
			
			//Skip to titlescreen if fire button pressed.
			if(checkCommand(CMD_PLAYER_SKIP_TO_TITLE)) {
				triggerState(STATE_TITLE);
			}
			break;
	}

	endOfFrameTransition();
}

void superFrame() {
	for(int i = 0; i < playerOrigin.y + 10; i += 8) {
		SDL_Texture *superTex = getTextureVersion("mike-01.png", ASSET_SUPER);
		Sprite superSprite = makeSprite(superTex, zeroCoord(), SDL_FLIP_NONE);
		drawSpriteAbs(superSprite, deriveCoord(playerOrigin, 0, i));
	}
}

void scriptRenderFrame() {

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
					triggerState(STATE_TITLE);
					break;
			}
			break;
		}
		
		case STATE_END_GAME: {
			switch(scriptStatus.sceneNumber) {
				case END_WARP:
					superFrame();
					break;
			}
			break;
		}
		case STATE_LEVEL_COMPLETE: {
			switch(scriptStatus.sceneNumber) {
				case END_WARP:
					superFrame();
					break;
			}

			// Show LEVEL COMPLETE message
			waveCompleteOn = true;

			break;
		}
		case STATE_INTRO:
			switch(scriptStatus.sceneNumber) {
				//Show "Les Miskin presents"
				case INTRO_LOGO: {
					Sprite presents = makeSprite(getTexture("lm-presents.png"), zeroCoord(), SDL_FLIP_NONE);
					drawSpriteAbs(presents, /*step*/makeCoord(screenBounds.x/2 - 3, 98));
					break;
				}
				//Add special blue trailing shadows to Mike as he flies off.
				case INTRO_BATTLE_MIKE_DEPART: {
					superFrame();
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

void initScripts() {
	Script intro, title, game, gameOver, coin, level, end;

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

	game.scenes[0] = 								newTimedStep(SCENE_INFINITE, 0, FADE_IN);
	game.totalScenes = 1;
	scripts[STATE_GAME] = game;

	gameOver.scenes[0] = 							newTimedStep(SCENE_LOOP, 10000, FADE_OUT);
	gameOver.scenes[1] = 							newCueStep();
	gameOver.totalScenes = 2;
	scripts[STATE_GAME_OVER] = gameOver;

	coin.scenes[COIN_CUE] = 						newCueStep();
	coin.scenes[COIN_PLAY] = 						newTimedStep(SCENE_LOOP, 1650, FADE_NONE);
	coin.scenes[COIN_END_CUE] = 					newCueStep();
	coin.totalScenes = 3;
	scripts[STATE_COIN] = coin;
	
	level.scenes[END_PAUSE_CUE] = 					newCueStep();
	level.scenes[END_PAUSE] = 						newTimedStep(SCENE_LOOP, 1000, FADE_NONE);
	level.scenes[END_SMILE_CUE] = 					newCueStep();
	level.scenes[END_SMILE] = 						newTimedStep(SCENE_LOOP, 1000, FADE_NONE);
	level.scenes[END_WARP_CUE] = 					newCueStep();
	level.scenes[END_WARP] = 						newTimedStep(SCENE_LOOP, 1250, FADE_OUT);
	level.scenes[END_FINISH] = 						newStateStep(STATE_GAME);
	level.totalScenes = 7;
	scripts[STATE_LEVEL_COMPLETE] = level;
	
	end.scenes[END_PAUSE_CUE] = 					newCueStep();
	end.scenes[END_PAUSE] = 						newTimedStep(SCENE_LOOP, 1000, FADE_NONE);
	end.scenes[END_SMILE_CUE] = 					newCueStep();
	end.scenes[END_SMILE] = 						newTimedStep(SCENE_LOOP, 1000, FADE_NONE);
	end.scenes[END_WARP_CUE] = 						newCueStep();
	end.scenes[END_WARP] = 							newTimedStep(SCENE_LOOP, 1250, FADE_OUT);
	end.scenes[END_FINISH] = 						newStateStep(STATE_TITLE);
	end.totalScenes = 7;
	scripts[STATE_END_GAME] = end;
}
