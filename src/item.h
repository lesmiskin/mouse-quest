#ifndef ITEM_H
#define ITEM_H

#include "common.h"

typedef enum {
	TYPE_FRUIT,
	TYPE_COIN,
	TYPE_WEAPON,
	TYPE_HEALTH
} ItemType;

extern void shutdownInput();
extern bool canSpawn(ItemType type);
extern void spawnItem(Coord coord, ItemType type);
extern void itemInit();
extern void itemGameFrame();
extern void itemShadowFrame();
extern void itemRenderFrame();
extern void itemAnimateFrame();
extern void resetItems();

#endif
