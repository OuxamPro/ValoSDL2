# Compilateur
CC = gcc

# Flags de compilation
CFLAGS = -Wall -Wextra -g
SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS = $(shell sdl2-config --libs) -lSDL2_image -lSDL2_ttf -lmysqlclient

# Nom de l'exécutable
TARGET = game

# Fichiers source
SRCS = game.c

# Fichiers objets
OBJS = $(SRCS:.c=.o)

# Règle par défaut
all: $(TARGET)

# Règle de compilation
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(SDL_LIBS)

# Règle pour les fichiers objets
%.o: %.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -f $(OBJS) $(TARGET)

# Installation des dépendances (macOS)
install-deps:
	brew install sdl2 sdl2_image sdl2_ttf mysql

.PHONY: all clean install-deps 