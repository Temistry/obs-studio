@echo off
setlocal enabledelayedexpansion

echo ========================================
echo OBS Studio Build Script (VS 2022)
echo ========================================

REM ====== 설정 부분 ======
REM 현재 디렉토리를 소스 디렉토리로 사용
set SOURCE_DIR=%CD%
set BUILD_DIR=%SOURCE_DIR%\build

REM 의존성 경로 (필요에 따라 수정)
set DEPS_PATH=D:\obs-deps\win64
set QTDIR=D:\Qt\5.15.2\msvc2022_64
set CEF_ROOT_DIR=D:\cef_binary\75.1.16

REM Visual Studio 2022 설정
set VS_GENERATOR="Visual Studio 17 2022"
set VS_ARCHITECTURE=x64
set BUILD_CONFIG=Release
REM ========================

echo Source Directory: %SOURCE_DIR%
echo Build Directory: %BUILD_DIR%

REM CMake 확인
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: CMake not found
    echo Please install CMake from: https://cmake.org/download/
    pause
    exit /b 1
)

REM Visual Studio 환경 확인
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo WARNING: Visual Studio compiler not found
    echo Trying to set up Visual Studio environment...
    
    REM Visual Studio 2022 경로 확인
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
    if exist "%VS_PATH%" (
        echo Found Visual Studio 2022 Community
        call "%VS_PATH%" x64
        goto :check_vs
    )
    
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    if exist "%VS_PATH%" (
        echo Found Visual Studio 2022 Professional
        call "%VS_PATH%" x64
        goto :check_vs
    )
    
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
    if exist "%VS_PATH%" (
        echo Found Visual Studio 2022 Enterprise
        call "%VS_PATH%" x64
        goto :check_vs
    )
    
    echo ERROR: Visual Studio 2022 not found
    echo Please run from "Developer Command Prompt for VS 2022"
    pause
    exit /b 1
)

:check_vs
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Visual Studio environment setup failed
    pause
    exit /b 1
)

echo Visual Studio environment ready.

REM 1. 빌드 폴더 생성
echo Creating build directory...
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

REM 2. CMake 구성
echo Configuring with CMake...
pushd "%BUILD_DIR%"

REM 의존성 경로가 존재하는지 확인하고 CMake 명령 구성
set CMAKE_ARGS=cmake .. -G %VS_GENERATOR% -A %VS_ARCHITECTURE%

REM 의존성 경로가 존재하면 추가
if exist "%DEPS_PATH%" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DDepsPath="%DEPS_PATH%"
    echo Using DepsPath: %DEPS_PATH%
) else (
    echo WARNING: DepsPath not found: %DEPS_PATH%
)

if exist "%QTDIR%" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DQTDIR="%QTDIR%"
    echo Using QTDIR: %QTDIR%
) else (
    echo WARNING: QTDIR not found: %QTDIR%
)

if exist "%CEF_ROOT_DIR%" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DCEF_ROOT_DIR="%CEF_ROOT_DIR%"
    echo Using CEF_ROOT_DIR: %CEF_ROOT_DIR%
) else (
    echo WARNING: CEF_ROOT_DIR not found: %CEF_ROOT_DIR%
)

echo Running: %CMAKE_ARGS%
%CMAKE_ARGS%

if errorlevel 1 (
    echo ERROR: CMake configuration failed.
    echo Check the error messages above.
    pause
    exit /b 1
)

echo CMake configuration successful.

REM 3. 빌드 실행
echo Building OBS Studio...
cmake --build . --config %BUILD_CONFIG% --parallel

if errorlevel 1 (
    echo ERROR: Build failed.
    echo Check the error messages above.
    pause
    exit /b 1
)

popd

echo ========================================
echo BUILD COMPLETED SUCCESSFULLY!
echo ========================================
echo.
echo The built executable should be at:
echo %BUILD_DIR%\rundir\bin\64bit\obs64.exe
echo.
pause
