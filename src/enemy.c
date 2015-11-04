#include <time.h>
#include "enemy.h"
#include "assets.h"
#include "renderer.h"
#include "player.h"
#include "common.h"
#include "weapon.h"
#include "item.h"
#include "hud.h"

//TODO: Reintroduce sprite flickering, but localised on upcoming 'Character' struct (keep original Surface on asset record?).
//TODO: Reinstate proper flicker effect with white alt graphic.
//TODO: Centralise enemy sprite frames into own structure (e.g. frame counts).
//TODO: Rather than frelling about making asset name descriptions on every frame - just keep a pointer to the right asset in an animation array.
//TODO: Shoudn't need an initial sprite for the enemy -- since we're setting it when animated(?)

typedef struct {
	Coord origin;
	Coord parallax;
	int animFrame;
	EnemyType enemyType;
} EnemyShot;

#define MAX_SHOTS 20

static int gameTime;

typedef struct {
	float time;
	int x;
	EnemyType type;
} EnemySpawn;

static int ENEMY_SPEED = 9;
static int ENEMY_SPAWN_INTERVAL = 27;
static double HIT_KNOCKBACK = 5.0;
static double COLLIDE_DAMAGE = 1;
static double SHOT_DAMAGE = 1;
static double SHOT_HZ = 1000 / 0.25;
static double SHOT_SPEED = 1.3;

//TODO: Homing.
//TODO: Shooter chance ratio.

Enemy enemies[MAX_ENEMIES];
const int ENEMY_BOUND = 32;
static int enemyCount;
static const int DISK_IDLE_FRAMES = 12;
static const int VIRUS_IDLE_FRAMES = 6;
static const int CD_IDLE_FRAMES = 4;
static const int BUG_IDLE_FRAMES = 6;
static const int DEATH_FRAMES = 7;
static const double ENEMY_HEALTH = 4.0;

static EnemyShot enemyShots[MAX_SHOTS];
static int enemyShotCount;
static const int ENEMY_SHOT_BOUND = 24;
static const int MAX_VIRUS_SHOT_FRAMES = 4;
static const int MAX_PLASMA_SHOT_FRAMES = 2;
static const double DIFFICULTY_INTERVAL = 5 / 1000;
static long lastDifficultyTime;

static bool invalidEnemy(Enemy *enemy) {
	return
		enemy->animFrame == 0 ||
		enemy->parallax.y > screenBounds.y + ENEMY_BOUND / 2;
}

static bool invalidEnemyShot(EnemyShot *enemyShot) {
	return
		(enemyShot->origin.x == 0 &&
		enemyShot->origin.y == 0) ||
		enemyShot->origin.y > screenBounds.y + ENEMY_SHOT_BOUND/2;
}

static Enemy nullEnemy(void) {
	Enemy enemy = { };
	return enemy;
}

static EnemyShot nullEnemyShot(void) {
	EnemyShot enemyShot = { };
	return enemyShot;
}

void hitEnemy(Enemy* enemy, double damage) {
	play("Hit_Hurt9.wav");
	enemy->hitAnimate = true;
	enemy->health -= damage;

	//Apply knockback by directly altering enemy's Y coordinate (improve with lerping).
	enemy->origin.y -= HIT_KNOCKBACK;
}

void enemyShadowFrame(void) {
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
		char frameFile[50];
		char* filename = enemyShots[i].enemyType == ENEMY_VIRUS ?
			"virus-shot.png" :
			"shot-blue-%02d.png";

		sprintf(frameFile, filename, enemyShots[i].animFrame);
		SDL_Texture *shotTexture = getTextureVersion(frameFile, ASSET_SHADOW);
		Sprite shotShadow = makeSprite(shotTexture, zeroCoord(), SDL_FLIP_VERTICAL);
		Coord shadowCoord = parallax(enemyShots[i].parallax, PARALLAX_SUN, PARALLAX_LAYER_SHADOW, PARALLAX_X, PARALLAX_SUBTRACTIVE);
		shadowCoord.y += STATIC_SHADOW_OFFSET;
		drawSpriteAbs(shotShadow, shadowCoord);
	}
}

