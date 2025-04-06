# ValoSDL

Par Maxime Quesnel élève en B2 à L'ESGI Grenoble pour le projet de fin de semestre en C.
Un jeu développé en C utilisant SDL2 où le joueur doit éviter des balles qui rebondissent sur les bords de l'écran.

## Fonctionnalités

- Menu principal avec sélection de personnage
- Trois niveaux de difficulté (Facile, Intermédiaire, Difficile)
- Tutoriel interactif expliquant les contrôles
- Système de score avec classement par difficulté
- Menu de pause pendant le jeu
- Animations et effets visuels
- Gestion du son et de la musique
- Interface utilisateur intuitive

## Contrôles

- Flèches directionnelles : Déplacer le personnage
- Échap : Mettre le jeu en pause
- Souris : Navigation dans les menus

## Installation

### Prérequis

- SDL2
- SDL2_image
- SDL2_ttf
- SDL2_mixer
- MySQL/MariaDB

### Installation des dépendances

```bash
# macOS
brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer mysql

# Linux (Ubuntu/Debian)
sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libmysqlclient-dev
```

### Compilation

```bash
make
```

## Structure du projet

```
.
├── assets/           # Images et ressources
│   ├── arrow_keys.png
│   └── ...
├── sounds/          # Fichiers audio
│   ├── background.mp3
│   ├── button.wav
│   ├── collision.wav
│   ├── pause.wav
│   └── select.wav
├── user/            # Images des personnages
│   ├── astra.png
│   ├── breach.png
│   └── ...
├── game.c           # Code source principal
├── Makefile         # Fichier de compilation
└── README.md        # Documentation
```

## Fonctionnalités détaillées

### Menu Principal
- Sélection du personnage parmi une galerie d'images
- Accès au classement
- Options de jeu

### Sélection de Difficulté
- Facile : 3 balles, vitesse lente
- Intermédiaire : 6 balles, vitesse moyenne
- Difficile : 9 balles, vitesse rapide

### Tutoriel
- Instructions claires sur les contrôles
- Animation des touches directionnelles
- Explication du but du jeu

### Système de Score
- Enregistrement des scores par difficulté
- Classement séparé pour chaque niveau
- Sauvegarde dans une base de données MySQL

### Interface
- Menus interactifs avec effets de survol
- Animations fluides
- Retours visuels sur les actions
- Sons de feedback

## Contribution

Les contributions sont les bienvenues ! N'hésitez pas à :
1. Fork le projet
2. Créer une branche pour votre fonctionnalité
3. Commiter vos changements
4. Pousser vers la branche
5. Ouvrir une Pull Request

## Licence

Ce projet est sous licence MIT. Voir le fichier LICENSE pour plus de détails.
