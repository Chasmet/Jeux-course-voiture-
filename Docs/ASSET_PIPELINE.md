# Pipeline des personnages, karts et animations

## Principe

Les photos et vidéos de référence définissent l’apparence et les mouvements, mais une vidéo MP4 ne constitue pas un acteur 3D jouable. Le jeu utilise deux niveaux d’assets :

1. **Blocage automatique glTF** : modèles légers générés par `scripts/generate_blockout_assets.py` pour tester immédiatement échelle, couleurs, caméra et gameplay.
2. **Production FBX** : modèles finaux avec topologie propre, textures, squelette, skinning et animations.

Les références privées ne sont pas stockées dans le dépôt. Seuls les modèles originaux produits pour le jeu doivent être ajoutés.

## Arborescence finale

```text
Assets/
├── Characters/
│   ├── Cheikh/
│   │   ├── cheikh.fbx
│   │   ├── cheikh_bodycolor.png
│   │   ├── cheikh_normal.png
│   │   └── cheikh_orm.png
│   ├── Yvane/yvane.fbx
│   ├── Nelvyn/nelvyn.fbx
│   └── Nova/nova.fbx
├── Vehicles/
│   ├── AzureComet/azure_comet.fbx
│   ├── SolarStrike/solar_strike.fbx
│   ├── EmeraldPulse/emerald_pulse.fbx
│   └── VioletPhoton/violet_photon.fbx
└── Animation/
    ├── driver_adult.fbx
    ├── driver_tall_child.fbx
    ├── driver_child.fbx
    └── driver_adult_light.fbx
```

## Contraintes pilotes Android

- Une unité de scène correspond à un mètre.
- Personnage orienté vers l’axe avant du véhicule lors de l’export.
- Environ 15 000 à 30 000 triangles par pilote au niveau de détail principal.
- Textures 1K pour le jeu mobile, 2K uniquement pour les visages si le budget mémoire le permet.
- Trois niveaux de détail : LOD0, LOD1 et LOD2.
- Un seul squelette compatible pour les pilotes de même catégorie de proportions.
- Transformations appliquées avant export ; aucune échelle négative.

## Squelette minimal

```text
root
└── pelvis
    ├── spine_01
    │   └── spine_02
    │       ├── neck
    │       │   └── head
    │       ├── clavicle_l → upperarm_l → lowerarm_l → hand_l
    │       └── clavicle_r → upperarm_r → lowerarm_r → hand_r
    ├── thigh_l → calf_l → foot_l
    └── thigh_r → calf_r → foot_r
```

Les mains doivent rester correctement fixées au volant par contraintes ou IK. Le bassin doit rester stable dans le siège et les jambes doivent conserver une position crédible autour du châssis.

## Clips obligatoires

- `idle`
- `steer_left`
- `steer_right`
- `drift_left`
- `drift_right`
- `boost`
- `brake`
- `jump`
- `land`
- `impact_left`
- `impact_right`
- `victory`
- `defeat`

Les clips de conduite restent sur place. Le déplacement du kart est géré par le gameplay, pas par le mouvement racine du personnage.

## Hiérarchie des karts

```text
kart_root
├── body
├── steering_wheel
├── wheel_front_l
├── wheel_front_r
├── wheel_rear_l
├── wheel_rear_r
├── suspension_front_l
├── suspension_front_r
├── suspension_rear_l
├── suspension_rear_r
├── driver_socket
├── boost_socket_l
└── boost_socket_r
```

Les roues avant doivent pouvoir pivoter pour la direction et les quatre roues tourner autour de leur axe. Le `driver_socket` fixe la position du pilote. Les deux sockets arrière servent aux flammes du turbo.

## Import O3DE

Le format de production prioritaire est FBX. L’Asset Processor génère ensuite les assets optimisés. Les personnages sont utilisés avec EMotionFX via un Actor et un AnimGraph. Les modèles glTF générés automatiquement sont uniquement des remplacements temporaires.

## Ordre de remplacement

1. Générer les blocages glTF.
2. Valider les proportions dans les karts.
3. Créer les quatre maillages finaux.
4. Créer et tester les squelettes.
5. Produire les clips d’animation.
6. Exporter en FBX.
7. Configurer les Actor, Motion Set et AnimGraph O3DE.
8. Remplacer les chemins de blocage par les chemins de production dans `Assets/Config/game_content.json`.
9. Tester le rendu et les performances sur Android.
