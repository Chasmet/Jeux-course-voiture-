# CHK Space Kart — O3DE Android

Prototype de jeu de karting 3D spatial conçu avec Open 3D Engine (O3DE).

## Contenu V1

- 4 pilotes : Cheikh, Yvane, Nelvin et Nova-7.
- 5 circuits : Orbite de Mars, Anneaux de Saturne, Éclipse lunaire, Faille de la Nébuleuse et Singularité finale.
- 3 tours par course.
- Trois adversaires pilotés par IA.
- Commandes tactiles Android : moitié gauche pour diriger, moitié droite pour activer le boost.
- Clavier : flèches gauche/droite et Espace pour le boost.
- Manette : stick gauche et bouton A.
- Affichage 3D procédural temporaire via le Gem DebugDraw d’O3DE.
- Changement automatique vers le circuit suivant après chaque course.

## État réel

Cette branche contient une base O3DE native C++ destinée à produire un APK Android réel par l’outil d’export O3DE. Ce n’est pas encore une version commerciale : les karts et circuits sont représentés par de la géométrie procédurale de prototype. Les modèles 3D, textures, sons, menus et effets VFX finaux restent à intégrer.

## Préparer le projet

Prérequis : O3DE 26.05 enregistré, Android SDK API 34, NDK compatible O3DE, CMake, Ninja, JDK et une clé Android.

Sous Windows PowerShell :

```powershell
$env:O3DE_ENGINE_PATH = "C:\o3de"
.\scripts\prepare_project.ps1
& "$env:O3DE_ENGINE_PATH\scripts\o3de.bat" register -pp .
cmake -B build/windows -S . -G "Visual Studio 17 2022"
cmake --build build/windows --target SpaceKart.GameLauncher Editor --config profile
```

## Exporter le véritable APK

```powershell
$env:O3DE_ENGINE_PATH = "C:\o3de"
.\scripts\export_android.ps1 -Configuration profile
```

L’export officiel O3DE génère le projet Gradle dans `build/game-android`, puis le véritable APK. Les fichiers Gradle Android ne sont donc pas falsifiés ni écrits à la main : ils sont générés par O3DE.

## GitHub Actions

Le workflow `.github/workflows/build-android.yml` utilise un runner Windows auto-hébergé portant les labels `self-hosted`, `Windows`, `X64`, `o3de`. O3DE et le SDK Android doivent y être installés, car le moteur et ses dépendances dépassent les capacités pratiques d’un runner GitHub standard.

L’APK compilé est publié comme Artifact GitHub sous le nom `CHK-Space-Kart-APK`.

## Structure

```text
.
├── Assets/SpaceKart/gameplay.json
├── Config/shader_global_build_options.json
├── Gem/
│   ├── Include/SpaceKart/SpaceKartTypeIds.h
│   ├── Platform/Android/
│   ├── Source/SpaceKartModule.cpp
│   ├── Source/SpaceKartSystemComponent.cpp
│   ├── Source/SpaceKartSystemComponent.h
│   ├── CMakeLists.txt
│   └── gem.json
├── Platform/Android/android_project.json
├── Registry/load_level.setreg
├── scripts/prepare_project.ps1
├── scripts/export_android.ps1
├── .github/workflows/build-android.yml
├── CMakeLists.txt
└── project.json
```
