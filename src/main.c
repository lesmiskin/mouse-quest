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

// !!!IMPORTANT!!!
// Perceived slowness on startup is due to the CLion window!
// Run the game from the terminal (with nothing showing in the background)
// and the game will be perfectly smooth!
// !!!IMPORTANT!!!

/* BUG: We use 'sizeof' way too often on things like Enums. This is *NOT* a good way to
		check for sizes, in fact it may just be pure coincidence that it works at all.
		If we're getting frequent attempts to render enemies without any textures, say,
 		then this is definitely the source of the problem. */

static const char *GAME_TITLE = "Mouse Quest";

bool running = true;

static void initSDL() {
	SDL_Init(SDL_INIT_VIDEO);

	//Init SDL_Image for PNG support.
	if(!IMG_Init(IMG_INIT_PNG)) {
		fatalError("Fatal error", "SDL_Image did not initialise.");
	}

	//Init SDL_Mixer for simpler, high-level sound API.
	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
		fatalError("Fatal error", "SDL_Mixer did not initialise.");
	}
}
static void initWindow() {
	window = SDL_CreateWindow(
		GAME_TITLE,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		(int)windowSize.x,					//dimensions
		(int)windowSize.y,
		SDL_WINDOW_OPENGL | (FULLSCREEN ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0)
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

static void setWindowIcon() {
	SDL_Surface* icon = reloadSurface("mike-lean-02.png");
	SDL_SetWindowIcon(window, icon);
}

#if defined(_WIN32)
	int main(int argc, char *argv[]) {

        // Windowed mode argument.
        if(argc > 1 && strcmp(argv[1], "-window") == 0) {
            FULLSCREEN = false;
            windowSize.x = 672;
            windowSize.y = 768;
        }

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
	setWindowIcon();
	initInput();
	initScripts();
	playerInit();
	initBackground();
	enemyInit();
	hudInit();
	pewInit();
	itemInit();
	levelInit();

#ifdef DEBUG_SKIP_TO_GAME
	triggerState(STATE_GAME);
#else
	triggerState(STATE_TITLE);
#endif
		
//	triggerState(STATE_LEVEL_COMPLETE);

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
			backgroundRenderFrame();
			enemyBackgroundRenderFrame();	// we show certain enemies behind the background.
			foregroundRenderFrame();		// show platforms.

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
			hudRenderFrameBackground();
			playerRenderFrame();
			hudRenderFrame();
			faderRenderFrame();
			persistentHudRenderFrame();
			updateCanvas();
		}
	}

    return 0;
}
