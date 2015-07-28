#include "common.h"
#include "renderer.h"
#include "assets.h"
#include "player.h"

void hudGameFrame(void) {

}

void hudRenderFrame(void) {
	//Only show if playing.
	if(gameState != STATE_GAME) return;

	//Life bar
	Sprite life = makeSprite(getTexture("life.png"), zeroCoord(), SDL_FLIP_NONE);
	Sprite lifeHalf = makeSprite(getTexture("life-half.png"), zeroCoord(), SDL_FLIP_NONE);
	Sprite lifeNone = makeSprite(getTexture("life-none.png"), zeroCoord(), SDL_FLIP_NONE);

	//TODO: Tidy this repetitive code up.
	if(playerHealth >= 2) {
		drawSpriteAbs(life, makeCoord(10, 10));
	}else if(playerHealth == 1) {
		drawSpriteAbs(lifeHalf, makeCoord(10, 10));
	}else{
		drawSpriteAbs(lifeNone, makeCoord(10, 10));
	}

	if(playerHealth >= 4) {
		drawSpriteAbs(life, makeCoord(24, 10));
	}else if(playerHealth == 3) {
		drawSpriteAbs(lifeHalf, makeCoord(24, 10));
	}else{
		drawSpriteAbs(lifeNone, makeCoord(24, 10));
	}

	if(playerHealth >= 6) {
		drawSpriteAbs(life, makeCoord(38, 10));
	}else if(playerHealth == 5) {
		drawSpriteAbs(lifeHalf, makeCoord(38, 10));
	}else{
		drawSpriteAbs(lifeNone, makeCoord(38  , 10));
	}

	if(playerHealth == 8) {
		drawSpriteAbs(life, makeCoord(52, 10));
	}else if(playerHealth == 7) {
		drawSpriteAbs(lifeHalf, makeCoord(52, 10));
	}else{
		drawSpriteAbs(lifeNone, makeCoord(52, 10));
	}

}