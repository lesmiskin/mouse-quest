#include <time.h>
#include "formations.h"
#include "enemy.h"
#include "myc.h"

static void incScript(Enemy *e, int toMove) {
	if(e->scriptInc == toMove) e->scriptInc++;
}

static bool onInc(Enemy *e, int thisInc) {
	return e->scriptInc == thisInc;
}

static bool scriptDue(Enemy* e, int time) {
	return due(e->spawnTime, time);
}

static void applyFormation(Enemy *e) {
	e->formationOrigin = e->origin;
}

static void left(Enemy* e) {
	e->origin.x -= e->speedX;
}

static void right(Enemy* e) {
	e->origin.x += e->speedX;
}

void formationFrame(Enemy* e) {
	switch(e->movement) {

		// SPIN -------------------------------------------------
		case P_SWIRL_LEFT:
			e->origin.x = sineInc(e->origin.x, &e->swayIncX, -e->speedX, 2);
			applyFormation(e);
			break;
		case P_SWIRL_RIGHT:
			e->origin.x = sineInc(e->origin.x, &e->swayIncX, e->speedX, 2);
			applyFormation(e);
			break;

		// CURVE -------------------------------------------------
		case P_PEEL_RIGHT:
			if(scriptDue(e, 650)) {
				e->origin.x = sineInc(e->origin.x, &e->swayIncX, -e->speedX, 7);
			}
			applyFormation(e);
			break;
		case P_PEEL_LEFT:
			if(scriptDue(e, 650)) {
				e->origin.x = sineInc(e->origin.x, &e->swayIncX, e->speedX, 7);
			}
			applyFormation(e);
			break;

		// CURVE -------------------------------------------------
		case P_CURVE_RIGHT:
			if(scriptDue(e, 550) && e->origin.x < 190 && onInc(e, 0)) {
				right(e);
			} else if(scriptDue(e, 1750) && e->origin.x > 150) {
				left(e); incScript(e, 0);
			}
			applyFormation(e);
			break;
		case P_CURVE_LEFT:
			if(scriptDue(e, 550) && e->origin.x > 80 && onInc(e, 0)) {
				left(e);
			} else if(scriptDue(e, 1750) && e->origin.x < 120) {
				right(e); incScript(e, 0);
			}
			applyFormation(e);
			break;

		// SNAKE -------------------------------------------------
		case P_SNAKE_RIGHT:
			e->origin.x = sineInc(e->origin.x, &e->swayIncX, e->speedX, 1.2);
			applyFormation(e);
			break;
		case P_SNAKE_LEFT:
			e->origin.x = sineInc(e->origin.x, &e->swayIncX, -e->speedX, 1.2);
			applyFormation(e);
			break;

		// CROSSOVER -------------------------------------------------
		case P_CROSS_RIGHT:
			e->origin.x += sineInc(e->origin.x, &e->swayIncX, e->speedX, 2.9);
			applyFormation(e);
			break;
		case P_CROSS_LEFT:
			e->origin.x = sineInc(e->origin.x, &e->swayIncX, -e->speedX, 2.9);
			applyFormation(e);
			break;

		// STRAFER -------------------------------------------------
		case P_STRAFE_RIGHT:
			if(scriptDue(e, 500))
				e->origin.x = sineInc(e->origin.x, &e->swayIncX, -e->speedX, 5);

			applyFormation(e);
			break;
		case P_STRAFE_LEFT:
			if(scriptDue(e, 500))
				e->origin.x = sineInc(e->origin.x, &e->swayIncX, e->speedX, 5);
			applyFormation(e);
			break;

		case PATTERN_BOSS_INTRO:
			if(e->scriptInc == 0) {
				e->scrollDir = true;
				e->scriptInc++;
			}
			e->formationOrigin.y = e->origin.y;
			break;

		case PATTERN_BOSS:
			// Scroll down to the middle of the screen.
			if(scriptDue(e, 4000) && e->scriptInc == 0) {
				e->scrollDir = !e->scrollDir;
				e->spawnTime = clock(); //HACK!
				e->scriptInc++;
			// Now scroll up and down.
			} else if(scriptDue(e, 2250) && e->scriptInc == 1) {
				e->scrollDir = !e->scrollDir;
				e->spawnTime = clock(); //HACK!
			}
			e->formationOrigin.y = e->origin.y;

			// Blasts.
			if(due(e->lastBlastTime, 1000)) {
				e->blasting = !e->blasting;
				e->lastBlastTime = clock();
			}

			// Sway him from side to side.
			e->formationOrigin.x = sineInc(e->origin.x, &e->swayIncX, 0.04, 30);

			break;

		case PATTERN_CIRCLE:
			//TODO: Find out why swayIncX and swayIncY need to be swapped :p
			e->formationOrigin.y = sineInc(e->origin.y, &e->swayIncX, 0.04, 21);
			e->formationOrigin.x = cosInc(e->origin.x, &e->swayIncY, 0.04, 21);
			break;
		case PATTERN_SNAKE:
			e->formationOrigin.x = sineInc(e->origin.x, &e->swayIncX, 0.075, 12);
			e->formationOrigin.y = e->origin.y;
			break;
		case PATTERN_BOB:
			e->formationOrigin.x = e->origin.x;
			e->formationOrigin.y = sineInc(e->origin.y, &e->swayIncY, 0.075, 12);;
			break;
		default:
			e->formationOrigin = e->origin;
			break;
	}
}