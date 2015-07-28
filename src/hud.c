#include "common.h"
#include "renderer.h"
#include "assets.h"
#include "player.h"

#define NUM_HEARTS 4
static Sprite life, lifeHalf, lifeNone;
static Coord lifePositions[NUM_HEARTS];

void hudGameFrame(void) {

}

void hudInit(void) {
	life = makeSprite(getTexture("life.png"), zeroCoord(), SDL_FLIP_NONE);
	lifeHalf = makeSprite(getTexture("life-half.png"), zeroCoord(), SDL_FLIP_NONE);
	lifeNone = makeSprite(getTexture("life-none.png"), zeroCoord(), SDL_FLIP_NONE);

	//Set drawing coordinates for heart icons - each is spaced out in the upper-left.
	for(int i=0; i < NUM_HEARTS; i++){
		lifePositions[i] = makeCoord(10 + (i * 16), 10);
	}
}

void hudRenderFrame(void) {
	//Only show if playing.
	if(gameState != STATE_GAME) return;

	//Loop through icons and draw them at the appropriate 'fullness' levels for each health bar.
	// This algorithm will automatically scale according to whatever we choose to set the player's total health to.
	float healthPerHeart = playerStrength / NUM_HEARTS;		//e.g. for 4 hearts, 1 heart = 25 hitpoints.
	for(int bar=0; bar < NUM_HEARTS; bar++) {
		//The total health represented by this bar. We do a bit of crazy magic here to make sure we calculate this
		// correctly for a left-to-right pass.
		float barHealth = playerStrength - ((NUM_HEARTS - (bar+1)) * healthPerHeart);

		//Full bar.
		if(playerHealth >= barHealth) {
			drawSpriteAbs(life, lifePositions[bar]);
		//Between half and full.
		}else if(playerHealth >= barHealth - (healthPerHeart/2)) {
			drawSpriteAbs(lifeHalf, lifePositions[bar]);
		//Under half / none.
		}else{
			drawSpriteAbs(lifeNone, lifePositions[bar]);
		}
	}
}
