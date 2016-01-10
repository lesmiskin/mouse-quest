#include <time.h>
#include <assert.h>
#include "SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_mixer.h"
#include "common.h"
#include "assets.h"
#include "renderer.h"
#include "player.h"
#include "input.h"
#include "background.h"
#include "weapon.h"
#include "enemy.h"
#include "scripting.h"
#include "scripts.h"
#include "hud.h"
#include "item.h"

static const char *GAME_TITLE = "Mouse Quest";

#ifdef DEBUG_WINDOW_T500
	static const bool FULLSCREEN = false;
#else
#endif

bool running = true;

static void initSDL(void) {
	//Kick off the minor subsystems first, so they don't slow down the game before
	// it's started.
	SDL_Init(/*SDL_INIT_AUDIO | */SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);

	//Init SDL_Image for PNG support.
	if(!IMG_Init(IMG_INIT_PNG)) {
		fatalError("Fatal error", "SDL_Image did not initialise.");
	}

	//Init SDL_Mixer for simpler, high-level sound API.
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		fatalError("Fatal error", "SDL_Mixer did not initialise.");
	}

	//Now that everything's loaded - start the video.
	SDL_InitSubSystem(SDL_INIT_VIDEO);
}
static void initWindow(void) {
	window = SDL_CreateWindow(
		GAME_TITLE,
#ifdef DEBUG_WINDOW_T500
		1250,		//Off to the window side.
		0,
#else
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
#endif
		(int)windowSize.x,					//dimensions
		(int)windowSize.y,
		SDL_WINDOW_OPENGL | (FULLSCREEN ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_MAXIMIZED)
	);

	//Hide cursor in fullscreen
	if(FULLSCREEN) SDL_ShowCursor(SDL_DISABLE);

	assert(window != NULL);
}
static void shutdownWindow(void) {
	if(window == NULL) return;			//OK to call if not yet setup (an established subsystem pattern elsewhere)

	SDL_DestroyWindow(window);
	window = NULL;
}
static void shutdownMain(void) {
	shutdownAssets();
	shutdownRenderer();
	shutdownWindow();
	shutdownInput();

	SDL_Quit();
}

int main()  {
	//Seed randomMq number generator
	srand(time(NULL));

	atexit(shutdownMain);

	initSDL();
	initWindow();
	initRenderer();
	initAssets();
	initInput();

	initScripts();
	playerInit();
	initBackground();
	enemyInit();
	hudInit();
	pewInit();
	itemInit();


#ifdef DEBUG_SKIP_TO_GAME
	triggerState(STATE_GAME);
#else
	triggerState(STATE_INTRO);
#endif

#ifdef DEBUG_GODMODE
	godMode = true;
#endif

#ifdef DEBUG_FANWEAPONS
	weaponInc = 3;
#endif

	long lastRenderFrameTime = clock();
	long lastGameFrameTime = lastRenderFrameTime;
	long lastAnimFrameTime = lastRenderFrameTime;

	//Main game loop (realtime)
	while(running){
		//Game frame
		if(timer(&lastGameFrameTime, GAME_HZ)) {
			pollInput();
			processSystemCommands();
			backgroundGameFrame();
			scriptGameFrame();
			playerGameFrame();
			enemyGameFrame();
			itemGameFrame();
			pewGameFrame();
			hudGameFrame();
		}

		//Animation frame
		if(timer(&lastAnimFrameTime, ANIMATION_HZ)) {
			playerAnimate();
			animateEnemy();
			pewAnimateFrame();
			hudAnimateFrame();
			itemAnimateFrame();
		}

		//Renderer frame
		double renderFPS;
		if(timer(&lastRenderFrameTime, RENDER_HZ)) {
			//V-sync requires us to clear on every frame.
			if(vsync) clearBackground(makeBlack());

			backgroundRenderFrame();
			if(ENABLE_SHADOWS) {
				pewShadowFrame();
				enemyShadowFrame();
				playerShadowFrame();
				itemShadowFrame();
			}
			itemRenderFrame();
 			enemyRenderFrame();
			pewRenderFrame();
			scriptRenderFrame();
			playerRenderFrame();
			hudRenderFrame();
			faderRenderFrame();
			updateCanvas();
		}
	}

    return 0;
}
