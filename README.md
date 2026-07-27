# Space Kart Legends — O3DE Android

Jeu de karting arcade 3D original pour Android, développé avec Open 3D Engine 26.05.

## Direction du jeu

- Quatre pilotes : Cheikh, Yvane, Nelvyn et Nova.
- Quatre karts futuristes personnalisés : Comète Azur, Frappe Solaire, Impulsion Émeraude et Photon Violet.
- Cinq circuits dans l’espace : Orbite Zéro, Anneaux de Saturne, Nébuleuse Turbo, Station Titan et Trou Noir Final.
- Caméra troisième personne derrière le kart.
- Accélération automatique, freinage, dérapage, mini-turbo, saut et adversaires IA.
- Jeu original : aucun personnage, circuit, musique, logo ou objet Nintendo n’est repris.

## État de la branche

La branche `agent/o3de-space-kart-3d-v1` remplace l’ancien prototype HTML 2D par un projet O3DE C++.

Déjà présents :

- boucle de course en trois tours ;
- cinq géométries de circuit ;
- quatre pilotes avec statistiques différentes ;
- IA et classement ;
- dérapage chargé et mini-turbo ;
- boost, freinage, saut et récupération ;
- caméra de poursuite ;
- commandes clavier, manette et écran tactile Android ;
- générateur de huit vrais modèles glTF de blocage ;
- validation automatique GitHub Actions ;
- workflow d’export Android produisant un véritable APK sur un runner O3DE configuré.

Les modèles glTF sont des remplacements techniques originaux. Les versions finales ressemblantes, riggées et animées devront être exportées en FBX puis traitées par EMotionFX. Les vidéos fournies servent de références de mouvement ; elles ne sont pas utilisées comme fausses animations 3D.

## Structure

```text
Assets/Config/game_content.json       Pilotes, karts, circuits et animations
Assets/Blockout/                      Modèles glTF générés automatiquement
Docs/ASSET_PIPELINE.md                Pipeline FBX, squelette et EMotionFX
Gem/Source/                           Gameplay C++ O3DE
scripts/generate_blockout_assets.py   Génération des huit modèles temporaires
scripts/validate_project.py           Contrôles structurels et régressions
.github/workflows/                    Validation et export Android
```

## Génération des modèles temporaires

```bash
python scripts/generate_blockout_assets.py
python scripts/validate_project.py
```

Le premier script produit quatre pilotes et quatre karts séparés dans `Assets/Blockout/`. Le second vérifie les modèles, les données du jeu, les fichiers C++ et le correctif de trajectoire.

## Contrôles

### Android tactile

- moitié gauche : direction analogique ;
- zone supérieure droite : turbo ;
- zone centrale droite : dérapage ;
- zone inférieure droite : freinage ;
- accélération automatique.

### Clavier et manette

- `A/D` ou flèches gauche/droite : direction ;
- `W` ou flèche haut : accélérer ;
- `S` ou flèche bas : freiner ;
- `Espace` ou bouton A : déraper ;
- `B` ou bouton B : turbo ;
- `R` : replacer le kart ;
- `N` : circuit suivant.

## Export Android

Le workflow `.github/workflows/build-android.yml` utilise l’export Android officiel d’O3DE. Il génère les modèles temporaires, valide le projet, produit le projet Gradle, construit le véritable APK, calcule son SHA-256 puis le publie comme artifact GitHub.

Ce workflow nécessite un runner Linux préparé avec O3DE 26.05, Android SDK/NDK, JDK, Gradle, CMake et Ninja. Aucun faux APK ou fichier renommé n’est créé.

Exemple d’export local :

```bash
$O3DE_ENGINE_PATH/scripts/o3de.sh export-project \
  -es export_source_android.py \
  -pp . \
  -abp build/android \
  -ll INFO \
  --config profile \
  --asset-mode PAK
```

## Limites actuelles

- Aucune compilation O3DE complète n’a encore été exécutée dans cet environnement.
- Aucun APK n’est encore disponible.
- Les pilotes réalistes riggés, les animations EMotionFX, le HUD graphique LyShine, les objets bonus et les décors artistiques complets restent à intégrer.
- Le rendu procédural actuel sert à tester le gameplay avant le remplacement par les assets finaux.
