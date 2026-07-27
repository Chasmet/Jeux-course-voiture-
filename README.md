# Space Kart Legends — O3DE Android

Jeu de karting arcade 3D original pour Android, développé avec Open 3D Engine (O3DE) 26.05.

## Direction du jeu

- Sensations de conduite arcade inspirées des grands jeux de kart, sans reprendre de personnage, circuit, musique, logo ou objet Nintendo.
- Quatre pilotes 3D : Cheikh, Yvane, Nelvyn et Nova.
- Cinq circuits 3D dans l’espace : Orbite Zéro, Anneaux de Saturne, Nébuleuse Turbo, Station Titan et Trou Noir Final.
- Caméra troisième personne derrière le kart.
- Dérapage, mini-turbo, tremplins, portions antigravité, objets bonus et trois adversaires IA.
- Cible Android, orientation paysage, 60 images/s visées sur appareils récents et mode 30 images/s pour appareils modestes.

## État de cette branche

Cette branche remplace l’ancien prototype HTML 2D par une structure de projet O3DE C++ et un premier prototype procédural. Les karts, pilotes et pistes de la première version sont construits à partir de primitives 3D afin de tester rapidement le gameplay avant l’intégration des modèles artistiques finaux.

## Structure

- `Gem/` : code C++ du gameplay O3DE.
- `Assets/SpaceKart/Data/` : données des pilotes et des circuits.
- `scripts/` : scripts de préparation et d’export Android.
- `.github/workflows/` : validation du projet et export APK sur une machine O3DE configurée.

## Compilation O3DE

O3DE 26.05, Android SDK/NDK, JDK, Gradle, CMake et Ninja sont nécessaires. L’export Android officiel d’O3DE génère un projet Gradle puis construit le véritable APK.

```powershell
$env:O3DE_ENGINE_PATH = "C:\O3DE\26.05"
& "$env:O3DE_ENGINE_PATH\scripts\o3de.bat" register --project-path .
& "$env:O3DE_ENGINE_PATH\scripts\o3de.bat" export-project `
  -es "$env:O3DE_ENGINE_PATH\scripts\o3de\ExportScripts\export_source_android.py" `
  -pp . `
  -abp build\game-android `
  --config profile `
  --asset-mode PAK
```

L’APK produit se trouve ensuite dans le dossier Gradle généré, sous `app/build/outputs/apk/`.

## Contrôles du prototype

- Clavier : `A/D` ou flèches gauche/droite pour tourner.
- Accélération automatique.
- `Espace` : dérapage/mini-turbo.
- `B` : boost d’objet.
- `R` : replacer le kart sur la piste.
- Manette : stick gauche pour tourner, bouton A pour déraper, bouton B pour l’objet.

Les contrôles tactiles Android seront reliés au HUD LyShine dans l’étape suivante.