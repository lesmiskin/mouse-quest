#include <assert.h>
#include <time.h>
#include "assets.h"
#include "common.h"
#include "renderer.h"
#include "player.h"

//TODO: Stop sharing the renderScale, and do it properly! (Present an already-scaled data structure).
//TODO: Better way of returning/zeroing structures?

SDL_Texture *renderBuffer;
const int STATIC_SHADOW_OFFSET = 8;
SDL_Renderer *renderer = NULL;
static int renderScale;
static const double PIXEL_SCALE = 1;				//pixel doubling for assets.
static const int BASE_SCALE_WIDTH = 224;		//our base scale for all game activity.
static const int BASE_SCALE_HEIGHT = 256;		//our base scale for all game activity.
Coord pixelGrid;								//helps aligning things to the tiled background.
Coord screenBounds;
static const int PARALLAX_SCALE_DIVISOR = 10;

const FadeMode FADE_BOTH = FADE_IN | FADE_OUT;
static SDL_Texture* fadeOverlay;
static const int FADE_DURATION = 1000;
static int fadeAlphaInc;
static long currentFadeStartTime;
static FadeMode currentFadeMode;
static int currentFadeAlpha;

Coord getTextureSize(SDL_Texture *texture) {
	int x, y;
	SDL_QueryTexture(texture, NULL, NULL, &x, &y);

	return makeCoord(x, y);
}

Sprite makeSprite(SDL_Texture *texture, Coord offset, SDL_RendererFlip flip) {
	Sprite sprite = {
		texture, offset, getTextureSize(texture), flip
	};
	return sprite;
}

Sprite makeSimpleSprite(char *textureName) {
	SDL_Texture *texture = getTexture(textureName);
	return makeSprite(texture, zeroCoord(), SDL_FLIP_NONE);
}

bool inScreenBounds(Coord subject) {
	return
		subject.x > 0 && subject.x < screenBounds.x &&
		subject.y > 0 && subject.y < screenBounds.y;
}

int scalePixels(int pixels) {
	return pixels * renderScale;
}

//Default sprite_t drawing scales with pixel grid, for easy tiling.
void drawSprite(Sprite drawSprite, Coord origin) {
	Coord scaledOrigin = scaleCoord(origin, renderScale);
	drawSpriteAbs(drawSprite, scaledOrigin);
}

//Optional absolutely-positioned drawing, for precise character movements.
void drawSpriteAbsRotated(Sprite sprite, Coord origin, double angle) {
	//Ensure we're always calling this with an initialised sprite_t.
	assert(sprite.texture != NULL);

	//Offsets should be relative to image pixel metrics, not screen metrics.
	int offsetX = sprite.offset.x * renderScale;
	int offsetY = sprite.offset.y * renderScale;

	//NB: We adjust the offset to ensure all sprites are drawn centered at their origin points
	offsetX -= (sprite.size.x / 2) * renderScale;
	offsetY -= (sprite.size.y / 2) * renderScale;

	//Configure target location output sprite_t size, adjusting the latter for the constant sprite_t scaling factor.
	SDL_Rect destination  = {
		(origin.x + offsetX),
		(origin.y + offsetY),
		sprite.size.x * renderScale,
		sprite.size.y * renderScale
	};

	//Rotation
	SDL_Point rotateOrigin = { 0, 0 };
	if(angle > 0) {
		rotateOrigin.x = (int)sprite.size.x / 2;
		rotateOrigin.y = (int)sprite.size.y / 2;
	};

	SDL_RenderCopyEx(renderer, sprite.texture, NULL, &destination, angle, &rotateOrigin, sprite.flip);
}

void drawSpriteAbs(Sprite sprite, Coord origin) {
	drawSpriteAbsRotated(sprite, origin, 0);
}

void setDrawColour(Colour colour) {
	SDL_SetRenderDrawColor(
		renderer,
		colour.red,
		colour.green,
		colour.blue,
		colour.alpha
	);
}
void clearBackground(Colour colour) {
	setDrawColour(colour);
	SDL_RenderClear(renderer);
}
void updateCanvas(void) {
	//Change rendering target to window.
	SDL_SetRenderTarget(renderer, NULL);

	//Activate scaler, and blit the buffer to the screen.
	SDL_RenderSetLogicalSize(renderer, pixelGrid.x, pixelGrid.y);
	SDL_RenderCopy(renderer, renderBuffer, NULL, NULL);

	//Actually update the screen itself.
//	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
//	SDL_UpdateWindowSurface(window);

	//Reset render target back to texture buffer
	SDL_SetRenderTarget(renderer, renderBuffer);
}

void shutdownRenderer(void) {
	if(renderer == NULL) return;			//OK to call if not yet setup (thanks, encapsulation)

	SDL_DestroyRenderer(renderer);
	renderer = NULL;
}

Coord getSunPosition(void) {
	return makeCoord(
		screenBounds.x / 2,
//		screenBounds.x + (screenBounds.x / 2),
		-screenBounds.y / 3
	);
}

Coord getParallaxOffset(void) {
	//IMPORTANT: We only ever want the *X axis parallax*. Omitting the
	// Y axis causes trig enemy shots to work correctly, when
	// parallax is enabled.

	return makeCoord(
		(screenBounds.x / 2) - playerOrigin.x,
		(screenBounds.y / 2)
	);
}

