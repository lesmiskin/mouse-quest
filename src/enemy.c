#include <time.h>
#include "renderer.h"
#include "common.h"
#include "weapon.h"
#include "enemy.h"
#include "assets.h"
#include "player.h"
#include "item.h"
#include "hud.h"
#include "sound.h"

//TODO: Plume flicker
//TODO: Coin pickup sparkle (see: Super Mario World)

#define MAX_SHOTS 500
#define MAX_SPAWNS 10

typedef struct {
	Coord origin;
	Coord parallax;
	int animFrame;
	EnemyType enemyType;
	Coord homeTarget;
	bool homing;
} EnemyShot;

typedef struct {
	double lastSpawnTime;
	int x;
	EnemyType type;
	int max;
	int count;
	double speed;
} EnemySpawn;

EnemySpawn spawns[MAX_SPAWNS];
Enemy enemies[MAX_ENEMIES];
int spawnInc = 0;
const double HEALTH_LIGHT = 2.0;
const double HEALTH_HEAVY = 5.0;

static int gameTime;
static int enemyCount;
static int enemyShotCount;
static EnemyShot enemyShots[MAX_SHOTS];
static double rollSine[5] = { 0.0, 1.25, 2.5, 3.75, 5.0 };

//Debugging settings...
//static int FORMATION_INTERVAL = 20;
//static double ENEMY_SPEED = 0.4;
//static double ENEMY_SPEED_FAST = 0.4;
//static const double HEALTH_LIGHT = 1.0;
//static double SHOT_HZ = 500;

static int FORMATION_INTERVAL = 75;
static double ENEMY_SPEED = 0.9;

static double ENEMY_SPEED_FAST = 1.8;
static double SHOT_HZ = 1000;
static double SHOT_SPEED = 2;
static double SHOT_DAMAGE = 1;

const int ENEMY_BOUND = 26;
static const int ENEMY_SHOT_BOUND = 8;
static int ENEMY_DISTANCE = 22;
static double HIT_KNOCKBACK = 0.0;
static double COLLIDE_DAMAGE = 1;

static const int MAGNET_IDLE_FRAMES = 8;
static const int DISK_IDLE_FRAMES = 12;
static const int VIRUS_IDLE_FRAMES = 6;
static const int CD_IDLE_FRAMES = 4;
static const int BUG_IDLE_FRAMES = 6;
static const int DEATH_FRAMES = 7;
static const int MAX_VIRUS_SHOT_FRAMES = 4;
static const int MAX_PLASMA_SHOT_FRAMES = 2;

bool invalidEnemy(Enemy *enemy) {
	return
			enemy->animFrame == 0 ||
			enemy->parallax.y > screenBounds.y + ENEMY_BOUND / 2;
}

static bool invalidEnemyShot(EnemyShot *enemyShot) {
	return
			(enemyShot->origin.x == 0 &&
			 enemyShot->origin.y == 0) ||
			(enemyShot->parallax.y > screenBounds.y + ENEMY_SHOT_BOUND/2 ||
			 enemyShot->parallax.x > screenBounds.x + ENEMY_SHOT_BOUND/2 ||
			 enemyShot->parallax.x < 0 - ENEMY_SHOT_BOUND/2
			);
}

static Enemy nullEnemy() {
	Enemy enemy = { };
	return enemy;
}

static EnemyShot nullEnemyShot() {
	EnemyShot enemyShot = { };
	return enemyShot;
}

void hitEnemy(Enemy* enemy, double damage, bool collision) {
	play("Hit_Hurt9.wav");
	enemy->hitAnimate = true;
	enemy->health -= damage;
	enemy->collided = collision;

	//Apply knockback by directly altering enemy's Y coordinate (improve with lerping).
	enemy->origin.y -= HIT_KNOCKBACK;
}

