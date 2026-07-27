param(
    [string]$EnginePath = $env:O3DE_ENGINE_PATH
)

$ErrorActionPreference = "Stop"
if ([string]::IsNullOrWhiteSpace($EnginePath)) {
    throw "Définis O3DE_ENGINE_PATH vers l’installation O3DE."
}

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$sourceLevel = Join-Path $EnginePath "Templates\DefaultProject\Template\Levels\DefaultLevel\DefaultLevel.prefab"
$targetDir = Join-Path $projectRoot "Levels\SpaceKartArena"
$targetLevel = Join-Path $targetDir "SpaceKartArena.prefab"

if (!(Test-Path $sourceLevel)) {
    throw "Niveau modèle O3DE introuvable : $sourceLevel"
}

New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
if (!(Test-Path $targetLevel)) {
    Copy-Item $sourceLevel $targetLevel
    Write-Host "Niveau SpaceKartArena créé depuis le modèle O3DE."
}

& (Join-Path $EnginePath "scripts\o3de.bat") register --this-engine
& (Join-Path $EnginePath "scripts\o3de.bat") register -pp $projectRoot
Write-Host "Projet CHK Space Kart enregistré dans O3DE."
