#include "myc.h"
#include "common.h"
#include <unistd.h>
#include <time.h>

SDL_Window *window = NULL;
GameState gameState;
const bool vsync = false;

const bool ENABLE_PARALLAX = true;
const bool ENABLE_SHADOWS = true;
const bool ALPHA_SHADOWS = false;
bool FULLSCREEN = true;

//Windowed resolutions
//Coord windowSize = { 224, 256 };		//1:1 scale
//Coord windowSize = { 448, 512 };		//2:1 scale
//Coord windowSize = { 672, 768 };		//3:1 scale
//Coord windowSize = { 896, 1024 };		//4:1 scale

//Widescreen resolutions
//Coord windowSize = { 1280, 720};		//720p
//Coord windowSize = { 1360, 768 };		//Perfect 3:1.
//Coord windowSize = { 1600, 900 };

//Native resolution

//	Coord windowSize = { 1680, 1050 };
	Coord windowSize = { 672, 768 };		//4:1 scale

const double ASPECT = 1.142;
const int ANIMATION_HZ = 1000 / 12;		//12fps
const int RENDER_HZ = 1000 / 60;		//60fps
const int GAME_HZ = 1000 / 60;			//60fps
const double RADIAN_CIRCLE = 6.28;

bool stateInitialised;

bool isScripted() {
	return gameState != STATE_GAME;
}

void triggerState(GameState newState) {
	gameState = newState;
	stateInitialised = false;
}

int randomMq(int min, int max) {
	return (rand() % (max + 1 - min)) + min;
}

bool chance(int probability) {
	//Shortcuts for deterministic scenarios (impossible and always)
	if(probability == 0) {
		return false;
	}else if (probability == 100) {
		return true;
	}

	//TODO: Consider simplified randomMq expression based on size of probability (e.g. 50% needs only range of 1).

	int roll = randomMq(0, 100);			//dice roll up to 100 (to match with a percentage-based probability amount)
	return probability >= roll;			//e.g. 99% is higher than a roll of 5, 50, and 75.
}

//Constructors
Coord makeCoord(double x, double y) {
	Coord coord = { x, y };
	return coord;
}
Coord deriveCoord(Coord original, double xOffset, double yOffset) {
	original.x += xOffset;
	original.y += yOffset;
	return original;
}
Coord zeroCoord() {
	Coord coord = { };
	return coord;
}
Coord scaleCoord(Coord subject, double scalar) {
	return makeCoord(subject.x * scalar, subject.y * scalar);
}
Coord addCoords(Coord a, Coord b) {
	return makeCoord(
		a.x + b.x,
		a.y + b.y
	);
}
Rect makeRect(double x, double y, double width, double height) {
	Rect rect = { x, y, width, height };
	return rect;
}
Colour makeColour(int red, int green, int blue, int alpha) {
	Colour colour = { red, green, blue, alpha };
	return colour;
}
Colour makeOpaque(int red, int green, int blue) {
	Colour colour = makeColour(red, green, blue, 255);
	return colour;
}
Colour makeWhite() {
	return makeOpaque(255, 255, 255);
}
Colour makeBlack() {
	return makeOpaque(0, 0, 0);
}

//Convenience functions
char *combineStrings(const char *a, const char *b) {
	//Allocate exact amount of space necessary to hold the two strings.
	char *result = malloc(strlen(a) + strlen(b) + 1);		//+1 for the zero-terminator
	strcpy(result, a);
	strcat(result, b);

	return result;
}
void quit() {
	running = false;
}
void fatalError(const char *title, const char *message) {
	//Show a message box, then quit.
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR,
		title,
		message,
		window
	);

	//NB: Our own quit() is not enough, since asset errors can hang event loop.
	SDL_Quit();
}
bool fileExists(const char *path) {
	return access(path, R_OK ) == 0;
}
long ticsToMilliseconds(long tics) {
	//we want the duration version of the platform-independent seconds, so we / 1000.
	long platformAgnosticMilliseconds = CLOCKS_PER_SEC / 1000;

	return tics / platformAgnosticMilliseconds;
}
int toMilliseconds(int seconds) {
	return seconds * 1000;
}

