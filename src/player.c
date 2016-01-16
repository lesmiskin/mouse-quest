#include <time.h>
#include "player.h"
#include "weapon.h"
#include "assets.h"
#include "renderer.h"
#include "input.h"
#include "hud.h"
#include "sound.h"

//NB: There is a conceptual distinction between realtime and animated effects. e.g. movment is realtime, whereas the
// animation frames are not. This is consistent with the presentation of Tyrian, and rotating powerups/weapons in
// Quake. i.e. the update rate is higher than the animation rate.
//TODO: Stop player being able to accel. past his MAX velocity with both strafe and forward motions.

typedef enum {
	LEAN_NONE = 0,
	LEAN_LEFT = 1,
	LEAN_RIGHT = 2
} LeanDirection;

typedef enum {
	Y_NONE = 0,
	Y_UP = 1,
	Y_DOWN = 2
} YDirection;

PlayerState playerState;

const PlayerState PSTATE_CAN_ANIMATE = PSTATE_NOT_PLAYING | PSTATE_NORMAL | PSTATE_WON | PSTATE_DYING | PSTATE_SMILING;
const PlayerState PSTATE_CAN_CONTROL = PSTATE_NORMAL;

static bool canControl() {
	return (playerState&PSTATE_CAN_CONTROL) > 0;
}
static bool canAnimate() {
	return ( playerState&PSTATE_CAN_ANIMATE) > 0;
}
static bool isDying() {
	return playerState == PSTATE_DYING;
}

bool godMode = false;
bool useMike;		//onscreen, responds to actions etc.
bool hideMike;		//still there, but don't render this frame.
double intervalFrames;
double dieInc = 0;
Coord playerOrigin;
double playerStrength = 8.0;
double playerHealth;
static long deathTime = 0;
static const double PLAYER_MAX_SPEED = 4.0;
//static const double MOMENTUM_INC_DIVISOR = 6.5;	//works well with joystick.
static const double MOMENTUM_INC_DIVISOR = 7.5;
static const int ANIMATION_FRAMES = 8;
static const int SHOOTING_FRAMES = 2;
static const int SMILING_FRAMES = 13;
static int animationInc;
static double momentumInc;			//PLAYER_MAX_SPEED / MOMENTUM_INC_DIVISOR = momentumInc
static LeanDirection leanDirection;
static YDirection yDirection;
static Sprite bodySprite;
static Coord thrustState;			//Stores direction state (-1 = left/down, 1 = up/right, 0 = stationary)
static Coord momentumState;
static Coord PLAYER_SIZE = { 6, 7 };
static Rect movementBounds;
static bool begunDyingRender = false;
static bool begunSmiling = false;
static const double HIT_KNOCKBACK = 0.5;
bool playerShooting;
bool begunShooting;
static double BUBBLE_TIME_SECONDS = 1.5;
static bool bubbleFinished = false;
static long bubbleLastTime;
static char* frameName;
static long lastHitTime;
static bool pain;
static bool flickerPain;
static const int PAIN_RECOVER_TIME = 2000;
static bool painShocked;
static PlayerState lastState;

static double dieBounce;
static bool dieDir;
static double dieSide;
static bool begunDyingGame;

extern void smile(void) {
	lastState = playerState;
	playerState = PSTATE_SMILING;
}

extern bool atFullhealth(void) {
	return playerHealth == playerStrength;
}

extern void restoreHealth(void) {
	if(godMode) return;

	playerHealth = playerStrength;
}

void hitPlayer(double damage) {
	//Don't take damage when in hit recovery mode.
	if(pain && !isDying() || godMode) return;

	if(begunDyingGame && dieBounce < 1) {
		dieBounce = 4.0;
		return;
	}

	//Take damage.
	playerHealth -= damage;
	play("Hit_Hurt18.wav");
//	SDL_HapticRumblePlay(haptic, 5.00, 750);

	//Remove any powerups / reset weapon to default..
	if(weaponInc > 0) {
		weaponInc = 0;
		play("loss.wav");
	}

	lastHitTime = clock();
	pain = true;
	painShocked = 0;

	//Apply knockback, but only if we're within bounds.
	Coord predicted = addCoords(playerOrigin, deriveCoord(playerOrigin, 0, HIT_KNOCKBACK));
	if(predicted.y <= movementBounds.height) {
		playerOrigin.y += HIT_KNOCKBACK;
	}
}

