# Building and Releasing 808Glo Pro on Windows

This guide produces the native 64-bit Windows deliverables for 808Glo Pro:

- x64 VST3 bundle
- x64 Standalone application
- Signed Inno Setup installer

Run the release build on native 64-bit Windows with MSVC. A MinGW, Wine, Linux cross-compile, or unsigned local build is not a Windows release candidate.

## Fixed product identity

Do not change these values after release. Existing DAW projects use the plug-in identity.

| Field | Value |
|---|---|
| Product | 808Glo Pro |
| Version | 1.0.1 |
| Company | Diamond Loopz |
| Bundle ID | `com.diamondloopz.808glopro` |
| Manufacturer code | `Dlpz` |
| Plug-in code | `Eglo` |
| JUCE | 9.0.1 |
| C++ standard | C++20 |
| Minimum CMake | 3.22 |
| Installer AppId | `{347184D2-0C31-4D6F-82AF-1A7A3ACE6C29}` |

Keep the Installer AppId unchanged for all 1.x updates so Windows recognizes upgrades correctly.

## Release prerequisites

Install or provide all of the following:

- Windows 11 x64 release machine or clean virtual machine.
- Visual Studio 2022 with Desktop development with C++, MSVC x64 tools, and a current Windows SDK.
- CMake 3.22 or newer.
- Git if JUCE is a Git checkout.
- Python 3 for factory-preset validation.
- A local JUCE 9.0.1 source checkout.
- `pluginval` for plug-in validation.
- Steinberg's VST3 `validator.exe`.
- Windows SDK `signtool.exe`.
- A trusted Authenticode code-signing certificate available through the Windows certificate store, hardware token, or the certificate provider's supported signing client.
- A current Inno Setup 6 release with `x64compatible` architecture support.
- A commercial JUCE licence appropriate for this closed-source commercial release, unless the complete release is distributed in compliance with JUCE's applicable open-source licence.

Use an x64 Native Tools Command Prompt or Developer PowerShell for Visual Studio. Confirm the tools:

```powershell
cmake --version
cl
python --version
where.exe signtool.exe
```

Verify JUCE exactly:

```powershell
$JuceDir = "C:\SDKs\JUCE"
if (-not (Test-Path "$JuceDir\CMakeLists.txt")) { throw "JUCE_DIR is invalid" }
git -C $JuceDir describe --tags --exact-match
```

The final command must report `9.0.1`. Do not build a release from JUCE `master`, JUCE 8, or an unreviewed later revision.

## 1. Configure a clean x64 build

Open Developer PowerShell at the project root:

```powershell
Set-Location "C:\absolute\path\to\808GloPro"

$Root = (Get-Location).Path
$Build = Join-Path $Root "build\windows-release"
$JuceDir = "C:\SDKs\JUCE"
$Version = "1.0.1"
```

Use a new build directory for every release candidate. Do not reuse a tree created with another JUCE version, generator, architecture, or compiler toolset.

```powershell
cmake -S $Root -B $Build -G "Visual Studio 17 2022" -A x64 `
  "-DJUCE_DIR:PATH=$JuceDir" `
  -DGLO_FETCH_JUCE=OFF `
  -DGLO_BUILD_PLUGIN=ON `
  -DGLO_BUILD_TESTS=ON `
  -DGLO_BUILD_TOOLS=ON

if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
```

Review the configure output. It must show the intended MSVC x64 toolchain and JUCE 9 without an unexpected dependency fallback.

## 2. Validate presets and build

```powershell
python tools\validate_presets.py Resources\FactoryPresets.json
if ($LASTEXITCODE -ne 0) { throw "Factory preset validation failed" }
```

The release bank must report 128 curated presets.

Build the release products and tests:

```powershell
cmake --build $Build --config Release `
  --target 808GloPro_All 808GloCoreTests 808GloJuceIntegrationTests 808GloRenderPreview `
  --parallel

if ($LASTEXITCODE -ne 0) { throw "Release build failed" }

ctest --test-dir $Build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Core tests failed" }
```

Any compiler warning from 808Glo Pro code, test failure, or preset validation failure blocks release.

## 3. Locate and inspect the artifacts

```powershell
$Artifacts = Join-Path $Build "808GloPro_artefacts\Release"
$VstBundle = Join-Path $Artifacts "VST3\808Glo Pro.vst3"
$VstBinary = Join-Path $VstBundle "Contents\x86_64-win\808Glo Pro.vst3"
$Standalone = Join-Path $Artifacts "Standalone\808Glo Pro.exe"

