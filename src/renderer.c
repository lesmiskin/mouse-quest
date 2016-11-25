#include <assert.h>
#include <time.h>
#include "assets.h"
#include "common.h"
#include "renderer.h"
#include "player.h"
#include "myc.h"

// Core rendering
static const int BASE_SCALE_WIDTH = 224;
static const int BASE_SCALE_HEIGHT = 256;
SDL_Texture *renderBuffer;
SDL_Renderer *renderer = NULL;
Coord pixelGrid;								//helps aligning things to the tiled background.
Coord screenBounds;
static int renderScale;
static const double PIXEL_SCALE = 1;			//pixel doubling for assets.

// Fader
const FadeMode FADE_BOTH = FADE_IN | FADE_OUT;
static SDL_Texture* blackFader;
static SDL_Texture* whiteFader;
static const int FADE_DURATION = 1000;
static int fadeAlphaInc;
static FadeMode currentFadeMode;
static int currentFadeAlpha;
static bool fadeWhite;

// Screenshots
static SDL_Texture *shotBuffer;
static Coord shotDimensions;
static int screenshotInc = 0;

const int STATIC_SHADOW_OFFSET = 8;

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

//Default sprite_t drawing scales with pixel grid, for easy tiling.
void drawSprite(Sprite drawSprite, Coord origin) {
	Coord scaledOrigin = scaleCoord(origin, renderScale);
	drawSpriteAbs(drawSprite, scaledOrigin);
}

//Optional absolutely-positioned drawing, for precise character movements.
void drawSpriteAbsRotated2(Sprite sprite, Coord origin, double angle, double scaleX, double scaleY) {
	//Ensure we're always calling this with an initialised sprite_t.
	assert(sprite.texture != NULL);

	//Offsets should be relative to image pixel metrics, not screen metrics.
	int offsetX = sprite.offset.x * renderScale;
	int offsetY = sprite.offset.y * renderScale;

    // FIXME: Origin is when when scaling the sprites beyond 1.0

	//NB: We adjust the offset to ensure all sprites are drawn centered at their origin points
	offsetX -= (sprite.size.x / 2) * renderScale;
	offsetY -= (sprite.size.y / 2) * renderScale;

	//Configure homeTarget location output sprite_t size, adjusting the latter for the constant sprite_t scaling factor.
	SDL_Rect destination  = {
		(origin.x + offsetX),
		(origin.y + offsetY),
		sprite.size.x * scaleX,
		sprite.size.y * scaleY
	};

	//Rotation
	SDL_Point rotateOrigin = { 0, 0 };
	if(angle > 0) {
		rotateOrigin.x = (int)sprite.size.x / 2;
		rotateOrigin.y = (int)sprite.size.y / 2;
	};

	SDL_RenderCopyEx(renderer, sprite.texture, NULL, &destination, angle, &rotateOrigin, sprite.flip);
}

void drawSpriteAbsRotated(Sprite sprite, Coord origin, double angle) {
	drawSpriteAbsRotated2(sprite, origin, angle, 1, 1);
}

void drawSpriteAbs(Sprite sprite, Coord origin) {
	drawSpriteAbsRotated(sprite, origin, 0);
}

void screenshot() {
    // We do a little magic here to take screenshots with 3x scale.
    // We render one frame to a specially-scaled canvas texture, then write *that* to the file system :)
	// TODO: Stop overwriting existing shots between sessions.

	// Loop max screenshots.
	if(screenshotInc > 9999) screenshotInc = 0;

    // Prepare screenshot surface.
    SDL_Surface *shot = SDL_CreateRGBSurface(
        0, (int)shotDimensions.x, (int)shotDimensions.y, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000
    );

    // Redirect renderer to the screenshot buffer.
    SDL_SetRenderTarget(renderer, shotBuffer);

    // Use SDL to blit (and scale) renderBuffer contents to the screenshot buffer.
    SDL_RenderSetLogicalSize(renderer, (int)pixelGrid.x, (int)pixelGrid.y);
    SDL_RenderCopy(renderer, renderBuffer, NULL, NULL);

    // Copy the pixels from the renderer to our screenshot surface.
    SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, shot->pixels, shot->pitch);

    // Switch rendering target back to the buffer.
    SDL_SetRenderTarget(renderer, renderBuffer);

    // Prepare filename, and save to file system.
    char filename[12];
    sprintf(filename, "MQ%04d.bmp", screenshotInc++);

	// Save it to the filesystem
	SDL_SaveBMP(shot, filename);
	SDL_FreeSurface(shot);
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
void updateCanvas() {
	//Change rendering homeTarget to window.
	SDL_SetRenderTarget(renderer, NULL);

	//Activate scaler, and blit the buffer to the screen.
	SDL_RenderSetLogicalSize(renderer, (int)pixelGrid.x, (int)pixelGrid.y);
	SDL_RenderCopy(renderer, renderBuffer, NULL, NULL);

	//Actually update the screen itself.
	SDL_RenderPresent(renderer);

	//Reset render homeTarget back to texture buffer
	SDL_SetRenderTarget(renderer, renderBuffer);
}

