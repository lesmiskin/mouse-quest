#include "myc.h"
#include <time.h>
#include "renderer.h"
#include "formations.h"
#include "enemy.h"
#include "assets.h"
#include "player.h"
#include "item.h"
#include "hud.h"
#include "sound.h"

#define MAX_SHOTS 500
#define MAX_SPAWNS 10
#define MAX_BOOMS 20

typedef struct {
	Coord origin;
	Coord parallax;
	int animFrame;
	EnemyType enemyType;
	Coord homeTarget;
	bool homing;
	bool isKey;
	double spinInc;
} EnemyShot;

typedef struct {
	Coord origin;
	int animFrame;
    double scale;
} Boom;

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
const double HEALTH_LIGHT = 1.5;
const double HEALTH_HEAVY = 5.0;
bool bossOnscreen = false;
double bossHealth = 0;

static int boomCount;
static Boom booms[MAX_BOOMS];
static int gameTime;
static int enemyCount;
static int enemyShotCount;
static EnemyShot enemyShots[MAX_SHOTS];
static double rollSine[5] = { 0.0, 1.25, 2.5, 3.75, 5.0 };

static double SHOT_BOSS_HZ = 100;
static double SHOT_HZ = 500;
static double SHOT_SPEED = 1;
static double BOSS_SHOT_SPEED = 2;
static double SHOT_DAMAGE = 1;

const int ENEMY_BOUND = 26;
static const int ENEMY_SHOT_BOUND = 8;
static double HIT_KNOCKBACK = 0.0;
static double COLLIDE_DAMAGE = 1;
static double BOSS_COLLIDE_DAMAGE = 1000;

static const int BOSS_IDLE_FRAMES = 12;
static const int MAGNET_IDLE_FRAMES = 8;
static const int CONE_IDLE_FRAMES = 12;
static const int DISK_IDLE_FRAMES = 12;
static const int VIRUS_IDLE_FRAMES = 6;
static const int CD_IDLE_FRAMES = 4;
static const int BUG_IDLE_FRAMES = 6;
static const int DEATH_FRAMES = 7;
static const int MAX_VIRUS_SHOT_FRAMES = 4;
static const int MAX_PLASMA_SHOT_FRAMES = 2;

double dieSpin;

static bool bossDeathDir = false;

bool invalidEnemy(Enemy *enemy) {
	// TODO: Use OnScreen/InBounds()?

	// Hack here to let intro boss start at the bottom of the screen.
	bool exceptionForBossIntro = enemy->type != ENEMY_BOSS_INTRO;

	return
		enemy->animFrame == 0 ||
		enemy->parallax.x < -ENEMY_BOUND*4 ||
		enemy->parallax.x > screenBounds.x + ENEMY_BOUND*4 ||
		(enemy->parallax.y > screenBounds.y + ENEMY_BOUND && exceptionForBossIntro);
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
	// Don't keep hitting boss if in pain (prevents kamikaze boss cheat)
	if(pain && enemy->type == ENEMY_BOSS && collision) {
		return;
	}

	play("Hit_Hurt9.wav");
	enemy->hitAnimate = true;
	enemy->health -= damage;
	enemy->collided = collision;

	//Apply knockback by directly altering enemy's Y coordinate (improve with lerping).
	enemy->origin.y -= HIT_KNOCKBACK;

	if(bossOnscreen) {
		bossHealth = enemy->health;
	}
}

void enemyShadowFrame() {

	//Render enemy shadows first (so everything else is above them).
	for(int i=0; i < MAX_ENEMIES; i++) {
		if(invalidEnemy(&enemies[i]) || !enemies[i].initialFrameChosen) continue;

        // Permit skipping boss rendering (e.g. delay after visual death).
        if(enemies[i].type == ENEMY_BOSS && !bossOnscreen) continue;

        //Draw shadow, if a shadow version exists for the current frame.
		SDL_Texture* shadowTexture = getTextureVersion(enemies[i].frameName, ASSET_SHADOW);
		if(shadowTexture == NULL) continue;

		Sprite shadow = makeSprite(shadowTexture, zeroCoord(), SDL_FLIP_NONE);
		Coord shadowCoord = parallax(enemies[i].parallax, PARALLAX_SUN, PARALLAX_LAYER_SHADOW, PARALLAX_X, PARALLAX_SUBTRACTIVE);
		shadowCoord.y += STATIC_SHADOW_OFFSET;

		drawSpriteAbsRotated(shadow, shadowCoord, dieSpin);
	}

	//Shot shadows
	for(int i=0; i < MAX_SHOTS; i++) {
		if(invalidEnemyShot(&enemyShots[i])) continue;

		//TODO: Fix duplication with enemyRenderFrame here (keep filename?)
		SDL_Texture *shotTexture;
		if(enemyShots[i].isKey) {
			shotTexture = getTextureVersion("key-a.png", ASSET_SHADOW);
		} else {
			shotTexture = getTextureVersion("virus-shot.png", ASSET_SHADOW);
		}
		Sprite shotShadow = makeSprite(shotTexture, zeroCoord(), SDL_FLIP_VERTICAL);
		Coord shadowCoord = parallax(enemyShots[i].parallax, PARALLAX_SUN, PARALLAX_LAYER_SHADOW, PARALLAX_X, PARALLAX_SUBTRACTIVE);
		shadowCoord.y += STATIC_SHADOW_OFFSET;
		drawSpriteAbs(shotShadow, shadowCoord);
	}
}

