#include "common.h"

typedef enum {
	TYPE_FRUIT,
	TYPE_COIN,
	TYPE_WEAPON,
	TYPE_HEALTH
} ItemType;

extern const int POWERUP_CHANCE;

extern void shutdownInput(void);
extern bool canSpawn(ItemType type);
extern void spawnItem(Coord coord, ItemType type);
extern void itemInit(void);
extern void itemGameFrame(void);
extern void itemRenderFrame(void);
extern void itemAnimateFrame(void);
extern void resetItems(void);
