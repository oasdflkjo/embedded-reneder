 

# Saturn Renderer Setup Script for Windows with CMake/Ninja/Clang
Write-Host "Setting up Saturn Renderer with CMake/Ninja/Clang..." -ForegroundColor Green

# Check if vcpkg is available
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
    Write-Host "vcpkg not found. Installing vcpkg..." -ForegroundColor Yellow
    
    # Install vcpkg
    $vcpkgPath = "C:\vcpkg"
    if (-not (Test-Path $vcpkgPath)) {
        Write-Host "Cloning vcpkg..." -ForegroundColor Cyan
        git clone https://github.com/Microsoft/vcpkg.git $vcpkgPath
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Failed to clone vcpkg. Please install git or download vcpkg manually." -ForegroundColor Red
            return
        }
    }
    
    # Bootstrap vcpkg
    Write-Host "Bootstrapping vcpkg..." -ForegroundColor Cyan
    & "$vcpkgPath\bootstrap-vcpkg.bat"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to bootstrap vcpkg." -ForegroundColor Red
        return
    }
} else {
    Write-Host "Found vcpkg at: $vcpkgPath" -ForegroundColor Green
}

# Install SDL2
Write-Host "Installing SDL2 via vcpkg..." -ForegroundColor Cyan
& "$vcpkgPath\vcpkg.exe" install sdl2:x64-windows
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to install SDL2." -ForegroundColor Red
    return
}

# Check for Visual Studio tools
Write-Host "Checking for Visual Studio Build Tools..." -ForegroundColor Cyan

$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($vsPath) {
        Write-Host "Found Visual Studio at: $vsPath" -ForegroundColor Green
    } else {
        Write-Host "Visual Studio with C++ tools not found. Please install Visual Studio with C++ support." -ForegroundColor Red
        return
    }
} else {
    Write-Host "Visual Studio Installer not found. Please install Visual Studio." -ForegroundColor Red
    return
}

Write-Host ""
Write-Host "Setup complete!" -ForegroundColor Green
Write-Host "To build the project:" -ForegroundColor Cyan
Write-Host "  1. Open a Visual Studio Developer Command Prompt, or run:" -ForegroundColor White
Write-Host "     & '$vsPath\Common7\Tools\Launch-VsDevShell.ps1'" -ForegroundColor Gray
Write-Host "  2. Then run:" -ForegroundColor White
Write-Host "     cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=$vcpkgPath/scripts/buildsystems/vcpkg.cmake" -ForegroundColor Gray
Write-Host "     cmake --build build" -ForegroundColor Gray
Write-Host "     .\build\saturn_renderer.exe" -ForegroundColor Gray 