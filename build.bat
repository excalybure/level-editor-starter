@echo off
REM Level Editor Build Script
REM Configures and builds the project using CMake

echo ========================================
echo Level Editor Starter - Build Script
echo ========================================

REM Check if CMake is available
cmake --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: CMake is not installed or not in PATH
    exit /b 1
)

REM Configure the project if build directory doesn't exist or if CMakeLists.txt is newer
if not exist "build\vs2022-x64" (
    echo Configuring project...
    cmake --preset vs2022-x64
    if %errorlevel% neq 0 (
        echo ERROR: CMake configuration failed
        exit /b 1
    )
) else (
    echo Build directory exists, checking if reconfiguration is needed...
    REM Simple check - if CMakeLists.txt is newer than the build directory, reconfigure
    forfiles /p . /m CMakeLists.txt /c "cmd /c echo CMakeLists.txt is newer" >nul 2>&1
    if %errorlevel% equ 0 (
        echo Reconfiguring project...
        cmake --preset vs2022-x64
        if %errorlevel% neq 0 (
            echo ERROR: CMake configuration failed
            exit /b 1
        )
    )
)

REM Build the project
echo Building project...
cmake --build build\vs2022-x64 --config Debug
if %errorlevel% neq 0 (
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Executables are in: build\vs2022-x64\Debug\
echo - level_editor.exe (main application)
echo - unit_test_runner.exe (run tests)
echo.
