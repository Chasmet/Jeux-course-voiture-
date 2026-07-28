# Space Kart Legends — Spécification de gameplay

## Boucle de jeu

1. Choix du pilote et de son kart associé.
2. Choix d’un circuit débloqué.
3. Grille de départ et compte à rebours de 3,5 secondes.
4. Course en trois tours contre trois adversaires IA.
5. Collecte d’objets, dérapages, mini-turbos, sauts et attaques.
6. Classement final, récompenses et déblocage du circuit suivant.

## Sensations de conduite

- Accélération automatique sur Android.
- Direction analogique sur toute la moitié gauche de l’écran.
- Le kart reste stable à petite vitesse et devient plus sensible à grande vitesse.
- Maintenir le dérapage dans un virage charge progressivement un mini-turbo.
- Relâcher le dérapage déclenche une poussée dont la durée dépend de la charge.
- Le frein réduit fortement la vitesse mais conserve la direction.
- Un kart touché ralentit, dévie ou tourne temporairement selon l’objet reçu.
- La caméra reste derrière le kart, légèrement surélevée, et regarde vers la sortie du virage.

## Pilotes

### Cheikh — équilibré

- Vitesse : 7/10
- Accélération : 7/10
- Maniabilité : 7/10
- Poids : 8/10
- Kart : Comète Azur
- Style : stable, puissant et accessible.

### Yvane — vitesse

- Vitesse : 9/10
- Accélération : 7/10
- Maniabilité : 6/10
- Poids : 5/10
- Kart : Frappe Solaire
- Style : rapide dans les lignes droites, plus exigeant dans les virages.

### Nelvyn — maniabilité

- Vitesse : 6/10
- Accélération : 9/10
- Maniabilité : 9/10
- Poids : 4/10
- Kart : Impulsion Émeraude
- Style : très réactif, excellent pour les dérapages et les relances.

### Nova — technique

- Vitesse : 8/10
- Accélération : 8/10
- Maniabilité : 7/10
- Poids : 5/10
- Kart : Photon Violet
- Style : polyvalent et efficace avec les objets.

## Objets originaux

### Turbo Comète

Accélération personnelle forte pendant 1,85 seconde. Plus fréquent pour les pilotes en troisième ou quatrième position.

### Bouclier Plasma

Protection pendant cinq secondes. Le premier impact est absorbé et détruit le bouclier.

### Mine Gravitationnelle

Attaque le concurrent le plus proche placé derrière. Elle provoque une rotation et une perte de vitesse.

### Impulsion Photon

Touche le concurrent le plus proche placé devant. Elle ralentit et déstabilise brièvement sa trajectoire.

Quatre zones de collecte sont réparties régulièrement sur chaque circuit. La distribution dépend de la position afin de garder les courses disputées sans supprimer l’avantage du meilleur pilote.

## Circuits

### 1. Orbite Zéro

Circuit d’apprentissage large et lisible. Virages progressifs, petites ondulations et grandes zones de dérapage.

### 2. Anneaux de Saturne

Piste plus étroite avec variations de hauteur, poussières d’anneaux et courbes rapides.

### 3. Nébuleuse Turbo

Circuit lumineux et nerveux avec fortes ondulations, zones de boost et visibilité changeante.

### 4. Station Titan

Tracé technique autour d’une station industrielle, passages resserrés et enchaînements précis.

### 5. Trou Noir Final

Circuit le plus difficile, fort relief, ambiance sombre, virages serrés et attraction visuelle du trou noir au centre.

## IA

- Trois profils légèrement différents en vitesse et trajectoire.
- Correction modérée pour maintenir une course serrée sans téléportation.
- Utilisation autonome des objets.
- Déviation de trajectoire lors des impacts.
- Choix de ligne oscillant autour du centre de la piste.
- Les effets de ralentissement, rotation, bouclier et turbo s’appliquent aussi à l’IA.

## Interface Android provisoire

- Haut gauche : circuit, pilote et position.
- Sous le classement : tour, vitesse et charge de mini-turbo.
- Moitié gauche : direction.
- Haut droit : objet.
- Centre droit : dérapage.
- Bas droit : frein.
- Centre écran : compte à rebours et résultat de course.

Le HUD actuel est dessiné par le prototype C++. La version artistique finale devra être reconstruite avec LyShine en conservant exactement ces zones tactiles.

## Progression prévue

- Orbite Zéro disponible dès le début.
- Une victoire ou un podium débloque le circuit suivant.
- Chaque pilote possède des défis de maîtrise.
- Les récompenses cosmétiques modifient uniquement les couleurs, traînées de turbo et effets de victoire.
- Aucun achat n’est nécessaire pour améliorer les statistiques de conduite.

## Critères de validation Android

- 30 images par seconde stables sur appareil modeste.
- 60 images par seconde visées sur appareil récent.
- Aucun bouton tactile recouvert par une encoche ou la barre système.
- Direction utilisable avec le pouce gauche sans masquer le kart.
- Dérapage et objet utilisables simultanément avec deux doigts.
- Reprise correcte après mise en arrière-plan.
- Course complète sur chacun des cinq circuits sans blocage ni sortie définitive de piste.
