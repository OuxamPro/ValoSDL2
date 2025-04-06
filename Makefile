# Compilateur et options
CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lmysqlclient

# Chemins d'inclusion et de bibliothèque
INCLUDES = -I/usr/local/include/SDL2 -I/usr/include/mysql
LIBRARIES = -L/usr/local/lib -L/usr/lib/mysql

# Fichiers source et objet
SRC = game.c
OBJ = $(SRC:.c=.o)
EXECUTABLE = gameBase

# Règles de compilation
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ)
	$(CC) $(OBJ) -o $(EXECUTABLE) $(LDFLAGS) $(LIBRARIES)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Installation des dépendances
install-deps:
	# macOS
	brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer mysql
	# Linux (Ubuntu/Debian)
	# sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev libmysqlclient-dev

# Création des dossiers nécessaires
setup:
	mkdir -p assets sounds user

# Nettoyage
clean:
	rm -f $(OBJ) $(EXECUTABLE)

# Installation complète
install: install-deps setup all

.PHONY: all clean install install-deps setup 