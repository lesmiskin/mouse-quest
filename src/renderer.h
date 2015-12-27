#ifndef RENDERER_H
#define RENDERER_H

#include "SDL.h"
#include "common.h"

typedef struct {
	SDL_Texture *texture;
	Coord offset;
	Coord size;
	SDL_RendererFlip flip;
} Sprite;

typedef enum {
	PARALLAX_SUN,
	PARALLAX_PAN
} ParallaxReference;

//Higher values = more stationary.
typedef enum {
	PARALLAX_LAYER_FOREGROUND = 6,
	PARALLAX_LAYER_SHADOW = 6,
	PARALLAX_LAYER_PLATFORM = 12,
	PARALLAX_LAYER_PLANET = 16
} ParallaxLayer;

typedef enum {
	PARALLAX_ADDITIVE = 1,
	PARALLAX_SUBTRACTIVE = -1
} ParallaxMode;

typedef enum {
	PARALLAX_X = 1,
	PARALLAX_Y = 2,
	PARALLAX_XY = 3,
} ParallaxDimensions;

extern SDL_Texture *renderBuffer;
extern const int STATIC_SHADOW_OFFSET;

extern void toggleFullscreen(void);
extern Coord getParallaxOffset(void);
extern Coord parallax(Coord subject, ParallaxReference reference, ParallaxLayer layer, ParallaxDimensions dimensions, ParallaxMode mode);
extern bool inScreenBounds(Coord subject);
extern Coord screenBounds;
extern int scalePixels(int pixels);
extern SDL_Renderer *renderer;
extern Coord pixelGrid;
extern Sprite makeSimpleSprite(char *textureName);
extern Sprite makeSprite(SDL_Texture *texture, Coord offset, SDL_RendererFlip flip);
extern void drawSpriteAbsRotated2(Sprite sprite, Coord origin, double angle, double scale);
extern void drawSpriteAbsRotated(Sprite drawSprite, Coord origin, double angle);
extern void drawSpriteAbs(Sprite drawSprite, Coord origin);
extern void drawSprite(Sprite drawSprite, Coord origin);
extern void setDrawColour(Colour drawColour);
extern void clearBackground(Colour clearColour);
extern void updateCanvas();
extern void initRenderer();
extern void shutdownRenderer();

//Fader
extern void faderRenderFrame(void);
extern bool isFading(void);
extern void fadeIn(void);
extern void fadeOut(void);

typedef enum {
	FADE_NONE = 0,
	FADE_IN = 1,
	FADE_OUT = 2
} FadeMode;
extern const FadeMode FADE_BOTH;

#endif