# Pilotes réels privés — priorité de production

Les modèles ressemblants de **Cheikh, Yvane et Nelvyn** sont la priorité absolue du projet.

## Protection des visages

Le dépôt étant public, les modèles dérivés des photos et vidéos ne doivent jamais y être ajoutés directement. Les fichiers privés sont installés localement dans :

```text
Assets/PrivateCharacters/Cheikh/
Assets/PrivateCharacters/Yvane/
Assets/PrivateCharacters/Nelvyn/
```

Ces dossiers ainsi que `Assets/Config/private_character_overrides.json` sont exclus par `.gitignore`.

## Pack V2 attendu

Chaque pilote V2 possède :

- un atlas de visage multi-angles 1024 × 512 dérivé des vraies photos ;
- une géométrie de tête distincte ;
- ses proportions et sa coiffure ;
- un squelette de 20 articulations ;
- les sockets `kart_seat_socket`, `steering_left_socket`, `steering_right_socket` et `face_camera_socket` ;
- 13 animations : `drive_idle`, `steer_left`, `steer_right`, `drift_left`, `drift_right`, `boost`, `brake`, `jump`, `land`, `impact_left`, `impact_right`, `spin`, `victory`.

Aucune photo ou vidéo brute ne doit être incluse dans le pack.

## Installation privée

```bash
python scripts/install_private_character_pack.py /chemin/SpaceKartLegends-vrais-pilotes-rigges-v2.zip
```

Le script vérifie les trois pilotes, les SHA-256, le squelette, les animations et l’absence de médias bruts avant l’installation.

## Intégration O3DE

Les GLB V2 sont les sources riggées privées. O3DE utilise principalement les fichiers FBX pour générer les produits EMotionFX `.actor` et `.motion`. La conversion FBX, le traitement par Asset Processor, les composants Actor et le graphe d’animation doivent être validés pendant la compilation réelle O3DE. Les anciens modèles de blocage restent uniquement un secours technique tant que l’acteur privé n’est pas disponible.
