project(mouse-quest)
cmake_minimum_required(VERSION 2.8)

# Set make parameters, and supply source files.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

# Output binary executable to same dir as sources, so
# assets can be referenced OK.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_executable(mouse-quest common.c renderer.c assets.c player.c input.c main.c background.c weapon.c enemy.c scripting.c scripts.c hud.c item.c)

# SDL includes (Source: https://github.com/tcbrindle/sdl2-cmake-scripts)
find_package(SDL2 REQUIRED)
find_package(SDL2_Image REQUIRED)
find_package(SDL2_Mixer REQUIRED)
include_directories(${SDL2_INCLUDE_DIR} ${SDL2_IMAGE_INCLUDE_DIR} ${SDL2_MIXER_INCLUDE_DIR})

target_link_libraries(mouse-quest -lm ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARIES} ${SDL2MIXER_LIBRARY})

# Leverage CMake to discover the SDL module.
#INCLUDE(FindPkgConfig)
#PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
#PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image)
#PKG_SEARCH_MODULE(SDL2MIXER REQUIRED SDL2_mixer)
#INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
#INCLUDE_DIRECTORIES(${SDL2IMAGE_INCLUDE_DIRS})
#INCLUDE_DIRECTORIES(${SDL2MIXER_INCLUDE_DIRS})
## Note: '-lm' to include glibm for math.h (glibc in included by default for stdio.h etc).
#TARGET_LINK_LIBRARIES(mouse-quest -lm ${SDL2_LIBRARIES} ${SDL2MIXER_LIBRARIES} ${SDL2IMAGE_LIBRARIES})



#
#
#project(surface)
#cmake_minimum_required(VERSION 2.8)
#
## Set make parameters, and supply source files.
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
#
## Output binary executable to same dir as sources, so
## assets can be referenced OK.
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
#
#add_executable(mouse-quest common.c renderer.c assets.c player.c input.c main.c background.c weapon.c enemy.c scripting.c scripts.c hud.c item.c)
#
## Leverage CMake to discover the SDL module.
#INCLUDE(FindPkgConfig)
#
## Dependencies.
#PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
#PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image)
#PKG_SEARCH_MODULE(SDL2MIXER REQUIRED SDL2_mixer)
#INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
#INCLUDE_DIRECTORIES(${SDL2IMAGE_INCLUDE_DIRS})
#INCLUDE_DIRECTORIES(${SDL2MIXER_INCLUDE_DIRS})
#
## Note: '-lm' to include glibm for math.h (glibc in included by default for stdio.h etc).
#TARGET_LINK_LIBRARIES(mouse-quest -lm ${SDL2_LIBRARIES} ${SDL2MIXER_LIBRARIES} ${SDL2IMAGE_LIBRARIES})