void enemyShadowFrame() {
	//Render enemy shadows first (so everything else is above them).
	for(int i=0; i < MAX_ENEMIES; i++) {
		if(invalidEnemy(&enemies[i]) || !enemies[i].initialFrameChosen) continue;

		//Draw shadow, if a shadow version exists for the current frame.
		SDL_Texture* shadowTexture = getTextureVersion(enemies[i].frameName, ASSET_SHADOW);
		if(shadowTexture == NULL) continue;

		Sprite shadow = makeSprite(shadowTexture, zeroCoord(), SDL_FLIP_NONE);
		Coord shadowCoord = parallax(enemies[i].parallax, PARALLAX_SUN, PARALLAX_LAYER_SHADOW, PARALLAX_X, PARALLAX_SUBTRACTIVE);
		shadowCoord.y += STATIC_SHADOW_OFFSET;

		drawSpriteAbs(shadow, shadowCoord);
	}

	//Shot shadows
	for(int i=0; i < MAX_SHOTS; i++) {
		if(invalidEnemyShot(&enemyShots[i])) continue;

		//TODO: Fix duplication with enemyRenderFrame here (keep filename?)
		SDL_Texture *shotTexture = getTextureVersion("virus-shot.png", ASSET_SHADOW);
		Sprite shotShadow = makeSprite(shotTexture, zeroCoord(), SDL_FLIP_VERTICAL);
		Coord shadowCoord = parallax(enemyShots[i].parallax, PARALLAX_SUN, PARALLAX_LAYER_SHADOW, PARALLAX_X, PARALLAX_SUBTRACTIVE);
		shadowCoord.y += STATIC_SHADOW_OFFSET;
		drawSpriteAbs(shotShadow, shadowCoord);
	}
}

void enemyRenderFrame() {

	//Render out not-null enemies wherever they may be.
	for(int i=0; i < MAX_ENEMIES; i++) {
		if(invalidEnemy(&enemies[i]) || !enemies[i].initialFrameChosen) continue;
		drawSpriteAbs(enemies[i].sprite, enemies[i].parallax);
	}

	//Shots
	for(int i=0; i < MAX_SHOTS; i++) {
		if(invalidEnemyShot(&enemyShots[i])) continue;

		SDL_Texture *shotTexture = getTexture("virus-shot.png");
		Sprite shotSprite = makeSprite(shotTexture, zeroCoord(), SDL_FLIP_VERTICAL);
		drawSpriteAbs(shotSprite, enemyShots[i].parallax);
	}
}

