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

static const char *GAME_TITLE = "Mouse Quest";
static const bool FULLSCREEN = false;
bool running = true;

static void initSDL(void) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	//Add PNG support via SDL_Image.
	if(!IMG_Init(IMG_INIT_PNG)) {
		fatalError("Fatal error", "SDL_Image did not initialise.");
	}

	//Sound via higher-level SDL_Mixer library.
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		fatalError("Fatal error", "SDL_Mixer did not initialise.");
	}
}
static void initWindow(void) {
	window = SDL_CreateWindow(
		GAME_TITLE,
#ifdef DEBUG_T500
		1250,		//Off to the window side.
		0,
#else
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
#endif
		(int)windowSize.x,					//dimensions
		(int)windowSize.y,
		FULLSCREEN ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_MAXIMIZED
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

	SDL_Quit();
}

int main()  {
	//Seed random number generator
	srand(time(NULL));

	atexit(shutdownMain);

	initSDL();
	initWindow();
	initRenderer();
	initAssets();

	initScripts();
	playerInit();
	initBackground();
	enemyInit();

	triggerState(STATE_INTRO);

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
			pewGameFrame();
			hudGameFrame();
		}

		//Animation frame
		if(timer(&lastAnimFrameTime, ANIMATION_HZ)) {
			playerAnimate();
			animateEnemy();
			pewAnimateFrame();
		}

		//Renderer frame
		double renderFPS;
		if(timer(&lastRenderFrameTime, RENDER_HZ)) {
			backgroundRenderFrame();
			scriptRenderFrame();
			enemyShadowFrame();
			playerShadowFrame();
 			enemyRenderFrame();
			pewRenderFrame();
			playerRenderFrame();
			hudRenderFrame();
			faderRenderFrame();
			updateCanvas();
		}
	}

    return 0;
}