//Perform an animation sceneNumber.
void playerAnimate(void) {
	if(!canAnimate()) return;

	//TODO: Fix hard-coded smiling state animation loop.

	//Reset animation loop.
	if(playerState != PSTATE_SMILING) {
		animationInc = animationInc == ANIMATION_FRAMES ? 1 : animationInc + 1;
	}

	char frameFile[255];
	char* animGroupName;
	AssetVersion frameVersion = ASSET_DEFAULT;

	//Death.
	if(playerState == PSTATE_SMILING) {
		if(!begunSmiling) {
			animationInc = 1;
			begunSmiling = true;
		}else if(animationInc == 5) {
			play("ping2.wav");
		}else if(animationInc == SMILING_FRAMES) {
			playerState = lastState;
			animationInc = 1;
			begunSmiling = false;
			return;
		}
		animGroupName = "mike-shades-%02d.png";
		animationInc++;
	}else if(isDying()) {
		//Start to die - reset animation frames.
		if(!begunDyingRender) {
			play("mike-die.wav");
			animationInc = 1;
			begunDyingRender = true;
			deathTime = clock();

			//Save score.
			if(score > topScore) topScore = score;
		}
			//Flag once bounced completely offscreen (with a little padding)
		else if(playerOrigin.y > screenBounds.y + 64){
			playerState = PSTATE_DEAD;
			deathTime = 0;
			triggerState(STATE_GAME_OVER);
//			triggerState(STATE_TITLE);
			return;
		}

		if(dieDir) {
			animGroupName = "mike-fright-right.png";
		}else{
			animGroupName = "mike-fright-left.png";
		}
	}
		//Pain: In shock (change frame)
	else if(pain && !painShocked) {
		animGroupName = "mike-shock3.png";
		painShocked = true;
	}
		//Idle frames.
	else{
		//Pain: Flicker during recovery time.
		if(pain) {
			if(flickerPain) {
//				frameVersion = ASSET_ALPHA;
				hideMike = true;
				flickerPain = false;
			}else{
				hideMike = false;
				flickerPain = true;
			}
			//Ensure mike is always restored after being in pain.
		}else if(hideMike) {
			hideMike = false;
		}

		//Shooting.
		if(playerShooting) {
			//Start shooting
			if(!begunShooting) {
				animationInc = 1;
				begunShooting = true;
			}
				//Cycle shooting animation.
			else if(animationInc > SHOOTING_FRAMES) {
				animationInc = 1;
			}
			if(leanDirection == LEAN_LEFT) {
				animGroupName = "mike-shoot-left-%02d.png";
			}else if(leanDirection == LEAN_RIGHT) {
				animGroupName = "mike-shoot-right-%02d.png";
			}else {
				animGroupName = "mike-shoot-%02d.png";
			}
		}
			//Regular idle.
		else{
			if(leanDirection == LEAN_LEFT) {
				animGroupName = "mike-lean-left-%02d.png";
			}else if(leanDirection == LEAN_RIGHT) {
				animGroupName = "mike-lean-right-%02d.png";
			}else if(yDirection == Y_UP) {
				animGroupName = "mike-%02d.png";
			}else {
				animGroupName = "mike-facing-%02d.png";
			}
		}
	}

	//Select animation frame from above group, and draw.
	sprintf(frameFile, animGroupName, animationInc);

	//Remember what frame we're on for shadow drawing.
	frameName = realloc(frameName, strlen(frameFile) + 1);
	strcpy(frameName, frameFile);

	//Now, assign it.
	SDL_Texture* texture = getTextureVersion(frameFile, frameVersion);

	bodySprite = makeSprite(texture, zeroCoord(), SDL_FLIP_NONE);
}

void playerShadowFrame(void) {
	if(!useMike || hideMike) return;

	SDL_Texture* shadowTexture = getTextureVersion(frameName, ASSET_SHADOW);
	if(shadowTexture != NULL) {
		Sprite shadow = makeSprite(shadowTexture, zeroCoord(), SDL_FLIP_NONE);
		Coord shadowCoord = parallax(playerOrigin, PARALLAX_SUN, PARALLAX_LAYER_SHADOW, PARALLAX_X, PARALLAX_SUBTRACTIVE);
		shadowCoord.y += STATIC_SHADOW_OFFSET;

		drawSpriteAbs(
				shadow,
				shadowCoord
		);
	}
}

//Render the player at a given frame, independent of animation.
void playerRenderFrame(void) {
	if(!useMike || hideMike) return;
	if((playerState&PSTATE_DEAD) > 0) return;

	switch(gameState) {
		case STATE_TITLE:
			playerOrigin.y = 220;

			Sprite bubbleSprite = makeSprite(getTexture("speech-coin.png"), zeroCoord(), SDL_FLIP_NONE);
			Coord position = playerOrigin;
			position.y -= 16;
			drawSprite(bubbleSprite, position);
			break;

		case STATE_GAME:
			if(!bubbleFinished) {
				if(timer(&bubbleLastTime, toMilliseconds(BUBBLE_TIME_SECONDS))) {
					bubbleFinished = true;
				}else{
					Sprite bubbleSprite = makeSprite(getTexture("speech-entry.png"), zeroCoord(), SDL_FLIP_NONE);
					Coord position = playerOrigin;
					position.y -= 16;
					position.x += 15;
					drawSprite(bubbleSprite, position);
				}
			}
			break;
	}

	//Draw him.
	drawSpriteAbs(bodySprite, playerOrigin);
}