void animateEnemy() {

	//Render out not-null enemies wherever they may be.
	for(int i=0; i < MAX_ENEMIES; i++) {
		if (invalidEnemy(&enemies[i])) continue;

		char frameFile[50];
		char* animGroupName = NULL;
		AssetVersion frameVersion = ASSET_DEFAULT;
		int maxFrames = 0;

		//Death animation
		if(enemies[i].dying) {
			//Switch over from idle (check what sequence we're currently on).
			if(enemies[i].animSequence != ENEMY_ANIMATION_DEATH){
				enemies[i].animSequence = ENEMY_ANIMATION_DEATH;
				enemies[i].animFrame = 1;

				//Spawn powerup (only in-game, and punish collisions)
				if(gameState == STATE_GAME && !enemies[i].collided){
					if(chance(5) && canSpawn(TYPE_WEAPON)) {
						spawnItem(enemies[i].formationOrigin, TYPE_WEAPON);
					}else if(chance(3) && canSpawn(TYPE_HEALTH)) {
						spawnItem(enemies[i].formationOrigin, TYPE_HEALTH);
					}else if(chance(15)){
						spawnItem(enemies[i].formationOrigin, TYPE_FRUIT);
					}else{
						spawnItem(enemies[i].formationOrigin, TYPE_COIN);
					}
				}
				//Zero if completely dead.
			}else if(enemies[i].animFrame == DEATH_FRAMES){
				enemies[i] = nullEnemy();
				raiseScore(10, false);
				continue;
			}

			animGroupName = "exp-%02d.png";
		}
			//Regular idle animation.
		else{
			//Being hit - change frame variant.
			if(enemies[i].hitAnimate && !enemies[i].wasHitLastFrame){
				enemies[i].hitAnimate = false;			//reset hit state.
				frameVersion = ASSET_HIT;
				enemies[i].wasHitLastFrame = true;
			}else{
				enemies[i].wasHitLastFrame = false;
			}

			//Select frames based on enemy type.
			switch(enemies[i].type) {
				case ENEMY_DISK:
					animGroupName = "disk-%02d.png";
					maxFrames = DISK_IDLE_FRAMES;
					break;
				case ENEMY_DISK_BLUE:
					animGroupName = "disk-blue-%02d.png";
					maxFrames = DISK_IDLE_FRAMES;
					break;
				case ENEMY_BUG:
					animGroupName = "bug-%02d.png";
					maxFrames = BUG_IDLE_FRAMES;
					break;
				case ENEMY_CD:
					animGroupName = "cd-%02d.png";
					maxFrames = CD_IDLE_FRAMES;
					break;
				case ENEMY_VIRUS:
					animGroupName = "virus-%02d.png";
					maxFrames = VIRUS_IDLE_FRAMES;
					break;
				case ENEMY_MAGNET:
					animGroupName = "magnet-%02d.png";
					maxFrames = MAGNET_IDLE_FRAMES;
					break;
				default:
					fatalError("Error", "No frames specified for enemy type");
			}
		}

		//Select animation frame from above group.
		sprintf(frameFile, animGroupName, enemies[i].animFrame);
		SDL_Texture* texture = getTextureVersion(frameFile, frameVersion);
		enemies[i].sprite = makeSprite(texture, zeroCoord(), SDL_FLIP_NONE);

		//Record animation frame for shadowing
		strncpy(enemies[i].frameName, frameFile, sizeof(frameFile));

		//Increment frame count for next frame (NB: absolute death will never get here).
		enemies[i].animFrame = enemies[i].animFrame == maxFrames ? 1 : enemies[i].animFrame + 1;

		//Flag as being OK to render, now the initial frame is chosen
		if(!enemies[i].initialFrameChosen) enemies[i].initialFrameChosen = true;
	}

	//Animate enemy projectiles
	for(int i=0; i < MAX_SHOTS; i++) {
		if (invalidEnemyShot(&enemyShots[i])) continue;
		if(	(enemyShots[i].enemyType == ENEMY_VIRUS && enemyShots[i].animFrame == MAX_VIRUS_SHOT_FRAMES) ||
			   (enemyShots[i].enemyType == ENEMY_CD && enemyShots[i].animFrame == MAX_PLASMA_SHOT_FRAMES)){
			enemyShots[i].animFrame = 0;
		}
		if(enemyShots[i].animFrame == MAX_VIRUS_SHOT_FRAMES) enemyShots[i].animFrame = 0;
		enemyShots[i].animFrame++;
	}
}

void spawnFormation(int x, EnemyType enemyType, int qty, double speed) {
	if(spawnInc == MAX_SPAWNS) spawnInc = 0;

	EnemySpawn s = { clock(), x, enemyType, qty, 0, speed };
	spawns[spawnInc++] = s;
}

void spawnEnemy(int x, int y, EnemyType type, EnemyPattern movement, EnemyCombat combat, double speed, double speedX, double swayInc, double health) {
	//Limit Enemy count to array size by looping over the top.
	if(enemyCount == MAX_ENEMIES) enemyCount = 0;

	//Note: We don't bother setting/choosing the initial frame, since all this logic is
	// centralised in Animate. As a result, we wait until that's been done before considering
	// it a candidate for rendering (shadowFrame included).

	//Build it.
	Enemy enemy = {
			makeCoord(x, y),
			makeCoord(x, y),			//same as origin, so rollcall works OK.
			makeCoord(x, y),
			{ },
			false,
			health,
			health,
			1,
			ENEMY_ANIMATION_IDLE,
			false,
			type,
			0,
			speed,
			speedX,
			"",
			false,
			false,
			movement,
			combat,
			swayInc,
			swayInc,
			zeroCoord(),
			false,
			clock()
	};

	//Add it to the list of renderables.
	enemies[enemyCount++] = enemy;
};

