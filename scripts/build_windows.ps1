param(
    [Parameter(Mandatory = $true)]
    [string]$JuceDir,

    [string]$BuildDir = "",

    [ValidateSet("x64", "ARM64")]
    [string]$Architecture = "x64"
)

$ErrorActionPreference = "Stop"
$ProjectDir = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $ProjectDir "build/windows-$Architecture"
}

if (-not (Test-Path (Join-Path $JuceDir "CMakeLists.txt"))) {
    throw "JUCE was not found at '$JuceDir'. Pass the JUCE 9 source folder."
}

cmake -S $ProjectDir -B $BuildDir -G "Visual Studio 17 2022" -A $Architecture `
    -DJUCE_DIR="$JuceDir" `
    -DGLO_FETCH_JUCE=OFF `
    -DGLO_BUILD_PLUGIN=ON `
    -DGLO_BUILD_TESTS=ON

cmake --build $BuildDir --config Release `
    --target 808GloPro_All 808GloCoreTests 808GloJuceIntegrationTests 808GloRenderPreview --parallel
ctest --test-dir $BuildDir -C Release --output-on-failure

Write-Host ""
Write-Host "Build finished. Unsigned artifacts are under:"
Write-Host (Join-Path $BuildDir "808GloPro_artefacts/Release")
Write-Host "Follow Docs/BUILD_WINDOWS.md before distributing them."
