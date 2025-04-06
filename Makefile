# Compilateur et options
CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lmysqlclient -lm

# Chemins d'inclusion
INCLUDES = -I/opt/homebrew/include/SDL2 -I/opt/homebrew/opt/mysql-client/include

# Chemins des bibliothèques
LIBRARIES = -L/opt/homebrew/lib -L/opt/homebrew/opt/mysql-client/lib

# Fichiers source et objet
SRC = game.c
OBJ = $(SRC:.c=.o)
EXEC = gameBase

# Règles de compilation
all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LIBRARIES) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Installation des dépendances
install-deps:
	brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer mysql-client

# Nettoyage
clean:
	rm -f $(OBJ) $(EXEC)

# Création des dossiers nécessaires
setup:
	mkdir -p sounds user

.PHONY: all clean install-deps setup 