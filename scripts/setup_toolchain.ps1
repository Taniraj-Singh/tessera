param(
    [switch]$SkipDownload
)

$ErrorActionPreference = "Stop"
$Root = Resolve-Path "$PSScriptRoot\.."
$Toolchain = Join-Path $Root ".toolchain"
$Archive = Join-Path $Toolchain "mingw.7z"
$ExtractTo = Join-Path $Toolchain "mingw"
$MingwUrl = "https://github.com/niXman/mingw-builds-binaries/releases/download/14.2.0-rt_v12-rev0/x86_64-14.2.0-release-posix-seh-ucrt-rt_v12-rev0.7z"

Write-Output "Installing CMake and Ninja via pip..."
python -m pip install --upgrade pip cmake ninja py7zr

if (-not $SkipDownload) {
    if (-not (Test-Path "$ExtractTo\mingw64\bin\g++.exe")) {
        New-Item -ItemType Directory -Force -Path $Toolchain | Out-Null
        Write-Output "Downloading portable MinGW-w64..."
        Invoke-WebRequest -Uri $MingwUrl -OutFile $Archive -UseBasicParsing
        Write-Output "Extracting MinGW-w64..."
        python -c "import py7zr; py7zr.SevenZipFile(r'$Archive', mode='r').extractall(path=r'$ExtractTo')"
    } else {
        Write-Output "MinGW-w64 already present at $ExtractTo"
    }
}

Write-Output ""
Write-Output "Toolchain ready. Build with:"
Write-Output "  .\scripts\build_all.ps1"
