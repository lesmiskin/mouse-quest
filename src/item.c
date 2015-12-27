#include <time.h>
#include "assets.h"
#include "common.h"
#include "item.h"
#include "weapon.h"
#include "renderer.h"
#include "player.h"
#include "hud.h"
#include "input.h"

//TODO: 'Battery pack' powerup.
//TODO: Better shooting animation (upward-facing).
//TODO: When moving left and right, use front-facing expression.

//TODO: Proper error message when asset version can't be found.
//TODO: Add coin HUD display.
//TODO: Add Mario-style starry animation on coin pickup.
//TODO: Add battery 'pop' animation when hurt.
//TODO: Add coin 'travel' animation on pickup.
//TODO: Add silver coins.
//TODO: Add point plume.

typedef enum {
	SPAWN_ALWAYS,
	SPAWN_ONE_ONSCREEN
} ItemSpawnPolicy;

typedef enum {
	ANIM_SEQUENCE,
	ANIM_BOOLEAN
} AnimationStyle;

typedef struct {
	ItemType type;
	Coord origin;
	Coord parallax;
	double swayInc;
	bool swing;
	int animFrame;
	int maxAnims;
	AnimationStyle animStyle;
	char* baseFrameName;
	char* realFrameName;
	bool traveling;
} Item;

#define MAX_ITEMS 100

static int itemCount = 0;
const int POWERUP_CHANCE = 100;
static Item items[MAX_ITEMS];
static const double ITEM_SPEED = 1.0;
const int POWERUP_BOUND = 24;
static const int slowAnimTime = 500 / 1000;
static bool boolAnimFrame = false;
static long lastBoolAnimTime;

static bool invalidPowerup(Item *powerup) {
	//Unset struct.
	if(powerup->maxAnims == 0) return true;

	//Past the screen bounds, plus it's own radius.
	return powerup->origin.y > screenBounds.y + POWERUP_BOUND/2;
}

static bool shouldAnimate(Item item) {
	return item.maxAnims > 1;
}

static void updateAnimationFrame(Item* item) {
	//Global flicker animation
	if(item->animStyle == ANIM_BOOLEAN) {
		item->animFrame = boolAnimFrame + 1;
	//Loop frames.
	}else if(item->animFrame == item->maxAnims) {
		item->animFrame = 1;
	//Increment normal frame.
	}else{
		item->animFrame++;
	}

	//Build the full filename of the animation, based on base, number of frame, and suffix.
	sprintf(
		item->realFrameName,
		"%s%02d.png",
		item->baseFrameName,
		item->animFrame
	);
}

static Coord itemParallax(Coord origin) {
	return parallax(origin, PARALLAX_PAN, PARALLAX_LAYER_FOREGROUND, PARALLAX_X, PARALLAX_ADDITIVE);
}

static ItemSpawnPolicy getSpawnPolicyForType(ItemType type) {
	switch(type) {
		case TYPE_COIN:
			return SPAWN_ALWAYS;
		case TYPE_HEALTH:
			return SPAWN_ONE_ONSCREEN;
		case TYPE_WEAPON:
			return SPAWN_ONE_ONSCREEN;
	}
}

bool canSpawn(ItemType type) {

	switch(type) {
		case TYPE_HEALTH:
			if(atFullhealth()) return false;
			break;
		case TYPE_WEAPON:
			if(atMaxWeapon()) return false;
			break;
	}

	ItemSpawnPolicy policy = getSpawnPolicyForType(type);

	switch(policy) {
		case SPAWN_ALWAYS:
			return true;
		case SPAWN_ONE_ONSCREEN:
			for(int i=0; i < MAX_ITEMS; i++) {
				if(invalidPowerup(&items[i])) continue;
				if(items[i].type == type) {
					return false;
				}
			}
			return true;
	}
}

void spawnItem(Coord coord, ItemType type) {
	//Stop spawning items after we've elapsed our total.
	if(itemCount == MAX_ITEMS) itemCount = 0;

	bool swing;
	int maxAnims = 1;
	AnimationStyle animRate;

	//Reserve space on struct for both string arrays (must be pointers, we can't reassign to arrays)
	char* realFrameName = malloc(sizeof(char) * 50);
	char* baseFrameName = malloc(sizeof(char) * 50);

	switch(type) {
		case TYPE_COIN:
			swing = false;
			maxAnims = 12;
			animRate = ANIM_SEQUENCE;
			strcpy(baseFrameName, "coin-");
			break;
		case TYPE_FRUIT: {
			int chance = randomMq(0, 100);

			if(chance < 33) {
				strcpy(baseFrameName, "cherries-");
				strcpy(realFrameName, "cherries-");
			}else if(chance > 66) {
				strcpy(baseFrameName, "grape-");
				strcpy(realFrameName, "grape-");
			}else{
				strcpy(baseFrameName, "pineapple-");
				strcpy(realFrameName, "pineapple-");
			}

			swing = false;
			animRate = ANIM_BOOLEAN;
			maxAnims = 2;
			break;
		}
		case TYPE_WEAPON:
			swing = true;
			animRate = ANIM_BOOLEAN;

			char* filename = NULL;
			switch(weaponInc) {
				case 0:
					filename = "powerup-double-";
					maxAnims = 2;
					break;
				case 1:
					filename = "powerup-triple-";
					maxAnims = 2;
					break;
				case 2:
					filename = "powerup-fan.png";
					break;
				default:
					filename = "powerup.png";
					break;
			}

			strcpy(baseFrameName, filename);
			strcpy(realFrameName, filename);
			break;
		case TYPE_HEALTH:
			swing = true;
			animRate = ANIM_BOOLEAN;
			maxAnims = 2;
			strcpy(baseFrameName, "battery-pack-");
			strcpy(realFrameName, "battery-pack-");
			break;
	}

	//Build, and init first animation frame.
	Item powerup = {
		type,
		coord,
		itemParallax(coord),
		0,
		swing,
		1,
		maxAnims,
		animRate,
		baseFrameName,
		realFrameName,
		false
	};
	if(shouldAnimate(powerup)) updateAnimationFrame(&powerup);

	items[itemCount++] = powerup;
}

