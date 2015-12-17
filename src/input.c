#include <stdbool.h>
#include "SDL.h"
#include "input.h"
#include "common.h"
#include "weapon.h"
#include "player.h"
#include "renderer.h"

//NB: We bind our SDL key codes to game-meaningful actions, so we can bind our logic against those rather than hard-
// coding keys throughout our code.

#define MAX_COMMANDS 20

SDL_Haptic* haptic = NULL;
bool hapticSupported = false;

static bool commands[MAX_COMMANDS];
static bool scriptCommands[MAX_COMMANDS];
static SDL_Joystick* joystick = NULL;
static bool usingJoystick = false;
static const int JOYSTICK_DEADZONE = 8000;

void scriptCommand(int commandFlag) {
	scriptCommands[commandFlag] = true;
}

bool checkCommand(int commandFlag) {
	return commands[commandFlag];
}

void shutdownInput(void) {
	if(usingJoystick) {
		SDL_HapticClose(haptic);
		SDL_JoystickClose(joystick);
		joystick = NULL;
	}
}

void initInput(void) {
	if(SDL_NumJoysticks() > 0) {
		usingJoystick = true;
		joystick = SDL_JoystickOpen(0);
		haptic = SDL_HapticOpenFromJoystick(joystick);

		if(SDL_HapticRumbleSupported(haptic)) {
			hapticSupported = true;
			SDL_HapticRumbleInit(haptic);
		}
	}
}

typedef enum {
	PS3_DPAD_UP = 4,
	PS3_DPAD_RIGHT = 5,
	PS3_DPAD_DOWN = 6,
	PS3_DPAD_LEFT = 7,
	PS3_BUTTON_X = 14,
	PS3_BUTTON_PS = 16,
	PS3_BUTTON_START = 3
} Ps3Buttons;

void pollInput(void) {
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
			case SDL_JOYBUTTONDOWN: {
				switch(gameState) {
					case STATE_INTRO:
						if(event.jbutton.button == PS3_BUTTON_X)
							commands[CMD_PLAYER_SKIP_TO_TITLE] = true;
						break;
					case STATE_TITLE:
						if(event.jbutton.button == PS3_BUTTON_X)
							commands[CMD_PLAYER_FIRE] = true;
						else if(event.jbutton.button == PS3_BUTTON_PS)
							commands[CMD_QUIT] = true;
						break;
					case STATE_GAME:
						if(event.jbutton.button == PS3_BUTTON_PS)
							commands[CMD_PLAYER_SKIP_TO_TITLE] = true;
						break;
				}
				break;
			}
			case SDL_KEYDOWN: {
				//Ignore held keys.
				if(event.key.repeat) break;

				SDL_Keycode keypress = event.key.keysym.scancode;

				if(keypress == SDL_SCANCODE_F11) {
					toggleFullscreen();
				}

				//Bind SDL keycodes to our custom actions, so we don't have to duplicate/remember keybindings everywhere in our code.
				switch(gameState) {
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
						if(	keypress == SDL_SCANCODE_G){
							godMode = !godMode;
						}
						if(	keypress == SDL_SCANCODE_H)
							weaponInc = weaponInc == MAX_WEAPONS-1 ? 0 : weaponInc + 1;

						if(	keypress == SDL_SCANCODE_J){
							if(playerHealth > 1) {
								playerHealth = 1;
							}else{
								playerHealth = playerStrength;
							}
						}

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
			if(		keysHeld[SDL_SCANCODE_LEFT] ||
					SDL_JoystickGetAxis(joystick, 0) < -JOYSTICK_DEADZONE ||
					SDL_JoystickGetButton(joystick, PS3_DPAD_LEFT))
				commands[CMD_PLAYER_LEFT] = true;
			else if(keysHeld[SDL_SCANCODE_RIGHT] ||
					SDL_JoystickGetAxis(joystick, 0) > JOYSTICK_DEADZONE ||
					SDL_JoystickGetButton(joystick, PS3_DPAD_RIGHT))
				commands[CMD_PLAYER_RIGHT] = true;

			if(		keysHeld[SDL_SCANCODE_UP] ||
					SDL_JoystickGetAxis(joystick, 1) < -JOYSTICK_DEADZONE ||
					SDL_JoystickGetButton(joystick, PS3_DPAD_UP))
				commands[CMD_PLAYER_UP] = true;
			else if(
					keysHeld[SDL_SCANCODE_DOWN] ||
					SDL_JoystickGetAxis(joystick, 1) > JOYSTICK_DEADZONE ||
					SDL_JoystickGetButton(joystick, PS3_DPAD_DOWN))
				commands[CMD_PLAYER_DOWN] = true;

			if(		keysHeld[SDL_SCANCODE_LCTRL] ||
					keysHeld[SDL_SCANCODE_SPACE] ||
					SDL_JoystickGetButton(joystick, PS3_BUTTON_X))
				commands[CMD_PLAYER_FIRE] = true;

			if(SDL_JoystickGetButton(joystick, PS3_BUTTON_START))
				commands[CMD_PLAYER_SKIP_TO_TITLE] = true;
			break;
	}

	memset(scriptCommands, 0, sizeof(scriptCommands));
}

void processSystemCommands(void) {
	if(checkCommand(CMD_QUIT)) quit();
}