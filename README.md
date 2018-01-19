# BGE-macos

# Mouse Sensitivity
This version of BGE has a mouse sensitivity property added to the config file and has been calibrated for use on osx.

# Font Rendering
The Game class has had work done on it towards getting text to render to the UI

# Texture rendering
The Ground Class has had work done on it towards getting textures to load and be rendered correctly.

# Setup
To build the project SDL2, SDL_TTF and OpenGL frameworks have to be installed and linked in the papropriate directories. Bullet and GLM have to be built from source. The following command can be used with cmake to build bullet as macos compatible frameworks: cmake . -G "Unix Makefiles" -DINSTALL_LIBS=ON -DBUILD_SHARED_LIBS=ON     -DFRAMEWORK=ON  -DCMAKE_OSX_ARCHITECTURES='i386;x86_64'     -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/Library/Frameworks     -DCMAKE_INSTALL_NAME_DIR=/Library/Frameworks -DBUILD_DEMOS:BOOL=OFF
