#ifndef INPUT_H
#define INPUT_H

typedef enum {
	CMD_QUIT = 0,
	CMD_PLAYER_UP = 1,
	CMD_PLAYER_DOWN = 2,
	CMD_PLAYER_LEFT = 3,
	CMD_PLAYER_RIGHT = 4,
	CMD_PLAYER_FIRE = 5,
	CMD_PLAYER_BOMB = 6,
	CMD_PLAYER_SKIP_TO_TITLE = 7,
	CMD_MUSIC_TOGGLE = 8
} Command;

extern void initInput(void);
extern void pollInput(void);
extern void processSystemCommands(void);
extern bool checkCommand(int commandFlag);
extern void command(int commandFlag);
extern void scriptCommand(int commandFlag);
extern SDL_Haptic* haptic;
extern bool hapticSupported;

#endif
