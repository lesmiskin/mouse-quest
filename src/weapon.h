#ifndef WEAPON_H
#define WEAPON_H

#define MAX_WEAPONS 4

extern int weaponInc;
extern bool atMaxWeapon();
extern void upgradeWeapon(void);
extern void pewGameFrame(void);
extern void pewShadowFrame(void);
extern void pewRenderFrame(void);
extern void pewAnimateFrame(void);
extern void pew(void);
extern void pewInit(void);
extern void resetPew(void);

#endif