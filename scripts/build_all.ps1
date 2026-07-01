param(
    [string]$BuildDir = "$PSScriptRoot\..\build",
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"
$Root = Resolve-Path "$PSScriptRoot\.."

function Resolve-Cmake {
    $cmake = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmake) {
        return $cmake.Source
    }

    $pipScripts = Join-Path $env:LOCALAPPDATA "Python\pythoncore-3.14-64\Scripts\cmake.exe"
    if (Test-Path $pipScripts) {
        return $pipScripts
    }

    throw "cmake not found. Install CMake or run: python -m pip install cmake"
}

function Resolve-MingwBin {
    $local = Join-Path $Root ".toolchain\mingw\mingw64\bin"
    if (Test-Path (Join-Path $local "g++.exe")) {
        return $local
    }
    return $null
}

$Cmake = Resolve-Cmake
$MingwBin = Resolve-MingwBin

if ($MingwBin) {
    $env:PATH = "$MingwBin;$env:PATH"
    & $Cmake -S $Root -B $BuildDir -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=$Config `
        -DCMAKE_CXX_COMPILER="$MingwBin\g++.exe"
    & $Cmake --build $BuildDir --parallel
    & $Cmake --test-dir $BuildDir
} else {
    & $Cmake -S $Root -B $BuildDir -DCMAKE_BUILD_TYPE=$Config
    & $Cmake --build $BuildDir --config $Config --parallel
    & $Cmake --test-dir $BuildDir -C $Config
}
