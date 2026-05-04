# Build script for Smart Oil Press Management
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $projectRoot "build\Desktop_Qt_6_7_3_MinGW_64_bit-Debug"

# Create build directory if it doesn't exist
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir -Force | Out-Null
}

# Configure with CMake
Write-Host "Configuring CMake..." -ForegroundColor Green
cd $buildDir
cmake -S "$projectRoot" -B . -G "Ninja" -DCMAKE_PREFIX_PATH="C:\Qt\6.7.3\mingw_64" -DCMAKE_BUILD_TYPE=Debug

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    exit 1
}

# Build with Ninja
Write-Host "Building with Ninja..." -ForegroundColor Green
ninja

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host "Build completed successfully!" -ForegroundColor Green
$exePath = Join-Path $buildDir "smartoil.exe"
Write-Host "Executable: $exePath" -ForegroundColor Cyan

# Run the application
Write-Host "Running application..." -ForegroundColor Green
& $exePath