void itemShadowFrame(void) {
	//Render powerup shadows
	for(int i=0; i < MAX_ITEMS; i++) {
		//NB: We deliberately skip shadows for traveling ones.
		if(invalidPowerup(&items[i]) || items[i].traveling) continue;

		SDL_Texture *texture = getTextureVersion(items[i].realFrameName, ASSET_SHADOW);
		Sprite sprite = makeSprite(texture, zeroCoord(), SDL_FLIP_NONE);
		Coord shadowCoord = parallax(items[i].parallax, PARALLAX_SUN, PARALLAX_LAYER_SHADOW, PARALLAX_X, PARALLAX_SUBTRACTIVE);
		shadowCoord.y += STATIC_SHADOW_OFFSET;
		drawSpriteAbs(sprite, shadowCoord);
	}
}

void itemRenderFrame(void) {
	//Render items
	for(int i=0; i < MAX_ITEMS; i++) {
		if(invalidPowerup(&items[i])) continue;

		SDL_Texture *texture = getTexture(items[i].realFrameName);
		Sprite sprite = makeSprite(texture, zeroCoord(), SDL_FLIP_NONE);
		if(items[i].traveling) {
			drawSpriteAbs(sprite, items[i].origin);
		}else{
			drawSpriteAbs(sprite, items[i].parallax);
		}
	}
}

void itemAnimateFrame(void) {
	//Powerups
	for(int i=0; i < MAX_ITEMS; i++) {
		if(invalidPowerup(&items[i])) continue;
		if(!shouldAnimate(items[i])) continue;

		updateAnimationFrame(&items[i]);
	}

	if(due(lastBoolAnimTime, 500)) {
		lastBoolAnimTime = clock();
		boolAnimFrame = !boolAnimFrame;
	}
}

void itemGameFrame(void) {
	//Powerups
	for(int i=0; i < MAX_ITEMS; i++) {
		if(invalidPowerup(&items[i])) continue;

		if(items[i].traveling) {
			//Stop when we reach the HUD icon
			if(items[i].origin.x >= 216) {
				//Register the action associated with the item.
				if(items[i].type == TYPE_COIN) {
					coins++;
				}

				Item nullItem = {};
				items[i] = nullItem;
				continue;
			}

			//Travel towards the HUD icon

			//TODO: Store this on spawn.
			Coord travelStep = getStep(makeCoord(216, 27), items[i].origin, 5, false);

			items[i].origin.x -= travelStep.x;
			items[i].origin.y -= travelStep.y;
			items[i].parallax = itemParallax(items[i].origin);
			continue;
		}

		//Scroll down screen
		items[i].origin.y += ITEM_SPEED;
		items[i].parallax = itemParallax(items[i].origin);

		//Sway in sine wave pattern
		if(items[i].swing){
			items[i].parallax.x = sineInc(items[i].origin.x, &items[i].swayInc, 0.05, 32);
		}

		//Check if player touching
		Rect powerupBound = makeSquareBounds(items[i].parallax, POWERUP_BOUND);
		if(inBounds(playerOrigin, powerupBound)) {
			switch(items[i].type) {
				case TYPE_COIN:
					play("Pickup_Coin34.wav");
					raiseScore(25, true);
					items[i].traveling = true;				//zoom off the screen.
					items[i].origin = items[i].parallax;	//start from visual origin.
					break;
				case TYPE_FRUIT:
					play("Pickup_Coin34.wav");
					raiseScore(100, true);
					break;
				case TYPE_WEAPON:
					play("Powerup8.wav");
					upgradeWeapon();
					spawnPlume(PLUME_LASER);
					break;
				case TYPE_HEALTH:
					play("Powerup8.wav");
					restoreHealth();
					spawnPlume(PLUME_POWER);
					break;
			}

//			SDL_HapticRumblePlay(haptic, 0.25, 100);

			//Null any non-traveling items immediately.
			if(!items[i].traveling) {
				Item nullPowerup = { };
				items[i] = nullPowerup;
			}
		}
	}
}

void resetItems(void) {
	memset(items, 0, sizeof(items));
	itemCount = 0;
}

void itemInit(void) {
	lastBoolAnimTime = clock();
	resetItems();
	itemAnimateFrame();
}