void enemyBackgroundRenderFrame() {
	// Just the enemies set to "background"
	for(int i=0; i < MAX_ENEMIES; i++) {
		if(invalidEnemy(&enemies[i]) || !enemies[i].initialFrameChosen || !enemies[i].inBackground) continue;
		drawSpriteAbs(enemies[i].sprite, enemies[i].parallax);
	}
}

void enemyRenderFrame() {
	//Render out not-null enemies wherever they may be.
	for(int i=0; i < MAX_ENEMIES; i++) {
		if(invalidEnemy(&enemies[i]) || !enemies[i].initialFrameChosen || enemies[i].inBackground) continue;
		
		// Permit skipping boss rendering (e.g. delay after visual death).
		if(enemies[i].type == ENEMY_BOSS && !bossOnscreen) continue;

        drawSpriteAbsRotated(enemies[i].sprite, enemies[i].parallax, dieSpin);
//		drawSpriteAbs(enemies[i].sprite, enemies[i].parallax);
	}

	//Shots
	for(int i=0; i < MAX_SHOTS; i++) {
		if(invalidEnemyShot(&enemyShots[i])) continue;

		SDL_Texture *shotTexture;
		if(enemyShots[i].isKey) {
			Sprite shotSprite = makeSimpleSprite("key-a.png");
			enemyShots[i].spinInc = enemyShots[i].spinInc > 360 ? 0 : enemyShots[i].spinInc + 5;
			drawSpriteAbsRotated(shotSprite, enemyShots[i].parallax, enemyShots[i].spinInc);
		} else {
			shotTexture = getTexture("virus-shot.png");
			Sprite shotSprite = makeSprite(shotTexture, zeroCoord(), SDL_FLIP_VERTICAL);
			drawSpriteAbs(shotSprite, enemyShots[i].parallax);
		}
	}

	// Booms
	for(int i=0; i < MAX_BOOMS; i++) {
		if (booms[i].origin.x == 0 && booms[i].origin.y == 0) continue;
		char frameFile[20];
		char* frameTemplate = "exp-%02d.png";

		sprintf(frameFile, frameTemplate, booms[i].animFrame);
		Coord boomParallax = parallax(booms[i].origin, PARALLAX_PAN, PARALLAX_LAYER_FOREGROUND, PARALLAX_XY, PARALLAX_ADDITIVE);
		drawSpriteAbsRotated2(makeSimpleSprite(frameFile), boomParallax, 0, booms[i].scale, booms[i].scale);
	}
}

void spawnBoom(Coord origin, double scale) {
	play(chance(50) ? "Explosion14.wav" : "Explosion3.wav");

	boomCount = boomCount > MAX_BOOMS-1 ? 0 : boomCount + 1;

	Boom boom = { origin, 1, (scale == 0.0 ? 1.0 : scale) };
	booms[boomCount] = boom;
}

