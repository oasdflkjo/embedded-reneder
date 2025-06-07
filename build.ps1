# Build script for Saturn Renderer
Write-Host "Building Saturn Renderer..." -ForegroundColor Green

# Find Visual Studio
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vsWhere)) {
    Write-Host "Visual Studio not found. Please run setup_sdl2.ps1 first." -ForegroundColor Red
    exit 1
}

$vsPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    Write-Host "Visual Studio with C++ tools not found." -ForegroundColor Red
    exit 1
}

# Find vcpkg
$vcpkgPath = ""
$possiblePaths = @(
    "$env:VCPKG_ROOT",
    "C:\vcpkg",
    "C:\tools\vcpkg",
    "C:\dev\vcpkg"
)

foreach ($path in $possiblePaths) {
    if ($path -and (Test-Path "$path\vcpkg.exe")) {
        $vcpkgPath = $path
        break
    }
}

if (-not $vcpkgPath) {
    Write-Host "vcpkg not found. Please run setup_sdl2.ps1 first." -ForegroundColor Red
    exit 1
}

Write-Host "Using vcpkg at: $vcpkgPath" -ForegroundColor Cyan
Write-Host "Using Visual Studio at: $vsPath" -ForegroundColor Cyan

# Store current directory
$currentDir = Get-Location

# Setup Visual Studio environment and run commands in the same session
Write-Host "Configuring with CMake..." -ForegroundColor Yellow
Write-Host "Using MSVC compiler" -ForegroundColor Cyan
& cmd /c "`"$vsPath\Common7\Tools\VsDevCmd.bat`" -arch=amd64 && cd /d `"$currentDir`" && cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=`"$vcpkgPath/scripts/buildsystems/vcpkg.cmake`""

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed." -ForegroundColor Red
    exit 1
}

# Build
Write-Host "Building..." -ForegroundColor Yellow
& cmd /c "`"$vsPath\Common7\Tools\VsDevCmd.bat`" -arch=amd64 && cd /d `"$currentDir`" && cmake --build build"

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Build successful!" -ForegroundColor Green
Write-Host "Run the program with:" -ForegroundColor Cyan
Write-Host "  .\build\saturn_renderer.exe" -ForegroundColor White 