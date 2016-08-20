#include <SDL.h>

#ifdef _WIN32
    #include "SDL_image.h"
    #include "SDL_mixer.h"
#elif __APPLE__
    #include "SDL2_Image/SDL_image.h"
    #include "SDL2_Mixer/SDL_mixer.h"
#elif __linux__
    #include "SDL2/SDL_image.h"
    #include "SDL2/SDL_mixer.h"
#endif