foreach ($Path in @($VstBundle, $VstBinary, $Standalone)) {
    if (-not (Test-Path $Path)) { throw "Missing release artifact: $Path" }
}
```

The Windows VST3 is a bundle directory. Package the complete `808Glo Pro.vst3` directory, including `Contents` and `moduleinfo.json`; never install only the inner binary.

Inspect dependencies from a Visual Studio Developer shell:

```powershell
dumpbin /dependents $VstBinary
dumpbin /dependents $Standalone
```

There must be no accidental dependency on a developer-only DLL, debug runtime, private SDK folder, or unshipped third-party library.

## 4. Sign the binaries

Finish all resource and metadata changes before signing. Configure the signing certificate and RFC 3161 timestamp service through environment variables or the certificate provider's secure client. Do not commit certificate files or passwords.

Certificate-store example:

```powershell
$CertThumbprint = $env:DIAMOND_LOOPZ_WINDOWS_CERT_SHA1
$TimestampUrl = $env:DIAMOND_LOOPZ_TIMESTAMP_URL

if ([string]::IsNullOrWhiteSpace($CertThumbprint)) {
    throw "DIAMOND_LOOPZ_WINDOWS_CERT_SHA1 is not set"
}
if ([string]::IsNullOrWhiteSpace($TimestampUrl)) {
    throw "DIAMOND_LOOPZ_TIMESTAMP_URL is not set"
}

signtool.exe sign /sha1 $CertThumbprint /fd SHA256 `
  /tr $TimestampUrl /td SHA256 $VstBinary
if ($LASTEXITCODE -ne 0) { throw "VST3 signing failed" }

signtool.exe sign /sha1 $CertThumbprint /fd SHA256 `
  /tr $TimestampUrl /td SHA256 $Standalone
if ($LASTEXITCODE -ne 0) { throw "Standalone signing failed" }
```

If the certificate is in the Local Machine store, add `/sm`. Hardware-token and cloud-signing certificates may require provider-specific options; follow the certificate issuer's current instructions while retaining SHA-256 file digests and RFC 3161 timestamping.

Verify both signatures:

```powershell
signtool.exe verify /pa /all /v $VstBinary
if ($LASTEXITCODE -ne 0) { throw "VST3 signature verification failed" }

signtool.exe verify /pa /all /v $Standalone
if ($LASTEXITCODE -ne 0) { throw "Standalone signature verification failed" }

Get-AuthenticodeSignature $VstBinary | Format-List
Get-AuthenticodeSignature $Standalone | Format-List
```

Both PowerShell results must show `Status : Valid`.

## 5. Run VST3 validation before packaging

```powershell
$ValidationDir = Join-Path $Root "dist\validation\windows"
New-Item -ItemType Directory -Force -Path $ValidationDir | Out-Null

$Pluginval = "C:\Tools\pluginval\pluginval.exe"
$Vst3Validator = "C:\Tools\vst3-validator\validator.exe"

& $Pluginval --strictness-level 10 --output-dir $ValidationDir $VstBundle
if ($LASTEXITCODE -ne 0) { throw "pluginval level 10 failed" }

& $Vst3Validator $VstBundle 2>&1 |
  Tee-Object -FilePath (Join-Path $ValidationDir "vst3-validator.log")
if ($LASTEXITCODE -ne 0) { throw "Steinberg VST3 validator failed" }
```

Both validators must exit with status 0. Do not suppress a failure, lower strictness for the public build, or replace binary validation with a Standalone launch.

## 6. Create the Inno Setup definition

Create `Packaging\Windows\808GloPro.iss` with the following release definition. Keep its AppId stable in every update.

```ini
#define MyAppName "808Glo Pro"
#define MyAppVersion "1.0.1"
#define MyAppPublisher "Diamond Loopz"
#define Artifacts "..\..\build\windows-release\808GloPro_artefacts\Release"