Coord parallax(Coord subject, ParallaxReference reference, ParallaxLayer layer, ParallaxDimensions dimensions, ParallaxMode mode) {
	if(!ENABLE_PARALLAX) return subject;

	//Use a different frame of reference depending on parameter.
	Coord relativeOrigin = reference == PARALLAX_SUN ?
		getSunPosition() :
	   	getParallaxOffset();

	//Calculate distance between points, soften by layer divisor, use mode positive or negative to determine direction,
	// and offset the subject coordinate accordingly.
	return makeCoord(
		dimensions & PARALLAX_X ? subject.x + (((relativeOrigin.x - subject.x) / layer) * mode) : subject.x,
		dimensions & PARALLAX_Y ? subject.y + (((relativeOrigin.y - subject.y) / layer) * mode) : subject.y
	);
}

bool isFading(void) {
	return currentFadeAlpha > 0 && currentFadeAlpha < 255;
}

void fadeIn(void) {
	currentFadeMode = FADE_IN;
	currentFadeAlpha = 255;
	currentFadeStartTime = clock();
}

void fadeOut(void) {
	currentFadeMode = FADE_OUT;
	currentFadeAlpha = 0;
	currentFadeStartTime = clock();
}

static void makeFader(void) {

	fadeAlphaInc = (255 * RENDER_HZ) / (double)FADE_DURATION;

//	Create tile map canvas texture.
	fadeOverlay = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_TARGET,
		(int)pixelGrid.x, (int)pixelGrid.y
	);

	//Allow the texture to blend into other things when rendered.
	SDL_SetTextureBlendMode(fadeOverlay, SDL_BLENDMODE_BLEND);

	//Change renderer context to output onto the tilemap.
	SDL_SetRenderTarget(renderer, fadeOverlay);

	//Can either render a texture with an alpha value, and alter the alpha value of it,
	// or render a solid texture, and just change the transparency with AlphaMod.

	//Render a solid colour, with full opaqueness (we will do alpha blending at render-time, at variable amount)
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	//Restore renderer context back to the window canvas, and reset draw colour.
	SDL_SetRenderTarget(renderer, NULL);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
}

void faderRenderFrame(void) {
	switch(currentFadeMode) {
		case FADE_NONE:
			return;
		case FADE_IN:
			//Halt the fader if we're <= the max value (otherwise we get a frame of opaqueness)
			if((currentFadeAlpha - fadeAlphaInc) <= 0) {
				currentFadeMode = FADE_NONE;
				currentFadeAlpha = 0;
				return;
			}else{
				currentFadeAlpha -= fadeAlphaInc;
			}
			break;
		case FADE_OUT:
			if(currentFadeAlpha >= 255) {
				currentFadeMode = FADE_NONE;
				currentFadeAlpha = 255;
			}else{
				currentFadeAlpha += 5;
			}
			break;
	}

	SDL_SetTextureAlphaMod(fadeOverlay, currentFadeAlpha);
	SDL_RenderCopy(renderer, fadeOverlay, NULL, NULL);
}

void initRenderer(void) {
	//Enable v-sync in SDL.
	if(vsync) SDL_GL_SetSwapInterval(1);

	//EXPERIMENTAL: Toggle for old-school 'bilinear filtering' look.
//	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

	//Init SDL renderer
	renderer = SDL_CreateRenderer(
		window,
		-1,							            //insert at default index position for renderer list.
		SDL_RENDERER_ACCELERATED |              //should use hardware acceleration
		SDL_RENDERER_TARGETTEXTURE          	//supports rendering to textures.
//		| SDL_RENDERER_PRESENTVSYNC
	);

	//TODO: We need some prose to describe the concepts at play here. Currently very confusing.

	//Set virtual screen size and pixel doubling ratio.
	screenBounds = makeCoord(BASE_SCALE_WIDTH, BASE_SCALE_HEIGHT);		//virtual screen size
	renderScale = PIXEL_SCALE;											//pixel doubling

	//Pixel grid is the blocky rendering grid we use to help tile things (i.e. backgrounds).
	pixelGrid = makeCoord(
		BASE_SCALE_WIDTH / renderScale,
		BASE_SCALE_HEIGHT / renderScale
	);

	//IMPORTANT: Make a texture which we render all contents to, then efficiently scale just this one
	// texture upon rendering. This creates a *massive* speedup. Thanks to: https://forums.libsdl.org/viewtopic.php?t=10567
	renderBuffer = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGB24,
		SDL_TEXTUREACCESS_TARGET,
		(int)pixelGrid.x,
		(int)pixelGrid.y
	);

	//Use SDL to scale our game activity so it's independent of the output resolution.
	//Base it on height, since we use a portrait, not landscape game window.
//	double sdlScale = windowSize.y / BASE_SCALE_HEIGHT;
//	SDL_RenderSetScale(renderer, sdlScale, sdlScale);

	//Clear the entire canvas, then use SDL to scale up our virtual resolution -
	// allowing us to have a centered, portrait window :)

	//Rachaie's background.
//	SDL_RenderClear(renderer);
//	SDL_RenderPresent(renderer);

	SDL_SetRenderTarget(renderer, renderBuffer);

//	SDL_RenderSetLogicalSize(renderer, pixelGrid.x, pixelGrid.y);

	//Alternative clipping strategy:
//	SDL_Rect viewportRect = {100, 0, pixelGrid.x, pixelGrid.y};
//	SDL_RenderSetViewport(renderer, &viewportRect);

	//Alternative clipping strategy:
//  SDL_Rect clipRect = {0, 0, pixelGrid.x, pixelGrid.y};
//	SDL_RenderSetClipRect(renderer, &clipRect);

/*	SDL_DisplayMode dp = {
		SDL_PIXELFORMAT_RGB24,
		200,
		200,
		60
	};
	SDL_SetWindowDisplayMode(window, &dp);
*/
//	SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "opengl");
//	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
//	SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "1");

//	SDL_SetWindowDisplayMode(window, &dp);
	assert(renderer != NULL);

	makeFader();
}
