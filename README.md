# Space Kart Legends — O3DE Android

Jeu de karting arcade 3D original pour Android, développé avec Open 3D Engine 26.05.

## Direction du jeu

- Quatre pilotes sélectionnables : Cheikh, Yvane, Nelvyn et Nova.
- Quatre karts futuristes : Comète Azur, Frappe Solaire, Impulsion Émeraude et Photon Violet.
- Cinq circuits spatiaux : Orbite Zéro, Anneaux de Saturne, Nébuleuse Turbo, Station Titan et Trou Noir Final.
- Championnat complet sur cinq courses avec attribution de points.
- Caméra troisième personne derrière le kart.
- Accélération automatique, freinage, dérapage, mini-turbo, objets, saut et adversaires IA.
- Jeu original : aucun personnage, circuit, musique, logo ou objet Nintendo n’est repris.

## État de la branche

La branche `agent/o3de-space-kart-3d-v1` remplace l’ancien prototype HTML 2D par un projet O3DE C++.

Déjà présents :

- écran de sélection du pilote ;
- compte à rebours et courses en trois tours ;
- enchaînement automatique des cinq circuits ;
- résultats après chaque course et score final du championnat ;
- IA, classement et rattrapage modéré ;
- dérapage chargé et mini-turbo ;
- freinage, saut et récupération ;
- quatre objets originaux avec effets sur le joueur et l’IA ;
- caméra de poursuite autonome créée au lancement si nécessaire ;
- commandes clavier, manette et écran tactile Android ;
- niveau O3DE de démarrage autonome ;
- configuration Android paysage `com.chasmet.spacekartlegends` ;
- profils graphiques Android avec plusieurs échelles de rendu ;
- génération automatique de 17 modèles glTF ;
- validation GitHub Actions ;
- workflow d’export Android produisant un véritable APK sur un runner O3DE configuré.

Les modèles glTF sont des remplacements techniques originaux. Les versions finales ressemblantes, riggées et animées devront être exportées en FBX puis traitées par EMotionFX. Les vidéos personnelles servent uniquement de références de mouvement.

## Modèles 3D générés

Le pipeline génère automatiquement :

- 4 pilotes ;
- 4 karts ;
- 5 pistes complètes avec largeur et épaisseur ;
- 4 objets de course.

Tous les fichiers sont des glTF 2.0 avec géométrie intégrée et sont contrôlés par GitHub Actions.

## Objets

- **Turbo Comète** : forte accélération personnelle.
- **Bouclier Plasma** : absorbe le prochain impact.
- **Mine Gravitationnelle** : perturbe le concurrent placé derrière.
- **Impulsion Photon** : ralentit le concurrent placé devant.

La distribution varie selon la position afin de conserver des courses disputées.

## Structure

```text
Assets/Config/game_content.json          Pilotes, karts, circuits, objets et animations
Assets/Blockout/                         17 modèles glTF générés automatiquement
Levels/SpaceKartLegends/                 Niveau de démarrage O3DE
Platform/Android/                        Package, version et orientation Android
Registry/                                Chargement du niveau et qualité mobile
Docs/ASSET_PIPELINE.md                   Pipeline FBX, squelette et EMotionFX
Docs/GAMEPLAY_SPEC.md                    Règles de course et expérience mobile
Gem/Source/                              Gameplay C++ O3DE
scripts/generate_blockout_assets.py      Pilotes et karts temporaires
scripts/generate_environment_assets.py   Pistes et objets temporaires
scripts/validate_project.py              Données, gameplay et assets
scripts/validate_environment_assets.py   Géométrie des pistes et objets
scripts/validate_android_scaffold.py     Niveau et configuration Android
.github/workflows/                       Validation et export Android
```

## Génération et validation

```bash
python scripts/generate_blockout_assets.py
python scripts/generate_environment_assets.py
python scripts/validate_project.py
python scripts/validate_environment_assets.py
python scripts/validate_android_scaffold.py
```

## Contrôles

### Sélection et championnat

- gauche/droite ou zones tactiles gauche/centre : choisir le pilote ;
- `Entrée`, bouton A/B ou zone tactile droite : valider ;
- après une course : valider pour lancer le circuit suivant ;
- après le cinquième circuit : valider pour recommencer le championnat.

### Course Android tactile

- moitié gauche : direction analogique ;
- zone supérieure droite : utiliser l’objet ;
- zone centrale droite : dérapage ;
- zone inférieure droite : freinage ;
- accélération automatique.

### Course clavier et manette

- `A/D` ou flèches gauche/droite : direction ;
- `W` ou flèche haut : accélérer ;
- `S` ou flèche bas : freiner ;
- `Espace` ou bouton A : déraper ;
- `B` ou bouton B : utiliser l’objet ;
- `R` : replacer le kart.

## Export Android

Le workflow `.github/workflows/build-android.yml` utilise le script officiel `export_source_android.py` d’O3DE. Il :

1. génère les 17 modèles temporaires ;
2. valide les données, la géométrie et le niveau de démarrage ;
3. configure Android API 34 et NDK 25 ;
4. génère le projet Gradle ;
5. construit le véritable APK ;
6. calcule son SHA-256 ;
7. publie l’APK comme artifact GitHub.

Ce workflow nécessite un runner Linux préparé avec O3DE 26.05, Android SDK/NDK, JDK, Gradle, CMake et Ninja. Aucun faux APK ou fichier renommé n’est créé.

Exemple d’export local :

```bash
$O3DE_ENGINE_PATH/scripts/o3de.sh export-project \
  --export-script "$O3DE_ENGINE_PATH/scripts/o3de/ExportScripts/export_source_android.py" \
  --project-path . \
  --android-build-path build/android \
  --config profile \
  --asset-mode PAK
```

## Validation actuelle

La validation GitHub contrôle avec succès les 17 maillages, les manifestes, le championnat, la sélection du pilote, les objets, le tactile, la caméra, le niveau de démarrage et la structure Android.

## Limites actuelles

- La compilation O3DE complète n’a pas encore été exécutée sur un runner équipé du moteur.
- Aucun APK n’est encore disponible.
- Les pilotes réalistes riggés, les animations EMotionFX, le HUD graphique LyShine, les sons et les décors artistiques définitifs restent à intégrer.
- Le rendu procédural et les glTF actuels servent à tester le gameplay avant le remplacement par les assets finaux.