void enemyRenderFrame(void) {

	//Render out not-null enemies wherever they may be.
	for(int i=0; i < MAX_ENEMIES; i++) {
		if(invalidEnemy(&enemies[i]) || !enemies[i].initialFrameChosen) continue;
		drawSpriteAbs(enemies[i].sprite, enemies[i].parallax);
	}

	//Shots
	for(int i=0; i < MAX_SHOTS; i++) {
		if(invalidEnemyShot(&enemyShots[i])) continue;

		char frameFile[50];
		char* filename = enemyShots[i].enemyType == ENEMY_VIRUS ?
			"virus-shot.png" :
			"shot-blue-%02d.png";

		sprintf(frameFile, filename, enemyShots[i].animFrame);
		SDL_Texture *shotTexture = getTexture(frameFile);
		Sprite shotSprite = makeSprite(shotTexture, zeroCoord(), SDL_FLIP_VERTICAL);

		drawSpriteAbs(shotSprite, enemyShots[i].parallax);
	}
}

void animateEnemy(void) {

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

				//Spawn powerup (only in-game, though)
				if(gameState == STATE_GAME){
					if(chance(2) && canSpawn(TYPE_HEALTH)) {
						spawnItem(enemies[i].origin, TYPE_HEALTH);
					}else if(chance(1) && canSpawn(TYPE_WEAPON)) {
						spawnItem(enemies[i].origin, TYPE_WEAPON);
					}else if(chance(10)){
						spawnItem(enemies[i].origin, TYPE_FRUIT);
					}else{
						spawnItem(enemies[i].origin, TYPE_COIN);
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

void spawnEnemy(int x, int y, EnemyType type) {
	//Limit Enemy count to array size by looping over the top.
	if(enemyCount == MAX_ENEMIES) enemyCount = 0;

	//Note: We don't bother setting/choosing the initial frame, since all this logic is
	// centralised in Animate. As a result, we wait until that's been done before considering
	// it a candidate for rendering (shadowFrame included).

	//Build it.
	Enemy enemy = {
		makeCoord(x, y),
		makeCoord(x, y),			//same as origin, so rollcall works OK.
		{ },
		false,
		ENEMY_HEALTH,
		ENEMY_HEALTH,
		1,
		ENEMY_ANIMATION_IDLE,
		false,
		type,
		0,
		ENEMY_SPEED * 0.1,
//		(random(ENEMY_SPEED_MIN, ENEMY_SPEED_MAX)) * 0.1,
		"",
		false,
		false
	};

	//Add it to the list of renderables.
	enemies[enemyCount++] = enemy;
};

static void spawnShot(Enemy* enemy) {
	play("Laser_Shoot34.wav");

	//TODO: Remove initial texture (not needed - we animate)
	SDL_Texture* texture = getTexture("shot-blue-01.png");
	Sprite defaultSprite = makeSprite(texture, zeroCoord(), SDL_FLIP_VERTICAL);
	EnemyShot shot = {
		enemy->origin,
		enemy->parallax,
		1,
		enemy->type
	};

	enemyShots[enemyShotCount++] = shot;
}

void resetEnemies() {
	memset(enemies, 0, sizeof(enemies));
	memset(enemyShots, 0, sizeof(enemyShots));
	enemyCount = 0;
	enemyShotCount = 0;
	lastDifficultyTime = 0;

	ENEMY_SPEED = 9;
	ENEMY_SPAWN_INTERVAL = 27;
	HIT_KNOCKBACK = 5.0;
	COLLIDE_DAMAGE = 1;
	SHOT_DAMAGE = 1;
	SHOT_HZ = 1000 / 0.25;
	SHOT_SPEED = 1.3;
}

static double rollSine[5] = { 0.0, 1.25, 2.5, 3.75, 5.0 };

void enemyGameFrame(void) {

//	static int ENEMY_SPEED = 10;
//	static int ENEMY_SPAWN_INTERVAL = 25;
//	static double HIT_KNOCKBACK = 5.0;
//	static double COLLIDE_DAMAGE = 1;
//	static double SHOT_DAMAGE = 1;

	//Bob enemies in sine pattern.
	switch(gameState) {
		case STATE_INTRO:
		case STATE_TITLE:
			for(int i=0; i < enemyCount; i++) {
				//Increment, looping on 2Pi radians (360 degrees)
				enemies[i].parallax.y = sineInc(enemies[i].origin.y, &rollSine[i], 0.1, 8);
			}
	}

	//Check for killed.
	switch(gameState) {
		case STATE_INTRO:
		case STATE_GAME:
		case STATE_TITLE:
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

	if(gameState != STATE_GAME) return;

	if(lastDifficultyTime == 0) {
		lastDifficultyTime = clock();
	}

	if(due(lastDifficultyTime, 30000)) {
		ENEMY_SPEED *= 1.1;
		ENEMY_SPAWN_INTERVAL /= 1.1;
//		HIT_KNOCKBACK /= 1.1;
		SHOT_DAMAGE *= 1.1;
		COLLIDE_DAMAGE *= 1.1;
		SHOT_SPEED *= 1.1;

		lastDifficultyTime = clock();
	}

	//Spawn new enemies at random positions, at a given time increment
	if(gameTime % ENEMY_SPAWN_INTERVAL == 0) {
		int xSpawnPos = random(0, (int)screenBounds.x + 33);			//HACK!
		spawnEnemy(
			xSpawnPos,
			-ENEMY_BOUND,
			(EnemyType)random(0, 4)
		);
	}

	//Scroll the enemies down the screen
	for(int i=0; i < MAX_ENEMIES; i++) {
		//Skip zeroed, or exploding (the explosion can stay where it is).
		if(invalidEnemy(&enemies[i]) || enemies[i].dying) continue;

		//Otherwise, scroll them down the screen
		enemies[i].origin.y += enemies[i].speed;

		//Set parallax.
		enemies[i].parallax = parallax(enemies[i].origin, PARALLAX_PAN, PARALLAX_LAYER_FOREGROUND, PARALLAX_X, PARALLAX_ADDITIVE);

		//Have we hit the player? (pass through if dying)
		Rect enemyBound = makeSquareBounds(enemies[i].parallax, ENEMY_BOUND);
		if(inBounds(playerOrigin, enemyBound)) {
			hitPlayer(COLLIDE_DAMAGE);
			hitEnemy(&enemies[i], playerStrength);
		}

		//Spawn shots.
		if(	(enemies[i].type == ENEMY_VIRUS || enemies[i].type == ENEMY_CD) &&
			timer(&enemies[i].lastShotTime, SHOT_HZ)
		) {
			if(enemyShotCount == MAX_SHOTS) enemyShotCount = 0;
			spawnShot(&enemies[i]);
		}
	}

	//Shots.
	for(int i=0; i < MAX_SHOTS; i++) {
		if(invalidEnemyShot(&enemyShots[i])) continue;

		//Scroll down screen.
		enemyShots[i].origin.y += SHOT_SPEED;

		//Parallax it
		enemyShots[i].parallax = parallax(enemyShots[i].origin, PARALLAX_PAN, PARALLAX_LAYER_FOREGROUND, PARALLAX_X, PARALLAX_ADDITIVE);

		//Hit the player?
		Rect shotBounds = makeSquareBounds(enemyShots[i].parallax, ENEMY_SHOT_BOUND);
		if(playerState != PSTATE_DYING && inBounds(playerOrigin, shotBounds)) {
			hitPlayer(SHOT_DAMAGE);
			enemyShots[i] = nullEnemyShot();
		}
	}

	gameTime++;
}

EnemySpawn makeSpawn(long time, int x, EnemyType type) {
	EnemySpawn s = { time, x, type };
	return s;
}

void enemyInit(void) {
	resetEnemies();
	animateEnemy();
}