static void spawnShot(Enemy* enemy) {
	play("Laser_Shoot34.wav");

	Coord adjustedShotParallax = parallax(enemy->formationOrigin, PARALLAX_PAN, PARALLAX_LAYER_FOREGROUND, PARALLAX_XY, PARALLAX_ADDITIVE);
	Coord homingStep = getStep(playerOrigin, adjustedShotParallax, SHOT_SPEED, true);

	//TODO: Remove initial texture (not needed - we animate)
	SDL_Texture* texture = getTexture("virus-shot.png");
	makeSprite(texture, zeroCoord(), SDL_FLIP_NONE);
	EnemyShot shot = {
		deriveCoord(enemy->origin, 0, 8),
		enemy->parallax,
		1,
		enemy->type,
		homingStep,
		enemy->combat == COMBAT_SHOOTER_HOMING
	};

	enemyShots[enemyShotCount++] = shot;
}

void resetEnemies() {
	memset(enemies, 0, sizeof(enemies));
	memset(enemyShots, 0, sizeof(enemyShots));
	enemyCount = 0;
	enemyShotCount = 0;
}

void enemyGameFrame() {
	//Bob enemies in sine pattern.
	switch(gameState) {
		case STATE_INTRO:
		case STATE_TITLE:
			for(int i=0; i < enemyCount; i++) {
				//Increment, looping on 2Pi radians (360 degrees)
				enemies[i].parallax.y = sineInc(enemies[i].origin.y, &rollSine[i], 0.125, 8);
			}
	}

	//Check for killed.
	switch(gameState) {
		case STATE_INTRO:
		case STATE_GAME:
			for (int i = 0; i < MAX_ENEMIES; i++) {
				//Skip zeroed, or exploding (the explosion can stay where it is).
				if (invalidEnemy(&enemies[i]) || enemies[i].dying) continue;

				//Flag for death sequence.
				if (enemies[i].health <= 0) {
					play(chance(50) ? "Explosion14.wav" : "Explosion3.wav");
					enemies[i].dying = true;
				}
			}
	}

	if(gameState != STATE_GAME && gameState != STATE_GAME_OVER) return;

	//Scroll the enemies down the screen
	for(int i=0; i < MAX_ENEMIES; i++) {
		//Skip zeroed, or exploding (the explosion can stay where it is).
		if(invalidEnemy(&enemies[i]) || enemies[i].dying) continue;

		//Scroll them down the screen
		enemies[i].origin.y += enemies[i].speed;

		switch(enemies[i].movement) {

			case P_CURVE_RIGHT:
				if(dueBetween(enemies[i].spawnTime, 750, 1600)) enemies[i].origin.x += enemies[i].speedX;
				if(dueBetween(enemies[i].spawnTime, 2250, 3250)) enemies[i].origin.x -= enemies[i].speedX;
				enemies[i].formationOrigin = enemies[i].origin;
				break;
			case P_CURVE_LEFT:
				if(dueBetween(enemies[i].spawnTime, 750, 1600)) enemies[i].origin.x -= enemies[i].speedX;
				if(dueBetween(enemies[i].spawnTime, 2250, 3250)) enemies[i].origin.x += enemies[i].speedX;
				enemies[i].formationOrigin = enemies[i].origin;
				break;

			case P_CROSSOVER_RIGHT:
				if(dueBetween(enemies[i].spawnTime, 750, 2500)) enemies[i].origin.x += enemies[i].speedX;
				enemies[i].formationOrigin = enemies[i].origin;
				break;
			case P_CROSSOVER_LEFT:
				if(dueBetween(enemies[i].spawnTime, 750, 2500)) enemies[i].origin.x -= enemies[i].speedX;
				enemies[i].formationOrigin = enemies[i].origin;
				break;

			case P_STRAFE_RIGHT:
				if(due(enemies[i].spawnTime, 500)) enemies[i].origin.x += enemies[i].speedX;
				enemies[i].formationOrigin = enemies[i].origin;
				break;
			case P_STRAFE_LEFT:
				if(due(enemies[i].spawnTime, 500)) enemies[i].origin.x -= enemies[i].speedX;
				enemies[i].formationOrigin = enemies[i].origin;
				break;

			case PATTERN_CIRCLE:
				//TODO: Find out why swayIncX and swayIncY need to be swapped :p
				enemies[i].formationOrigin.y = sineInc(enemies[i].origin.y, &enemies[i].swayIncX, 0.04, 21);
				enemies[i].formationOrigin.x = cosInc(enemies[i].origin.x, &enemies[i].swayIncY, 0.04, 21);
				break;
			case PATTERN_SNAKE:
				enemies[i].formationOrigin.x = sineInc(enemies[i].origin.x, &enemies[i].swayIncX, 0.075, 12);
				enemies[i].formationOrigin.y = enemies[i].origin.y;
				break;
			case PATTERN_BOB:
				enemies[i].formationOrigin.x = enemies[i].origin.x;
				enemies[i].formationOrigin.y = sineInc(enemies[i].origin.y, &enemies[i].swayIncY, 0.075, 12);;
				break;
			default:
				enemies[i].formationOrigin = enemies[i].origin;
				break;
		}

		//Set parallax against the formation origin.
		enemies[i].parallax = parallax(enemies[i].formationOrigin, PARALLAX_PAN, PARALLAX_LAYER_FOREGROUND, PARALLAX_XY, PARALLAX_ADDITIVE);

		//Have we hit the player? (pass through if dying)
		Rect enemyBound = makeSquareBounds(enemies[i].parallax, ENEMY_BOUND);
		if(inBounds(playerOrigin, enemyBound)) {
			hitPlayer(COLLIDE_DAMAGE);
			hitEnemy(&enemies[i], playerStrength, true);
		}

		//Spawn shots.
		if(enemies[i].combat == COMBAT_SHOOTER &&
		   timer(&enemies[i].lastShotTime, SHOT_HZ) &&
		   enemies[i].origin.y > 0
		) {
			if(enemyShotCount == MAX_SHOTS) enemyShotCount = 0;
			spawnShot(&enemies[i]);
		}
	}

	//Shots.
	for(int i=0; i < MAX_SHOTS; i++) {
		if(invalidEnemyShot(&enemyShots[i])) continue;

		if(enemyShots->homing) {
			//Home in on target vector.
			enemyShots[i].origin.x += enemyShots[i].homeTarget.x;
			enemyShots[i].origin.y += enemyShots[i].homeTarget.y;
		}else{
			//Otherwise fire straight down.
			enemyShots[i].origin.y += SHOT_SPEED;
		}

		//Parallax it
		enemyShots[i].parallax = parallax(enemyShots[i].origin, PARALLAX_PAN, PARALLAX_LAYER_FOREGROUND, PARALLAX_XY, PARALLAX_ADDITIVE);

		//Hit the player?
		Rect shotBounds = makeSquareBounds(enemyShots[i].parallax, ENEMY_SHOT_BOUND);
		if(playerState != PSTATE_DYING && inBounds(playerOrigin, shotBounds)) {
			hitPlayer(SHOT_DAMAGE);
			enemyShots[i] = nullEnemyShot();
		}
	}
}

void enemyInit() {
	resetEnemies();
	animateEnemy();
}