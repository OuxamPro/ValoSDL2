# ValoSDL

Par Maxime Quesnel élève en B2 à L'ESGI Grenoble pour le projet de fin de semestre en C.

## Fonctionnalités

- Menu principal avec sélection de personnage
- Trois niveaux de difficulté (Facile, Intermédiaire, Difficile)
- Système de score avec classement
- Animations fluides lors de la sélection des personnages
- Effets sonores et musique de fond
- Menu pause pendant le jeu
- Interface utilisateur intuitive

## Dépendances

- SDL2
- SDL2_image
- SDL2_ttf
- SDL2_mixer
- MySQL
- SQLite3

## Installation

1. Installer les dépendances sur macOS :
```bash
brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer mysql-client
```

2. Cloner le dépôt :
```bash
git clone [URL_DU_REPO]
cd [NOM_DU_REPO]
```

3. Compiler le jeu :
```bash
make
```

## Structure des fichiers

- `game.c` : Code source principal
- `Makefile` : Fichier de compilation
- `sounds/` : Dossier contenant les fichiers audio
  - `background.mp3` : Musique de fond
  - `select.wav` : Son de sélection
  - `collision.wav` : Son de collision
  - `button.wav` : Son de bouton
  - `pause.wav` : Son de pause
- `user/` : Dossier contenant les images des personnages

## Contrôles

- Flèches directionnelles : Déplacer le personnage
- Échap : Mettre le jeu en pause
- Souris : Sélectionner les options dans les menus

## Niveaux de difficulté

- Facile : 3 balles, vitesse lente
- Intermédiaire : 6 balles, vitesse moyenne
- Difficile : 9 balles, vitesse rapide

## Contribution

Les contributions sont les bienvenues ! N'hésitez pas à :
1. Forker le projet
2. Créer une branche pour votre fonctionnalité
3. Commiter vos changements
4. Pousser vers la branche
5. Ouvrir une Pull Request

## Licence

Ce projet est sous licence MIT. Voir le fichier `LICENSE` pour plus de détails.
