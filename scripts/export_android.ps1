param(
    [ValidateSet("profile", "release")]
    [string]$Configuration = "profile",
    [string]$EnginePath = $env:O3DE_ENGINE_PATH,
    [string]$AndroidSdk = $env:ANDROID_HOME
)

$ErrorActionPreference = "Stop"
if ([string]::IsNullOrWhiteSpace($EnginePath)) { throw "O3DE_ENGINE_PATH manquant." }
if ([string]::IsNullOrWhiteSpace($AndroidSdk)) { throw "ANDROID_HOME manquant." }

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
& (Join-Path $PSScriptRoot "prepare_project.ps1") -EnginePath $EnginePath

$o3de = Join-Path $EnginePath "scripts\o3de.bat"
& $o3de android-configure --set-value platform.sdk.api=34
& $o3de android-configure --set-value sdk.root=$AndroidSdk
& $o3de android-configure --set-value asset.mode=PAK
& $o3de android-configure --validate

Push-Location $projectRoot
try {
    & $o3de export-project -es export_source_android.py -pp . -abp build\game-android -ll INFO --config $Configuration --asset-mode PAK --build-assets --fail-on-asset-errors
    if ($LASTEXITCODE -ne 0) { throw "Échec de l’export Android O3DE ($LASTEXITCODE)." }

    $apks = Get-ChildItem -Path (Join-Path $projectRoot "build\game-android") -Recurse -Filter *.apk
    if (!$apks) { throw "O3DE/Gradle n’a produit aucun APK." }
    $apks | ForEach-Object { Write-Host "APK réel : $($_.FullName)" }
}
finally {
    Pop-Location
}