[Setup]
AppId={{347184D2-0C31-4D6F-82AF-1A7A3ACE6C29}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL=https://diamondloopz.com
DefaultDirName={autopf}\Diamond Loopz\808Glo Pro
DefaultGroupName=Diamond Loopz
DisableProgramGroupPage=yes
OutputDir=..\..\dist\windows
OutputBaseFilename=808GloPro-{#MyAppVersion}-Windows-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\808Glo Pro.exe
CloseApplications=force
RestartApplications=no

[Files]
Source: "{#Artifacts}\VST3\808Glo Pro.vst3\*"; DestDir: "{commoncf64}\VST3\808Glo Pro.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artifacts}\Standalone\808Glo Pro.exe"; DestDir: "{app}"; Flags: ignoreversion

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Icons]
Name: "{group}\808Glo Pro"; Filename: "{app}\808Glo Pro.exe"
Name: "{commondesktop}\808Glo Pro"; Filename: "{app}\808Glo Pro.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\808Glo Pro.exe"; Description: "Launch 808Glo Pro"; Flags: nowait postinstall skipifsilent unchecked
```

Factory presets are embedded in the application. The installer must not overwrite or delete user-created presets under AppData during an update or uninstall.

## 7. Build and sign the installer

```powershell
$Iscc = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
$Iss = Join-Path $Root "Packaging\Windows\808GloPro.iss"

& $Iscc $Iss
if ($LASTEXITCODE -ne 0) { throw "Inno Setup build failed" }

$Installer = Join-Path $Root `
  "dist\windows\808GloPro-$Version-Windows-x64-Setup.exe"
if (-not (Test-Path $Installer)) { throw "Installer was not created" }

signtool.exe sign /sha1 $CertThumbprint /fd SHA256 `
  /tr $TimestampUrl /td SHA256 $Installer
if ($LASTEXITCODE -ne 0) { throw "Installer signing failed" }

signtool.exe verify /pa /all /v $Installer
if ($LASTEXITCODE -ne 0) { throw "Installer signature verification failed" }

Get-AuthenticodeSignature $Installer | Format-List
```

The final status must be `Valid`. Never publish the unsigned Inno output or a package whose timestamp/signature verification failed.

## 8. Test the exact packaged installer

Use a clean Windows 11 x64 virtual machine or QA computer. Test the same signed installer intended for customers.

Interactive install:

```powershell
Start-Process -FilePath $Installer -Wait
```

Unattended QA install:

```powershell
$InstallProcess = Start-Process -FilePath $Installer `
  -ArgumentList "/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART", "/LOG=C:\Windows\Temp\808GloPro-install.log" `
  -Wait -PassThru

if ($InstallProcess.ExitCode -ne 0) {
    throw "Installer failed with exit code $($InstallProcess.ExitCode)"
}
```

Confirm the complete installed products:

```powershell
$InstalledVst = "C:\Program Files\Common Files\VST3\808Glo Pro.vst3"
$InstalledApp = "C:\Program Files\Diamond Loopz\808Glo Pro\808Glo Pro.exe"

if (-not (Test-Path $InstalledVst)) { throw "Installed VST3 is missing" }
if (-not (Test-Path $InstalledApp)) { throw "Installed Standalone app is missing" }

& $Pluginval --strictness-level 10 $InstalledVst
if ($LASTEXITCODE -ne 0) { throw "Installed VST3 failed pluginval" }

& $Vst3Validator $InstalledVst
if ($LASTEXITCODE -ne 0) { throw "Installed VST3 failed Steinberg validation" }
```

Launch the Standalone application and complete the Windows items in `QA_CHECKLIST.md`. Test the VST3 in FL Studio and at least two additional VST3 hosts.

Test uninstall from Windows Installed apps or directly on the QA machine:

```powershell
$Uninstaller = "C:\Program Files\Diamond Loopz\808Glo Pro\unins000.exe"
if (-not (Test-Path $Uninstaller)) { throw "Uninstaller is missing" }
Start-Process -FilePath $Uninstaller -Wait
```

Verify program binaries are removed while existing user-created preset data remains intact.

## 9. Hash and archive the release candidate

```powershell
$Hash = Get-FileHash -Algorithm SHA256 $Installer
$Hash | Format-List
$Hash.Hash + "  " + (Split-Path $Installer -Leaf) |
  Set-Content -Encoding ascii "$Installer.sha256"
```

Archive the signed installer, checksum, CMake configure output, compiler version, JUCE tag, validator logs, and completed QA checklist. Never archive private keys, certificate passwords, exported PFX files, or hardware-token credentials with the release.

## Current validation status

As of 2026-09-02, this workspace has not produced or tested a native Windows JUCE binary. Authenticode signing, pluginval level 10, Steinberg validation, host testing, installer installation/update/uninstall testing, and final packaging are pending. The installer is not approved for public release until every applicable gate above and in `RELEASE_CHECKLIST.md` is complete.

## Authoritative references

- [JUCE CMake API](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md)
- [JUCE 9.0.1 release](https://github.com/juce-framework/JUCE/releases/tag/9.0.1)
- [Steinberg VST3 bundle structure](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical%2BDocumentation/Locations%2BFormat/Plugin%2BFormat.html)
- [Steinberg VST3 plug-in locations](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical%2BDocumentation/Locations%2BFormat/Plugin%2BLocations.html)
- [Inno Setup AppId](https://jrsoftware.org/ishelp/topic_setup_appid.htm)
- [Inno Setup directory constants](https://jrsoftware.org/ishelp/topic_consts.htm)
- [pluginval](https://github.com/Tracktion/pluginval)
