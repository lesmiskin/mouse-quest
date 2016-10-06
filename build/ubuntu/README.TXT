Ubuntu version 0.13

==========================================
==           INTRODUCTION               ==
==========================================

Mouse Quest is a top-down shoot-em-up ("shmup" for short), in the style of
video games from the 1980's and 1990's.

You control Mike, a sentient computer mouse who's out to save the world
from a virus-infected fleet of computer hardware. Fight the humble floppy
disk and CD-ROM, all the way to peripherals of immense size and ferocity.

This version of Mouse Quest is a small preview only, and includes just one 
level and limited game features. The quality of the full game may change 
dramatically upon release, so please keep this in mind while playing.

If you have any comments or feedback on Mouse Quest, please contact Les
Miskin on <trilinear.nz@gmail.com>.


==========================================
==            INSTALLATION              ==
==========================================

1. Extract the Mouse Quest compressed file to a folder of your choosing.
2. From a terminal instance, run the command "sudo ./install.sh".
3. Type your password and press ENTER.
4. After the requisite SDL files have been installed to your computer, 
   press any key to start Mouse Quest.

Once Mouse Quest has been installed, you can run the 'mouse-quest' binary
file directly from your computer at any time, or make a link to it and 
store it on your desktop for easy access. You know you want to!


==========================================
==            HOW TO PLAY               ==
==========================================

Start the game by pressing SPACE.

To control Mike, use the arrow keys on your keyboard, and press SPACE to
shoot your laser. You can hold down SPACE to give your enemies a taste of 
your rapid fire mode, too!

To exit the game, press ESCAPE from the title screen. Or press ESCAPE twice 
if you're in the middle of a level.


==========================================
==                LICENSE               ==
==========================================

Visual and audio assets
-----------------------
The PNG, OGG, and WAV files supplied with the game are licensed under 
CC BY-NC-SA 4.0. See 'licenses/LICENSE-MEDIA.txt' or 
<https://creativecommons.org/licenses/by-nc-sa/4.0/>.

Other game files and source code
--------------------------------
All other files supplied with the game which are not listed above, 
including source if you received it, are licensed under CC BY-NC-ND 4.0. 
See 'licenses/LICENSE-DEFAULT.txt' or 
<https://creativecommons.org/licenses/by-nc-nd/4.0/>.

Third-party software
--------------------
SDL 2.0 runtime environment: See 'licenses/SDL2'.
SDL Mixer 2.0: See 'licenses/SDL2-Mixer'.
SDL Image 2.0: See 'licenses/SDL2-Image'.


==========================================
==                CREDITS               ==
==========================================

Programming and design
----------------------
Les Miskin

Special thanks
--------------
Rachael Ashby
Bob Brown
Steff Ashby

Software
--------
Bfxr
CLion
GarageBand
GIMP
GitLab
SDL 2.0
SDL Image 2.0
SDL Mixer 2.0


==========================================
==             CHANGELOG                ==
==========================================

0.1
Initial public release

0.11
Testing release for attempted bugfix: Shooting stops after a few seconds after game start.

0.12
Fixed bug: Shooting stops after a few seconds after game start.

0.13
Added boss introduction animation sequence.
Added unique music for boss encounter.
Fixed bug: Warning sound would sometimes not play during boss intro.