void animateEnemy() {
	// Booms
	for(int i=0; i < MAX_BOOMS; i++) {
		if (booms[i].origin.x == 0 && booms[i].origin.y == 0) continue;
		booms[i].animFrame++;

		if(booms[i].animFrame == DEATH_FRAMES) {
			booms[i].origin.x = 0;
			booms[i].origin.y = 0;
			continue;
		}
	}

	//Render out not-null enemies wherever they may be.
	for(int i=0; i < MAX_ENEMIES; i++) {
		if (invalidEnemy(&enemies[i])) continue;

		char frameFile[50];
		char* animGroupName = NULL;
		AssetVersion frameVersion = ASSET_DEFAULT;
		int maxFrames = 0;

		//Freeze animation on dying bosses.
		if(enemies[i].dying && enemies[i].type == ENEMY_BOSS) {
			return;
		}

		//Death animation
		if(enemies[i].dying) {
			//Change animation sequence to explosion (except for bosses)
			if(enemies[i].animSequence != ENEMY_ANIMATION_DEATH){
				enemies[i].animSequence = ENEMY_ANIMATION_DEATH;
				enemies[i].animFrame = 1;

				//Spawn powerup (only in-game, and punish collisions)
				if(gameState == STATE_GAME && !enemies[i].collided){
//					if(chance(5) && canSpawn(TYPE_WEAPON)) {
//						spawnItem(enemies[i].formationOrigin, TYPE_WEAPON);
//					}else if(chance(3) && canSpawn(TYPE_HEALTH)) {
//						spawnItem(enemies[i].formationOrigin, TYPE_HEALTH);
//					}else if(chance(15)){
//						spawnItem(enemies[i].formationOrigin, TYPE_FRUIT);
//					}else{
					spawnItem(enemies[i].formationOrigin, TYPE_COIN);
				}
			}
			//Zero if completely dead.
			else if(enemies[i].animFrame == DEATH_FRAMES){
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
				case ENEMY_CONE:
                    animGroupName = "cone-%02d.png";
					maxFrames = CONE_IDLE_FRAMES;
					break;
				case ENEMY_MAGNET:
					animGroupName = "magnet-%02d.png";
					maxFrames = MAGNET_IDLE_FRAMES;
					break;
				case ENEMY_BOSS_INTRO:
					animGroupName = "keyboss-mini-%02d.png";
					maxFrames = 2;
					break;
				case ENEMY_BOSS:
					animGroupName = "keyboss-%02d.png";
					maxFrames = BOSS_IDLE_FRAMES;
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

void spawnEnemy(int x, int y, EnemyType type, EnemyPattern movement, EnemyCombat combat, double speed, double speedX, double swayInc, double health, double frequency, double ampMult) {
	// Limit Enemy count to array size by looping over the top.
	// Todo: Consider fixing need for >= (using == bugs out boss explosions sometimes)
	if(enemyCount >= MAX_ENEMIES) enemyCount = 0;

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
		clock(),
		clock(),
		false,
		0,
		false,
		type == ENEMY_BOSS ? BOSS_COLLIDE_DAMAGE : COLLIDE_DAMAGE,
		0,
		0,
		type == ENEMY_BOSS_INTRO,	//nonInteractive
		type == ENEMY_BOSS_INTRO,	//inBackground
		frequency,
		ampMult
	};

	//Add it to the list of renderables.
	enemies[enemyCount++] = enemy;

	if(type == ENEMY_BOSS) {
		bossOnscreen = true;
		bossHealth = health;
	}
};

static void spawnShot(Enemy* enemy) {
	if(enemyShotCount == MAX_SHOTS) enemyShotCount = 0;

	play("Laser_Shoot34.wav");

	Coord adjustedShotParallax = parallax(enemy->formationOrigin, PARALLAX_PAN, PARALLAX_LAYER_FOREGROUND, PARALLAX_XY, PARALLAX_ADDITIVE);
	Coord homingStep = getStep(playerOrigin, adjustedShotParallax, enemy->type == ENEMY_BOSS ? BOSS_SHOT_SPEED : SHOT_SPEED, true);

	//TODO: Remove initial texture (not needed - we animate)
	SDL_Texture* texture = getTexture("virus-shot.png");
	makeSprite(texture, zeroCoord(), SDL_FLIP_NONE);

	//HACK!
	if(enemy->type == ENEMY_BOSS) {
		EnemyShot shot = {
			deriveCoord(enemy->formationOrigin, 0, 16),
			enemy->parallax,
			1,
			enemy->type,
			homingStep,
			enemy->combat == COMBAT_HOMING,
			enemy->type == ENEMY_BOSS,	// HACK!
			0
		};
		enemyShots[enemyShotCount++] = shot;
	} else {
		EnemyShot shot = {
			deriveCoord(enemy->origin, 0, 8),
			enemy->parallax,
			1,
			enemy->type,
			homingStep,
			enemy->combat == COMBAT_HOMING,
			enemy->type == ENEMY_BOSS,	// HACK!
			0
		};
		enemyShots[enemyShotCount++] = shot;
	}
}

void resetEnemies() {
	memset(enemies, 0, sizeof(enemies));
	memset(enemyShots, 0, sizeof(enemyShots));
	enemyCount = 0;
	enemyShotCount = 0;
	boomCount = 0;
	bossOnscreen = false;
	bossHealth = 0;
    dieSpin = 0;
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
					if(enemies[i].type == ENEMY_BOSS) {
						Mix_PauseMusic();
						enemies[i].sprite = makeSimpleSprite("keyboss-05.png");
					}else{
						play(chance(50) ? "Explosion14.wav" : "Explosion3.wav");
					}

					enemies[i].dying = true;
					enemies[i].fatalTime = clock();
					enemies[i].boomTime = clock();
				}
			}
	}

	if(gameState != STATE_GAME && gameState != STATE_GAME_OVER) return;

	//Scroll the enemies down the screen
	for(int i=0; i < MAX_ENEMIES; i++) {
		//Skip zeroed.
		if(invalidEnemy(&enemies[i])) continue;

		// Boss explosions.
		if(enemies[i].dying && enemies[i].type == ENEMY_BOSS) {
			// Final death.
			if(due(enemies[i].fatalTime, 6500)) {
				enemies[i] = nullEnemy();
				triggerState(STATE_LEVEL_COMPLETE);
				continue;
			}else if(bossOnscreen && due(enemies[i].fatalTime, 4000)) {
                // Final explosion to hide sprite vanishing.
                spawnBoom(deriveCoord(enemies[i].formationOrigin, -20, -15), 1);
                spawnBoom(deriveCoord(enemies[i].formationOrigin, 20, -15), 1);
                spawnBoom(deriveCoord(enemies[i].formationOrigin, -35, 0), 1);
                spawnBoom(enemies[i].formationOrigin, 1);
                spawnBoom(deriveCoord(enemies[i].formationOrigin, 35, 0), 1);
                spawnBoom(deriveCoord(enemies[i].formationOrigin, -20, 15), 1);
                spawnBoom(deriveCoord(enemies[i].formationOrigin, 20, 15), 1);
				bossOnscreen = false;
			}
			// Explosion and shaking drama.
			else if(bossOnscreen && enemies[i].dying && due(enemies[i].boomTime, 75)) {

                if(due(enemies[i].fatalTime, 1000)) {

                    // LOTS of explosions.
                    for(int j=0; j < 2; j++) {
                        spawnBoom(deriveCoord(enemies[i].formationOrigin, randomMq(-60, 60), randomMq(-15, 15)), 1);
                        enemies[i].boomTime = clock();
                    }

                    // Pain face >D
                    SDL_Texture* tex = getTextureVersion("keyboss-01.png", ASSET_HIT);
                    enemies[i].sprite = makeSprite(tex, zeroCoord(), SDL_FLIP_NONE);

                    // Throw some rewards out.
                    if(chance(66)) {
                        throwItem(
                                deriveCoord(enemies[i].formationOrigin, randomMq(-50, 50), randomMq(5, 15)),
                                chance(80) ? TYPE_COIN : TYPE_FRUIT,
                                chance(50) ? -1 : 1,
                                randomMq(160, 220) / 100.0,
                                randomMq(2, 16) / 10.0
                        );
                    }

                    // Shake 'n' bake.
                    enemies[i].formationOrigin.x += bossDeathDir ? 3 : -3;
                    enemies[i].formationOrigin.y += 2;	// slight drop down.
                    dieSpin += 0.5;

                }else{
                    // Explosions.
                    spawnBoom(deriveCoord(enemies[i].formationOrigin, randomMq(-60, 60), randomMq(-15, 15)), 1);
                    enemies[i].boomTime = clock();

                    enemies[i].formationOrigin.x += bossDeathDir ? 3 : -3;
                }

				// Boss shaking.
				bossDeathDir = !bossDeathDir;
			}
		}

		//Set parallax against the formation origin.
		enemies[i].parallax = parallax(enemies[i].formationOrigin, PARALLAX_PAN, PARALLAX_LAYER_FOREGROUND, PARALLAX_XY, PARALLAX_ADDITIVE);

		// If dying - nothing more to do here (explosions can stay where they are).
		if(enemies[i].dying) continue;

		//Scroll them down the screen
		if(enemies[i].scrollDir) {
			enemies[i].origin.y -= enemies[i].speed;
		}else{
			enemies[i].origin.y += enemies[i].speed;
		}

		//IMPORTANT - the main formation frame.
		formationFrame(&enemies[i]);

		//Have we hit the player? (pass through if dying)
		Rect enemyBound;
		if(enemies[i].type == ENEMY_BOSS) { //HACK!
			enemyBound = makeBounds(enemies[i].parallax, 72, 32);
		}else{
			enemyBound = makeSquareBounds(enemies[i].parallax, ENEMY_BOUND);
		}
		if(inBounds(playerOrigin, enemyBound) && isSolid() && !enemies[i].nonInteractive) {
			hitPlayer(enemies[i].collisionDamage);
			hitEnemy(&enemies[i], playerStrength, true);
		}

		//Spawn shots.
		if((enemies[i].combat == COMBAT_SHOOTER ||
			enemies[i].combat == COMBAT_HOMING) &&
		    enemies[i].type != ENEMY_BOSS &&	//HACK!
		    timer(&enemies[i].lastShotTime, SHOT_HZ) &&
		    enemies[i].origin.y > 0
		) {
			spawnShot(&enemies[i]);
		}else if(
			enemies[i].type == ENEMY_BOSS &&
			enemies[i].blasting &&
			timer(&enemies[i].lastShotTime, SHOT_BOSS_HZ)
		) {
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