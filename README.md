# ValoSDL

Par Maxime Quesnel élève en B2 à L'ESGI Grenoble pour le projet de fin de semestre en C.

Un jeu d'arcade développé en C avec SDL2, comprenant :
- Un menu principal avec sélection de personnage
- Trois niveaux de difficulté (Facile, Intermédiaire, Difficile)
- Un système de score avec base de données MySQL
- Un classement par niveau de difficulté

## Prérequis
- SDL2
- SDL2_image
- SDL2_ttf
- MySQL

## Installation

1. Installez les dépendances :
```bash
# Sur macOS avec Homebrew
brew install sdl2 sdl2_image sdl2_ttf mysql
```

2. Compilez le jeu :
```bash
make
```

3. Configurez la base de données :
```sql
CREATE DATABASE game_db;
USE game_db;
CREATE TABLE Scores (
    ID INT AUTO_INCREMENT PRIMARY KEY,
    Nom VARCHAR(50),
    Time INT,
    Difficulte VARCHAR(20)
);
```

## Contrôles
- Utilisez les touches fléchées pour déplacer le personnage
- Évitez les obstacles
- Essayez de survivre le plus longtemps possible

## Structure du projet
- `game.c` : Code source principal
- `user/` : Dossier contenant les sprites des personnages
- `Makefile` : Configuration de compilation
