#include <time.h>
#include <assert.h>
#include "mysdl.h"
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
#include "level.h"
#include "myc.h"

// Running slow from Clion, but fast from Console? CLOSE or MINIMISE the Clion IDE.
// The Java GUI crapness is slowing the drawing down in the background :p

/* BUG: We use 'sizeof' way too often on things like Enums. This is *NOT* a good way to
		check for sizes, in fact it may just be pure coincidence that it works at all.
		If we're getting frequent attempts to render enemies without any textures, say,
 		then this is definitely the source of the problem. */

static const char *GAME_TITLE = "Mouse Quest";

#ifdef DEBUG_WINDOW_T500
	static const bool FULLSCREEN = false;
#else
#endif

bool running = true;

static void initSDL() {
	// !!!IMPORTANT!!!
	// Perceived slowness on startup is due to the CLion window!
	// Run the game from the terminal (with nothing showing in the background)
	// and the game will be perfectly smooth!
	// Tip: Improve performance by toggling off UI scaling.
	// !!!IMPORTANT!!!

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK /* | SDL_INIT_HAPTIC*/);

	//Init SDL_Image for PNG support.
	if(!IMG_Init(IMG_INIT_PNG)) {
		fatalError("Fatal error", "SDL_Image did not initialise.");
	}

	//Init SDL_Mixer for simpler, high-level sound API.
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		fatalError("Fatal error", "SDL_Mixer did not initialise.");
	}
}
static void initWindow() {
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
static void shutdownWindow() {
	if(window == NULL) return;			//OK to call if not yet setup (an established subsystem pattern elsewhere)

	SDL_DestroyWindow(window);
	window = NULL;
}
static void shutdownMain() {
	shutdownAssets();
	shutdownRenderer();
	shutdownWindow();
	shutdownInput();

	SDL_Quit();
}

#if defined(_WIN32)
	int main(int argc, char *argv[])  {
#else
	int main()  {
#endif
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
	levelInit();

	// This delay ensures the game starts up fast *as is*.
//	SDL_Delay(2000);

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
			levelGameFrame();
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
