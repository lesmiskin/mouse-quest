#include <stdbool.h>
#include "mysdl.h"
#include "input.h"
#include "common.h"
#include "sound.h"
#include "weapon.h"
#include "player.h"
#include "renderer.h"
#include "myc.h"

//NB: We bind our SDL key codes to game-meaningful actions, so we can bind our logic against those rather than hard-
// coding keys throughout our code.

#define MAX_COMMANDS 20

static bool commands[MAX_COMMANDS];
static bool scriptCommands[MAX_COMMANDS];

void scriptCommand(int commandFlag) {
	scriptCommands[commandFlag] = true;
}

bool checkCommand(int commandFlag) {
	return commands[commandFlag];
}

void shutdownInput() {
}

void initInput() {
}

void pollInput() {
	//Tell SDL we want to examine events (otherwise getKeyboardState won't work).
	SDL_PumpEvents();

	//Respond to held keys
	const Uint8 *keysHeld = SDL_GetKeyboardState(NULL);

	//We're on a new frame, so clear all previous checkCommand (not key) states (i.e. set to false)
	memset(commands, 0, sizeof(commands));

	//Respond to SDL events, or key presses (not holds)
	SDL_Event event;
	while(SDL_PollEvent(&event) != 0) {
		switch(event.type) {
			case SDL_QUIT:
				commands[CMD_QUIT] = true;
				break;

			//Presses
			case SDL_KEYDOWN: {
				//Ignore held keys.
				if(event.key.repeat) break;

				SDL_Keycode keypress = event.key.keysym.scancode;

				switch(keypress) {
					case SDL_SCANCODE_F11:
						toggleFullscreen();
						break;
//					case SDL_SCANCODE_F10:
//						toggleMusic();
//						break;
					// Activate fire-on-space only after we've switched modes, to prevent too-soon firing on game start.
					case SDL_SCANCODE_SPACE:
					case SDL_SCANCODE_LCTRL:
						if(gameState == STATE_GAME) {
							canFireInLevel = true;
						}
						break;
				}

				//Bind SDL keycodes to our custom actions, so we don't have to duplicate/remember keybindings everywhere in our code.
				switch(gameState) {
					case STATE_COIN:
						if(keypress == SDL_SCANCODE_ESCAPE)
							triggerState(STATE_TITLE);
						break;
					case STATE_TITLE:
						if(	keypress == SDL_SCANCODE_LCTRL ||
							keypress == SDL_SCANCODE_SPACE
						)
							commands[CMD_PLAYER_FIRE] = true;

						if(keypress == SDL_SCANCODE_ESCAPE)
							commands[CMD_QUIT] = true;
						break;
					case STATE_INTRO:
						if(	keypress == SDL_SCANCODE_LCTRL ||
							keypress == SDL_SCANCODE_SPACE ||
							keypress == SDL_SCANCODE_ESCAPE)
							commands[CMD_PLAYER_SKIP_TO_TITLE] = true;
 						break;
					case STATE_GAME_OVER:
						if(	keypress == SDL_SCANCODE_ESCAPE ||
							keypress == SDL_SCANCODE_SPACE ||
							keypress == SDL_SCANCODE_LCTRL )
							commands[CMD_PLAYER_SKIP_TO_TITLE] = true;
						break;
					case STATE_GAME:
#ifdef DEBUG_CHEATS
						if(	keypress == SDL_SCANCODE_G)
							godMode = !godMode;

						if(	keypress == SDL_SCANCODE_H) {
							if(atMaxWeapon()) {
								changeWeapon(0);
							}else{
								upgradeWeapon();
							}
						}

						if(	keypress == SDL_SCANCODE_J){
							if(playerHealth > 1) {
								playerHealth = 1;
							}else{
								playerHealth = playerStrength;
							}
						}
#endif
						if(	keypress == SDL_SCANCODE_ESCAPE)
							commands[CMD_PLAYER_SKIP_TO_TITLE] = true;
						break;
				}
				break;
			}
		}
	}

	//Respond to held keys.
	switch(gameState) {
		case STATE_INTRO:
			if(scriptCommands[CMD_PLAYER_LEFT])
				commands[CMD_PLAYER_LEFT] = true;
			else if(scriptCommands[CMD_PLAYER_RIGHT])
				commands[CMD_PLAYER_RIGHT] = true;

			if(scriptCommands[CMD_PLAYER_UP])
				commands[CMD_PLAYER_UP] = true;
			else if(scriptCommands[CMD_PLAYER_DOWN])
				commands[CMD_PLAYER_DOWN] = true;

			if(scriptCommands[CMD_PLAYER_FIRE])
				commands[CMD_PLAYER_FIRE] = true;
			break;
		case STATE_GAME:
			if(keysHeld[SDL_SCANCODE_LEFT])
				commands[CMD_PLAYER_LEFT] = true;
			else if(keysHeld[SDL_SCANCODE_RIGHT])
				commands[CMD_PLAYER_RIGHT] = true;

			if(keysHeld[SDL_SCANCODE_UP])
				commands[CMD_PLAYER_UP] = true;
			else if(keysHeld[SDL_SCANCODE_DOWN])
				commands[CMD_PLAYER_DOWN] = true;

			if(keysHeld[SDL_SCANCODE_LCTRL] ||
			   keysHeld[SDL_SCANCODE_SPACE])
				commands[CMD_PLAYER_FIRE] = true;

			break;
	}

	memset(scriptCommands, 0, sizeof(scriptCommands));
}

void processSystemCommands() {
	if(checkCommand(CMD_QUIT)) quit();
}