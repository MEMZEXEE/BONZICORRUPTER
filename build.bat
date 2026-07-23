 :: Create build folder if it doesn't exist
if not exist "build" mkdir "build"

:: Assemble bootloader (pointing to src)
nasm -f bin src/boot.asm -o build/boot.bin || exit /b

:: Merge files (pointing to build/boot.bin and images/bonzi.raw)
copy /b build\boot.bin + images\bonzi.raw build\payload.bin || exit /b

:: Compile resources (pointing to src/resources.rc)
windres src/resources.rc -o build/resources.o || exit /b

:: Build EXE (pointing to src/main.cpp)
g++ src/main.cpp build/resources.o -o build/BONZICORRUPTER.exe ^
-static -mwindows -static-libgcc -static-libstdc++ ^
-lpthread -lwinmm -lmsimg32 || exit /b

:: Uncomment the 2 lines if you are going to put the EXE in an ISO
:: Create RAR archive
:: "C:\Program Files\WinRAR\WinRAR.exe" a -afrar build\BONZICORRUPTER.rar build\BONZICORRUPTER.exe || exit /b

:: Convert RAR to ISO
:: "C:\Program Files (x86)\AnyToISO\anytoiso.exe" /convert build\BONZICORRUPTER.rar build\BONZICORRUPTER.iso 