@echo off
echo ========================================
echo OBS Studio Simple Build (VS 2022)
echo ========================================

REM Visual Studio 환경 설정
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo Setting up Visual Studio environment...
    
    REM Visual Studio 2022 확인
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
    if exist "%VS_PATH%" (
        call "%VS_PATH%" x64
        goto :build_start
    )
    
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    if exist "%VS_PATH%" (
        call "%VS_PATH%" x64
        goto :build_start
    )
    
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    if exist "%VS_PATH%" (
        call "%VS_PATH%" x64
        goto :build_start
    )
    
    echo ERROR: Visual Studio 2022 not found
    echo Please run from "Developer Command Prompt for VS 2022"
    pause
    exit /b 1
)

:build_start
REM 빌드 디렉토리 생성
if not exist "build" mkdir build
cd build

REM CMake 구성 (의존성 없이)
echo Configuring CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
    echo CMake configuration failed
    pause
    exit /b 1
)

REM 빌드 실행
echo Building OBS Studio...
cmake --build . --config Release --parallel

if %errorlevel% neq 0 (
    echo Build failed
    pause
    exit /b 1
)

echo Build completed successfully!
echo Executable should be at: %CD%\rundir\bin\64bit\obs64.exe
pause 