int getPixel(SDL_Surface *surface, int x, int y) {
	int *pixels = (int *)surface->pixels;
	return pixels[ ( y * surface->w ) + x ];
}
void setPixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
	int *pixels = (int *)surface->pixels;
	pixels[ ( y * surface->w ) + x ] = pixel;
}

//Note: We offer an additive blend mode, which is different from the multiplicative approach offered by
//SDL's colour modulate. Also, we operate on a surface, rather than a texture.
SDL_Surface *colouriseSprite(SDL_Surface *original, Colour colour, ColourisationMethod method) {
	//Set all opaque pixels as per colour argument.
	for( int x = 0; x < original->w; x++) {
		for( int y = 0; y < original->h; y++) {
			//Obtain alpha channel from pixel
			int pixel = getPixel(original, x, y);
			Uint8 oAlpha, or, og, ob;
			SDL_GetRGBA(pixel, original->format, &or, &og, &ob, &oAlpha);

			//Don't colourise fully-transparent pixels.
			if(oAlpha == 0) continue;

			Colour final;

			//Set colour to what's supplied without any modulation.
			if(method == COLOURISE_ABSOLUTE) {
				final = colour;
				final.alpha = colour.alpha;
			//Increase each colour channel value by that of the input colour, and cancel out any channel
			// that is not in the input - ensuring a complete colourisation every time.
			}else{
				final.red = 	colour.red > 0 ? or + colour.red > 255 ? 255 : or + colour.red : 0;
				final.green = 	colour.green > 0 ? og + colour.green > 255 ? 255 : og + colour.green : 0;
				final.blue = 	colour.blue > 0 ? ob + colour.blue > 255 ? 255 : ob + colour.blue : 0;
				final.alpha = 	colour.alpha;
			}

			setPixel(original, x, y, SDL_MapRGBA(
				original->format,
				final.red, final.green, final.blue, final.alpha)
			);
		}
	}
}

bool inBounds(Coord point, Rect area) {
	return
		point.x >= area.x && point.x <= area.width &&
		point.y >= area.y && point.y <= area.height;
}

Rect makeBounds(Coord origin, double width, double height) {
	Rect bounds = {
		origin.x - (width / 2),
		origin.y - (height / 2),
		origin.x + (width / 2),
		origin.y + (height / 2)
	};
	return bounds;
}

Rect makeSquareBounds(Coord origin, double size) {
	return makeBounds(origin, size, size);
}

double getFPS(long now, long lastFrameTime) {
	long timeSinceLast = ticsToMilliseconds(now - lastFrameTime);

	//Prevent division by zero on first frame.
	if(timeSinceLast == 0) {
		return 0;
	}else{
		return 1000 / timeSinceLast;
	}
}

bool timer(long *lastTime, double hertz){
	long now = clock();
	if(due(*lastTime, hertz)) {
		*lastTime = now;
		return true;
	}else{
		return false;
	}
}

bool dueBetween(long compareTime, double milliseconds, double milliseconds2) {
	long time = ticsToMilliseconds(clock() - compareTime);
	return time >= milliseconds && time <= milliseconds2;
}

bool due(long compareTime, double milliseconds) {
	return ticsToMilliseconds(clock() - compareTime) >= milliseconds;
}

double sineInc(double offset, double *sineInc, double speed, double magnitude) {
	*sineInc = *sineInc >= RADIAN_CIRCLE ? 0 : *sineInc + speed;

	double sineOffset = (sin(*sineInc) * magnitude);
	return offset - sineOffset;
}

double cosInc(double offset, double *sineInc, double speed, double magnitude) {
	*sineInc = *sineInc >= RADIAN_CIRCLE ? 0 : *sineInc + speed;

	double sineOffset = (cos(*sineInc) * magnitude);
	return offset - sineOffset;
}

double getAngle(Coord a, Coord b) {
	return atan2(b.y - a.y, b.x - a.x);
}

Coord getStep(Coord a, Coord b, double speed, bool negativeMagic) {
	//Already there?
	if(a.x == b.x && a.y == b.y) return zeroCoord();

	//Find the angle.
	double angle = getAngle(a, b);

	//Do trig to find the width and height of the next movement.
	//TODO: Figure out weird neg magic going on here...
	return makeCoord(
		(cos(angle) * (negativeMagic ? -speed : speed)),
		(sin(angle) * (negativeMagic ? -speed : speed))
	);
}
