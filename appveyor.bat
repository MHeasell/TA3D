rem Required for bash to set itself up
SET MSYSTEM=MINGW64

rem Refresh packages list
C:\msys64\usr\bin\bash -lc "pacman -Syq" || goto :error

rem Install build tools
C:\msys64\usr\bin\bash -lc "pacman -Sq --needed --noconfirm make mingw-w64-x86_64-cmake mingw-w64-x86_64-toolchain" || goto :error

rem Install project dependencies
C:\msys64\usr\bin\bash -lc "pacman -Sq --needed --noconfirm mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-SDL2_mixer mingw-w64-x86_64-SDL2_net mingw-w64-x86_64-glew mingw-w64-x86_64-freetype mingw-w64-x86_64-dlfcn" || goto :error

rem Build the project
C:\msys64\usr\bin\bash -lc "cd $APPVEYOR_BUILD_FOLDER && mkdir build && cd build && cmake -G 'Unix Makefiles' ../src && make -j 2 && ./ta3d/src/ta3d_test" || goto :error

goto :EOF

:error
exit /b %errorlevel%
