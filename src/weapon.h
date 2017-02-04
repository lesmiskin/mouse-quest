#ifndef WEAPON_H
#define WEAPON_H

#define MAX_WEAPONS 3

extern bool canFireInLevel;
extern int weaponInc;
extern void changeWeapon(int newWeapon);
extern bool atMaxWeapon();
extern void upgradeWeapon();
extern void pewGameFrame();
extern void pewShadowFrame();
extern void pewRenderFrame();
extern void pewAnimateFrame();
extern void pew();
extern void pewInit();
extern void resetPew();

#endif