static void applyMomentum(void) {
	//TODO: Clean up duplication.

	//Increase momentum if we're thrusting towards that direction, but limit to max speed.
	if(thrustState.x < 0 && momentumState.x > -PLAYER_MAX_SPEED)		momentumState.x -= momentumInc;
	else if(thrustState.x > 0 && momentumState.x < PLAYER_MAX_SPEED) 	momentumState.x += momentumInc;
	if(thrustState.y < 0 && momentumState.y > -PLAYER_MAX_SPEED) 		momentumState.y -= momentumInc;
	else if(thrustState.y > 0 && momentumState.y < PLAYER_MAX_SPEED) 	momentumState.y += momentumInc;

	//If we're not thrusting in a direction, but still have some momentum, decelerate back to zero.
	if(thrustState.x == 0 && momentumState.x != 0){
		double currentDiff = fabs(momentumState.x);
		double offsetInc = currentDiff < momentumInc ? currentDiff : momentumInc;		//ensure we always return to zero, and don't say in limbo.
		momentumState.x += momentumState.x < 0 ? offsetInc : -offsetInc;				//decel by moving one increment towards zero.
	}
	if(thrustState.y == 0 && momentumState.y != 0) {
		double currentDiff = fabs(momentumState.y);
		double offsetInc = currentDiff < momentumInc ? currentDiff : momentumInc;
		momentumState.y += momentumState.y < 0 ? offsetInc : -offsetInc;
	}

	//Predict where we're going to end up.
	Coord predicted = addCoords(playerOrigin, momentumState);

	//Stop momentum if we'd reach, or overshoot, the screen bounds, but allow this kind
	// of behaviour during scripted sequences.
	if(!isScripted()) {
		if(predicted.x <= movementBounds.x || predicted.x >= movementBounds.width)
			momentumState.x = 0;
		if(predicted.y <= movementBounds.y || predicted.y >= movementBounds.height)
			momentumState.y = 0;
	}
}

static void recogniseThrust(void) {
	//Toggle thrust in a particular direction, based on key press.
	if(checkCommand(CMD_PLAYER_UP)){
		thrustState.y = -1;
		yDirection = Y_UP;
	}else if(checkCommand(CMD_PLAYER_DOWN)){
		thrustState.y = 1;
	}else if(yDirection != Y_NONE) {
		yDirection = Y_NONE;
	}

	if(checkCommand(CMD_PLAYER_LEFT)){
		thrustState.x = -1;
		leanDirection = LEAN_LEFT;
	}else if(checkCommand(CMD_PLAYER_RIGHT)){
		thrustState.x = 1;
		leanDirection = LEAN_RIGHT;
	}
		//Reset lean direction when unpressed.
	else if(leanDirection != LEAN_NONE){
		leanDirection = LEAN_NONE;
	}
}

void playerGameFrame(void) {
	if(isDying()) {
		//Set initial trajectory (we just do an incremental approach, no
		// parabolic math here).
		if(!begunDyingGame) {
			//Bounce towards opposite side of screen so we don't miss him.
			dieDir = playerOrigin.x < (screenBounds.x / 2);
			dieBounce = (double)randomMq(30, 55) / 10;
			dieSide = (double)randomMq(65, 85) / 100;
			begunDyingGame = true;
		}

		if(dieDir) {
			playerOrigin.x += dieSide;
		}else{
			playerOrigin.x -= dieSide;
		}

		//Decrease bounce thrust over time.
		playerOrigin.y -= (dieBounce -= 0.15);
	}

	if(!useMike || !canControl()) return;

	//Flag for death sequence.
	if(playerHealth <= 0) {
		playerState = PSTATE_DYING;
		pain = false;
		return;
	}

	//Recover from pain invincibility.
	if(pain && due(lastHitTime, PAIN_RECOVER_TIME)) {
		pain = false;
	}

	//Movement.
	recogniseThrust();
	applyMomentum();
	thrustState = zeroCoord();

	//Firing / auto-fire
	if(checkCommand(CMD_PLAYER_FIRE) || autoFire && gameState == STATE_GAME) {
		pew();
		playerShooting = true;
	} else {
		playerShooting = false;
		begunShooting = false;
	}

	//Draw player in correct position
	playerOrigin.x += momentumState.x;
	playerOrigin.y += momentumState.y;
}

void resetPlayer() {
	begunDyingRender = false;
	begunDyingGame = false;
	playerState = PSTATE_NORMAL;
	bubbleLastTime = clock();
	playerOrigin.y = 220;
	playerHealth = playerStrength;
	momentumState = zeroCoord();
	pain = false;
	bubbleFinished = false;
	animationInc = 0;

	playerOrigin = makeCoord(
			(screenBounds.x / 2),
			(int)(screenBounds.y * 0.9)
	);
}

void playerInit(void) {
	momentumInc = PLAYER_MAX_SPEED / MOMENTUM_INC_DIVISOR;

	//Calculate (in advance) the map boundary limitations.
	movementBounds = makeRect(
			PLAYER_SIZE.x / 2,
			PLAYER_SIZE.y / 2,
			screenBounds.x - (PLAYER_SIZE.x / 2),
			screenBounds.y - (PLAYER_SIZE.y / 2)
	);

	resetPlayer();
	playerAnimate();
}