void shutdownRenderer() {
	if(renderer == NULL) return;			//OK to call if not yet setup (thanks, encapsulation)

	SDL_DestroyRenderer(renderer);
	renderer = NULL;
}

Coord getSunPosition() {
	return makeCoord(
			screenBounds.x / 2,
//			screenBounds.x + (screenBounds.x / 2),
			-screenBounds.y / 3
	);
}

Coord getParallaxOffset() {
	//IMPORTANT: We only ever want the *X axis parallax*. Omitting the
	// Y axis causes trig enemy shots to work correctly, when
	// parallax is enabled.

	return makeCoord(
		(screenBounds.x / 2) - playerOrigin.x,
		(screenBounds.y / 2)
	);
}

Coord parallax(Coord subject, ParallaxReference reference, ParallaxLayer layer, ParallaxDimensions dimensions, ParallaxMode mode) {
	if(!ENABLE_PARALLAX || layer == 0) return subject;

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

bool isFading() {
	return currentFadeAlpha > 0 && currentFadeAlpha < 255;
}

void fadeInWhite() {
    currentFadeMode = FADE_IN;
    currentFadeAlpha = 255;
    fadeWhite = true;
}

void fadeIn() {
	currentFadeMode = FADE_IN;
	currentFadeAlpha = 255;
}

void fadeOut() {
	currentFadeMode = FADE_OUT;
	currentFadeAlpha = 0;
    fadeWhite = false;
}

static SDL_Texture* makeFader(int r, int g, int b) {
//	Create tile map canvas texture.
    SDL_Texture* fadeOverlay = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_TARGET,
        (int)pixelGrid.x, (int)pixelGrid.y
    );

    //Allow the texture to blend into other things when rendered.
    SDL_SetTextureBlendMode(fadeOverlay, SDL_BLENDMODE_BLEND);

    //Change renderer context to output onto the tilemap.
    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, fadeOverlay);

    //Can either render a texture with an alpha value, and alter the alpha value of it,
    // or render a solid texture, and just change the transparency with AlphaMod.

    //Render a solid colour, with full opaqueness (we will do alpha blending at render-time, at variable amount)
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderClear(renderer);

    //Restore render homeTarget to the buffer.
    SDL_SetRenderTarget(renderer, oldTarget);

    return fadeOverlay;
}

static void initFader() {
	fadeAlphaInc = (255 * RENDER_HZ) / (double)FADE_DURATION;

    blackFader = makeFader(0, 0, 0);
    whiteFader = makeFader(255, 255, 255);
}

void faderRenderFrame() {
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

    SDL_Texture* useFader = fadeWhite ? whiteFader : blackFader;
	SDL_SetTextureAlphaMod(useFader, currentFadeAlpha);
	SDL_RenderCopy(renderer, useFader, NULL, NULL);
}

void toggleFullscreen() {
	// Make a window.
	if(FULLSCREEN) {
		SDL_SetWindowSize(window, 672, 768);
		SDL_SetWindowFullscreen(window, SDL_WINDOW_SHOWN);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	// Go back to fullscreen.
	}else{
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}

	// Need to reinitialise fader after all this, otherwise it doesn't work properly :p
	initFader();

	FULLSCREEN = !FULLSCREEN;
}

void initRenderer() {
	//EXPERIMENTAL: Toggle for old-school 'bilinear filtering' look.
//	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

	//Init SDL renderer
	renderer = SDL_CreateRenderer(
		window,
		-1,							            //insert at default index position for renderer list.
		SDL_RENDERER_TARGETTEXTURE           	//supports rendering to textures.
	);
	assert(renderer != NULL);

	//Set virtual screen size and pixel doubling ratio.
	screenBounds = makeCoord(BASE_SCALE_WIDTH, BASE_SCALE_HEIGHT);		//virtual screen size
	renderScale = PIXEL_SCALE;											//pixel doubling

	//Pixel grid is the blocky rendering grid we use to help tile things (i.e. backgrounds).
	pixelGrid = makeCoord(
		BASE_SCALE_WIDTH / renderScale,
		BASE_SCALE_HEIGHT / renderScale
	);

	/* IMPORTANT: Make a texture which we render all contents to, then efficiently scale just
	 * this one texture upon rendering. This creates a *massive* speedup.
	 * Thanks to: https://forums.libsdl.org/viewtopic.php?t=10567 */
	renderBuffer = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGB24,
		SDL_TEXTUREACCESS_TARGET,
		(int)pixelGrid.x,
		(int)pixelGrid.y
	);

    // Screenshot canvas.
	shotDimensions = makeCoord(BASE_SCALE_WIDTH*3, BASE_SCALE_HEIGHT*3);
    shotBuffer = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_TARGET,
		(int)shotDimensions.x,
		(int)shotDimensions.y
    );

    /* Clear the entire window surface to a solid colour (need to do this since our actual
     * drawing area is a small vertical rectangle in the middle of the screen - leaving the
     * sides undrawn per-frame. */
	SDL_SetRenderTarget(renderer, NULL);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	// Everything from now on should be rendered on the clipped buffer region.
	SDL_SetRenderTarget(renderer, renderBuffer);

	initFader();
}