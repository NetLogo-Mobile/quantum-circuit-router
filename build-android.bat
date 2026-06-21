@echo off
setlocal enabledelayedexpansion

REM ---------------------------------------------------------------------------
REM Build libqcrouter.so for Android with 16 KB page-size support.
REM Uses Unity's bundled NDK (r23) + Visual Studio 2022's bundled CMake/Ninja.
REM ---------------------------------------------------------------------------

set "NDK=C:\Program Files\Unity\Hub\Editor\2022.3.62f3\Editor\Data\PlaybackEngines\AndroidPlayer\NDK"
set "CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
set "READELF=%NDK%\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-readelf.exe"

set "ROOT=%~dp0"
set "ABIS=arm64-v8a armeabi-v7a x86_64 x86"

if not exist "%NDK%"   ( echo [ERROR] NDK not found: %NDK%   & exit /b 1 )
if not exist "%CMAKE%" ( echo [ERROR] cmake not found: %CMAKE% & exit /b 1 )
if not exist "%NINJA%" ( echo [ERROR] ninja not found: %NINJA% & exit /b 1 )

for %%A in (%ABIS%) do (
    echo.
    echo ========================================================
    echo  Building ABI: %%A
    echo ========================================================
    "%CMAKE%" -G Ninja -S "%ROOT%." -B "%ROOT%Builds\cmake\%%A" ^
        -DCMAKE_TOOLCHAIN_FILE="%NDK%\build\cmake\android.toolchain.cmake" ^
        -DCMAKE_MAKE_PROGRAM="%NINJA%" ^
        -DANDROID_ABI=%%A ^
        -DANDROID_PLATFORM=android-21 ^
        -DCMAKE_BUILD_TYPE=Release ^
        -DCMAKE_LIBRARY_OUTPUT_DIRECTORY="%ROOT%Builds\Android\%%A"
    if errorlevel 1 ( echo [ERROR] configure failed for %%A & exit /b 1 )

    "%CMAKE%" --build "%ROOT%Builds\cmake\%%A"
    if errorlevel 1 ( echo [ERROR] build failed for %%A & exit /b 1 )
)

echo.
echo ========================================================
echo  Verifying 16 KB alignment (LOAD segments must be 2**14)
echo ========================================================
for %%A in (%ABIS%) do (
    echo.
    echo --- %%A : Builds\Android\%%A\libqcrouter.so ---
    "%READELF%" -l "%ROOT%Builds\Android\%%A\libqcrouter.so" | findstr /C:"LOAD" /C:"Alignment"
)

echo.
echo Done. LOAD segment alignment should read 0x4000 (16384) for 16 KB support.
endlocal
