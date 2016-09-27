#ifndef _COMMON_H
#define _COMMON_H

#include "mysdl.h"
#include <stdbool.h>

//#define DEBUG_WINDOW_T500
//#define DEBUG_SKIP_TO_GAME
//#define DEBUG_CHEATS

extern const bool ENABLE_PARALLAX;
extern const bool ENABLE_SHADOWS;
extern const bool ALPHA_SHADOWS;
extern bool FULLSCREEN;

//MISC
extern const double ASPECT;
extern const int ANIMATION_HZ;
extern const int RENDER_HZ;
extern const int GAME_HZ;
extern const double RADIAN_CIRCLE;	//2 * pi (or 2 * 3.14)

typedef enum {
	STATE_INTRO = 0,
	STATE_TITLE = 1,
	STATE_GAME = 2,
	STATE_GAME_OVER = 3,
	STATE_COIN = 4,
	STATE_LEVEL_COMPLETE = 5
} GameState;
GameState gameState;

extern const bool vsync;
extern bool isScripted();
extern bool stateInitialised;
extern void triggerState(GameState newState);

//COORDINATES
typedef struct {
	double x, y;
} Coord;
extern Coord scaleCoord(Coord subject, double scalar);
extern Coord makeCoord(double x, double y);
extern Coord deriveCoord(Coord original, double xOffset, double yOffset);
extern Coord zeroCoord();
extern Coord addCoords(Coord a, Coord b);

//RECTANGLES
typedef struct {
	double x, y;
	int width, height;
} Rect;
extern Rect makeRect(double x, double y, double width, double height);
extern bool inBounds(Coord point, Rect area);
extern Rect makeBounds(Coord origin, double width, double height);
extern Rect makeSquareBounds(Coord origin, double size);

//COLOURS
typedef struct {
	int red, green, blue, alpha;
} Colour;
typedef enum {
	COLOURISE_ABSOLUTE = 0,
	COLOURISE_ADDITIVE = 1
} ColourisationMethod;
extern Colour makeColour(int red, int green, int blue, int alpha);
extern Colour makeOpaque(int red, int green, int blue);
extern Colour makeWhite();
extern Colour makeBlack();

extern int getPixel(SDL_Surface *surface, int x, int y);
extern void setPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
extern SDL_Surface *colouriseSprite(SDL_Surface *original, Colour colour, ColourisationMethod method);

extern int randomMq(int min, int max);
extern bool chance(int probability);
extern char *combineStrings(const char *a, const char *b);
extern void quit();
extern void fatalError(const char *title, const char *message);
extern bool timer(long *lastTime, double hertz);
extern double getFPS(long now, long lastFrameTime);
extern bool due(long compareTime, double milliseconds);
extern bool dueBetween(long compareTime, double milliseconds, double milliseconds2);

extern SDL_Window *window;
extern bool running;
extern bool fileExists(const char *path);
extern long ticsToMilliseconds(long tics);
extern int toMilliseconds(int seconds);
extern Coord windowSize;

//MATH
extern double sineInc(double offset, double *sineInc, double speed, double magnitude);
extern double cosInc(double offset, double *sineInc, double speed, double magnitude);
extern double getAngle(Coord a, Coord b);
extern Coord getStep(Coord a, Coord b, double speed, bool negativeMagic);

#endif
