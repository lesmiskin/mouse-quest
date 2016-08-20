Building on Windows with MiniGW
-------------------------------
1. Download the *development libraries* for *MiniGW* for each SDL package:
   https://www.libsdl.org/download-2.0.php
   https://www.libsdl.org/projects/SDL_image/
   https://www.libsdl.org/projects/SDL_mixer/
2. Extract each archive into the *same folder* (e.g. C:\mingw_dev_lib)
3. Create the environment variable "SDL2DIR", pointing to the location of the
   32-bit SDL.h file, e.g. C:\mingw_dev_lib\i686-w64-mingw32\include\SDL2
4. Completely restart CLion.
5. Rebuild the CMake configuration from the File menu.

Further reading: https://github.com/mmatyas/supermariowar/wiki/Building-on-Windows

Error: "mouse-quest is not a valid Win32 application"
-----------------------------------------------------
Make sure CLion is configured to run mouse-quest.exe, not the Linux mouse-quest
binary.

Error: "SDL2.DLL not found"
---------------------------
Copy the contents of C:\mingw_dev_lib\i686-w64-mingw32\bin to C:\Windows

Other
-----
* You should have a mysdl.h source file to centralise platform-specific paths
  to SDL resources. If you don't have this, get it from the Coffee Quest repo.
