#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <mysql/mysql.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <sqlite3.h>


#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define PLAYER_SPEED 5
#define BALL_RADIUS 20
#define EASY_BALLS 3
#define MEDIUM_BALLS 6
#define HARD_BALLS 9
#define EASY_SPEED 3
#define MEDIUM_SPEED 4
#define HARD_SPEED 5
#define CHARACTERS_PER_ROW 4
#define CHARACTER_SIZE 80
#define CHARACTER_PADDING 20

int currentBallCount = EASY_BALLS;
int currentBallSpeed = EASY_SPEED;

typedef struct {
    int x, y; // Position du centre
    int dx, dy; // Vitesse
} Ball;

typedef struct {
    int id;
    char nom[50];
    int time;
    char difficulte[20];  // Nouveau champ pour la difficulté
} Score;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* playerTexture = NULL;
SDL_Texture* backgroundTexture = NULL; // Texture du fond
SDL_Texture* menuBackgroundTexture = NULL; // Texture du fond du menu
SDL_Texture* ballTexture = NULL; // Nouvelle texture pour les balles
SDL_Rect player;
Ball* balls = NULL; // Tableau dynamique de balles
Uint32 startTime;
Score topScores[10];

char selectedCharacter[256] = "user/phoenix.png"; // Variable globale pour stocker le personnage sélectionné

// Variables globales pour l'audio
Mix_Music* backgroundMusic = NULL;
Mix_Chunk* selectSound = NULL;
Mix_Chunk* collisionSound = NULL;
Mix_Chunk* buttonSound = NULL;
Mix_Chunk* pauseSound = NULL;

void initAudio() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("Erreur d'initialisation de SDL_mixer: %s\n", Mix_GetError());
        return;
    }

    // Charger les sons
    backgroundMusic = Mix_LoadMUS("sounds/background.mp3");
    selectSound = Mix_LoadWAV("sounds/select.wav");
    collisionSound = Mix_LoadWAV("sounds/collision.wav");
    buttonSound = Mix_LoadWAV("sounds/button.wav");
    pauseSound = Mix_LoadWAV("sounds/pause.wav");

    // Vérifier le chargement des sons
    if (!backgroundMusic || !selectSound || !collisionSound || !buttonSound || !pauseSound) {
        printf("Erreur de chargement des sons: %s\n", Mix_GetError());
    }

    // Définir le volume
    Mix_VolumeMusic(50); // Volume de la musique à 50%
    Mix_Volume(-1, 64); // Volume des effets sonores à 64
}

void cleanupAudio() {
    if (backgroundMusic) Mix_FreeMusic(backgroundMusic);
    if (selectSound) Mix_FreeChunk(selectSound);
    if (collisionSound) Mix_FreeChunk(collisionSound);
    if (buttonSound) Mix_FreeChunk(buttonSound);
    if (pauseSound) Mix_FreeChunk(pauseSound);
    Mix_CloseAudio();
}

void finish_with_error(MYSQL *con) {
    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
    exit(1);
}

void createDatabase(MYSQL *con) {
    // Vérifier si la base de données existe, la créer si nécessaire
    if (mysql_query(con, "CREATE DATABASE IF NOT EXISTS game_db")) {
        finish_with_error(con);
    }
    // Utiliser la base de données
    if (mysql_query(con, "USE game_db")) {
        finish_with_error(con);
    }
    printf("Base de données 'game_db' sélectionnée.\n");
}

void getTopScores(MYSQL *con, const char* difficulte) {
    char query[200];
    sprintf(query, "SELECT * FROM Scores WHERE Difficulte = '%s' ORDER BY Time DESC LIMIT 10", difficulte);
    if (mysql_query(con, query)) {
        finish_with_error(con);
    }

    MYSQL_RES *result = mysql_store_result(con);
    if (result == NULL) {
        finish_with_error(con);
    }

    int num_fields = mysql_num_fields(result);
    MYSQL_ROW row;

    int i = 0;
    while ((row = mysql_fetch_row(result)) && i < 10) {
        topScores[i].id = atoi(row[0]);
        strncpy(topScores[i].nom, row[1], 49);
        topScores[i].nom[49] = '\0';
        topScores[i].time = atoi(row[2]);
        strncpy(topScores[i].difficulte, row[3], 19);
        topScores[i].difficulte[19] = '\0';
        i++;
    }

    // Remplir le reste du tableau avec des scores vides si nécessaire
    for (; i < 10; i++) {
        topScores[i].id = 0;
        topScores[i].nom[0] = '\0';
        topScores[i].time = 0;
        topScores[i].difficulte[0] = '\0';
    }

    mysql_free_result(result);
}

SDL_Texture* loadTexture(const char* path);
SDL_Texture* createTextTexture(const char* text, SDL_Color color);
void drawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius);
bool checkCollision(SDL_Rect a, Ball ball);
void initBalls();
void moveBalls();
void drawRoundedRect(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color color);
void displayTime(Uint32 startTime);
void displayGameOver(Uint32 startTime);
void displayMenu();
void startGame();
void displayScores();
void insertScore(MYSQL *con, const char* nom, int time, const char* difficulte);
void selectDifficulty();
void selectCharacter();
void displayPauseMenu();
void displayTutorial();

SDL_Texture* loadTexture(const char* path) {
    SDL_Surface* surface = IMG_Load(path);
    if (!surface) {
        printf("Erreur de chargement d'image : %s\n", IMG_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

SDL_Texture* createTextTexture(const char* text, SDL_Color color) {
    TTF_Font* font = TTF_OpenFont("KOMIKAX_.ttf", 24); // Ajuste le chemin et la taille de la police ici
    if (!font) {
        printf("Erreur de chargement de la police : %s\n", TTF_GetError());
        return NULL;
    }
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text, color);
    if (!textSurface) {
        printf("Erreur de création de la surface du texte : %s\n", TTF_GetError());
        TTF_CloseFont(font);
        return NULL;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    TTF_CloseFont(font);
    return textTexture;
}

void drawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
            }
        }
    }
}

bool checkCollision(SDL_Rect a, Ball ball) {
    int circleCenterX = ball.x;
    int circleCenterY = ball.y;
    int closestX = (a.x < circleCenterX) ? a.x + a.w : a.x;
    int closestY = (a.y < circleCenterY) ? a.y + a.h : a.y;
    int distanceX = circleCenterX - closestX;
    int distanceY = circleCenterY - closestY;
    return (distanceX * distanceX + distanceY * distanceY) <= (BALL_RADIUS * BALL_RADIUS);
}

void initBalls() {
    srand(time(NULL));
    for (int i = 0; i < currentBallCount; i++) {
        // Choisir aléatoirement un des 4 côtés (0: haut, 1: droite, 2: bas, 3: gauche)
        int side = rand() % 4;
        
        switch(side) {
            case 0: // Haut
                balls[i].x = rand() % (SCREEN_WIDTH - BALL_RADIUS * 2) + BALL_RADIUS;
                balls[i].y = BALL_RADIUS;
                balls[i].dx = (rand() % 2 == 0) ? currentBallSpeed : -currentBallSpeed;
                balls[i].dy = currentBallSpeed;
                break;
            case 1: // Droite
                balls[i].x = SCREEN_WIDTH - BALL_RADIUS;
                balls[i].y = rand() % (SCREEN_HEIGHT - BALL_RADIUS * 2) + BALL_RADIUS;
                balls[i].dx = -currentBallSpeed;
                balls[i].dy = (rand() % 2 == 0) ? currentBallSpeed : -currentBallSpeed;
                break;
            case 2: // Bas
                balls[i].x = rand() % (SCREEN_WIDTH - BALL_RADIUS * 2) + BALL_RADIUS;
                balls[i].y = SCREEN_HEIGHT - BALL_RADIUS;
                balls[i].dx = (rand() % 2 == 0) ? currentBallSpeed : -currentBallSpeed;
                balls[i].dy = -currentBallSpeed;
                break;
            case 3: // Gauche
                balls[i].x = BALL_RADIUS;
                balls[i].y = rand() % (SCREEN_HEIGHT - BALL_RADIUS * 2) + BALL_RADIUS;
                balls[i].dx = currentBallSpeed;
                balls[i].dy = (rand() % 2 == 0) ? currentBallSpeed : -currentBallSpeed;
                break;
        }
    }
}

void moveBalls() {
    for (int i = 0; i < currentBallCount; i++) {
        balls[i].x += balls[i].dx;
        balls[i].y += balls[i].dy;
        if (balls[i].x - BALL_RADIUS <= 0 || balls[i].x + BALL_RADIUS >= SCREEN_WIDTH)
            balls[i].dx = -balls[i].dx;
        if (balls[i].y - BALL_RADIUS <= 0 || balls[i].y + BALL_RADIUS >= SCREEN_HEIGHT)
            balls[i].dy = -balls[i].dy;
    }
}

void drawRoundedRect(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    drawCircle(renderer, rect.x + radius, rect.y + radius, radius);
    drawCircle(renderer, rect.x + rect.w - radius, rect.y + radius, radius);
    drawCircle(renderer, rect.x + radius, rect.y + rect.h - radius, radius);
    drawCircle(renderer, rect.x + rect.w - radius, rect.y + rect.h - radius, radius);
    SDL_Rect top = {rect.x + radius, rect.y, rect.w - 2 * radius, radius};
    SDL_Rect bottom = {rect.x + radius, rect.y + rect.h - radius, rect.w - 2 * radius, radius};
    SDL_Rect left = {rect.x, rect.y + radius, radius, rect.h - 2 * radius};
    SDL_Rect right = {rect.x + rect.w - radius, rect.y + radius, radius, rect.h - 2 * radius};
    SDL_RenderFillRect(renderer, &top);
    SDL_RenderFillRect(renderer, &bottom);
    SDL_RenderFillRect(renderer, &left);
    SDL_RenderFillRect(renderer, &right);
    SDL_Rect center = {rect.x + radius, rect.y + radius, rect.w - 2 * radius, rect.h - 2 * radius};
    SDL_RenderFillRect(renderer, &center);
}

void displayTime(Uint32 startTime) {
    Uint32 elapsed = (SDL_GetTicks() - startTime) / 1000;
    char timeText[20];
    sprintf(timeText, "Temps: %u s", elapsed);
    SDL_Color textColor = {255, 255, 255}; // Blanc
    SDL_Texture* timeTexture = createTextTexture(timeText, textColor);
    SDL_Rect timeRect = {10, 10, 100, 50}; // Position en haut à gauche
    SDL_RenderCopy(renderer, timeTexture, NULL, &timeRect);
    SDL_DestroyTexture(timeTexture);
}

void displayGameOver(Uint32 startTime) {
    bool running = true;
    SDL_Event event;
    Uint32 elapsed = (SDL_GetTicks() - startTime) / 1000;
    char timeText[50];
    sprintf(timeText, "Perdu! Temps: %u s", elapsed);
    SDL_Color textColor = {255, 255, 255}; // Blanc
    SDL_Texture* gameOverText = createTextTexture(timeText, textColor);
    SDL_Rect gameOverRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, 200, 50};
    SDL_Rect buttonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 10, 200, 100};
    SDL_Texture* buttonText = createTextTexture("Retour au menu", textColor);
    SDL_Color buttonColor = {0, 0, 255}; // Bouton en bleu
    SDL_Color hoverColor = {0, 255, 0}; // Couleur de survol

    // Zone de saisie du nom
    char playerName[50] = "";
    int nameLength = 0;
    SDL_Rect nameInputRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 100, 200, 50};
    SDL_Texture* nameInputText = createTextTexture("Entrez votre nom:", textColor);
    SDL_Rect nameInputLabelRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 150, 200, 50};
    SDL_Texture* currentNameText = NULL;
    SDL_Rect currentNameRect = nameInputRect;

    // Déterminer la difficulté actuelle
    const char* difficulteActuelle;
    if (currentBallCount == EASY_BALLS) {
        difficulteActuelle = "Facile";
    } else if (currentBallCount == MEDIUM_BALLS) {
        difficulteActuelle = "Intermediaire";
    } else {
        difficulteActuelle = "Difficile";
    }

    // Initialiser la texture du nom
    if (strlen(playerName) > 0) {
        currentNameText = createTextTexture(playerName, textColor);
    } else {
        currentNameText = createTextTexture(" ", textColor); // Texture vide mais valide
    }

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                if (x > buttonRect.x && x < buttonRect.x + buttonRect.w &&
                    y > buttonRect.y && y < buttonRect.y + buttonRect.h) {
                    running = false;
                }
            }
            else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_BACKSPACE && nameLength > 0) {
                    playerName[--nameLength] = '\0';
                }
                else if (event.key.keysym.sym >= SDLK_a && event.key.keysym.sym <= SDLK_z && nameLength < 49) {
                    playerName[nameLength++] = event.key.keysym.sym;
                    playerName[nameLength] = '\0';
                }
                else if (event.key.keysym.sym == SDLK_RETURN && nameLength > 0) {
                    MYSQL *con = mysql_init(NULL);
                    if (con == NULL) {
                        fprintf(stderr, "mysql_init() failed\n");
                        exit(1);
                    }
                    if (mysql_real_connect(con, "localhost", "root", "", "game_db", 0, NULL, 0) == NULL) {
                        finish_with_error(con);
                    }
                    insertScore(con, playerName, elapsed, difficulteActuelle);
                    mysql_close(con);
                    running = false;
                }
                
                // Mettre à jour la texture du nom
                if (currentNameText) {
                    SDL_DestroyTexture(currentNameText);
                }
                if (strlen(playerName) > 0) {
                    currentNameText = createTextTexture(playerName, textColor);
                } else {
                    currentNameText = createTextTexture(" ", textColor);
                }
            }
        }

        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, NULL);
        
        // Afficher le texte "Entrez votre nom"
        SDL_RenderCopy(renderer, nameInputText, NULL, &nameInputLabelRect);
        
        // Afficher la zone de saisie
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &nameInputRect);
        if (currentNameText) {
            SDL_RenderCopy(renderer, currentNameText, NULL, &currentNameRect);
        }
        
        // Afficher le temps
        SDL_RenderCopy(renderer, gameOverText, NULL, &gameOverRect);
        
        // Afficher le bouton retour
        int x, y;
        SDL_GetMouseState(&x, &y);
        if (x > buttonRect.x && x < buttonRect.x + buttonRect.w &&
            y > buttonRect.y && y < buttonRect.y + buttonRect.h) {
            drawRoundedRect(renderer, buttonRect, 20, hoverColor);
        } else {
            drawRoundedRect(renderer, buttonRect, 20, buttonColor);
        }
        SDL_Rect textRect = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 35, 100, 50};
        SDL_RenderCopy(renderer, buttonText, NULL, &textRect);
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Nettoyer les textures
    if (gameOverText) SDL_DestroyTexture(gameOverText);
    if (buttonText) SDL_DestroyTexture(buttonText);
    if (nameInputText) SDL_DestroyTexture(nameInputText);
    if (currentNameText) SDL_DestroyTexture(currentNameText);
}

void displayMenu() {
    bool running = true;
    SDL_Event event;
    SDL_Rect playButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 120, 200, 100};
    SDL_Rect scoreButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, 200, 100};
    SDL_Rect quitButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 120, 200, 100};
    SDL_Color textColor = {255, 255, 255}; // Texte en blanc
    SDL_Color buttonColor = {0, 0, 255}; // Bouton en bleu
    SDL_Color hoverColor = {0, 255, 0}; // Couleur de survol
    SDL_Texture* playButtonText = createTextTexture("Jouer", textColor);
    SDL_Texture* scoreButtonText = createTextTexture("Classement", textColor);
    SDL_Texture* quitButtonText = createTextTexture("Quitter", textColor);
    menuBackgroundTexture = loadTexture("menu_background.png");

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                if (x > playButtonRect.x && x < playButtonRect.x + playButtonRect.w &&
                    y > playButtonRect.y && y < playButtonRect.y + playButtonRect.h) {
                    running = false;
                    selectCharacter();
                } else if (x > scoreButtonRect.x && x < scoreButtonRect.x + scoreButtonRect.w &&
                           y > scoreButtonRect.y && y < scoreButtonRect.y + scoreButtonRect.h) {
                    // Afficher le classement
                    printf("Bouton 'Classement' cliqué\n");
                    SDL_RenderClear(renderer);
                    SDL_RenderCopy(renderer, menuBackgroundTexture, NULL, NULL);
                    displayScores();
                    SDL_RenderPresent(renderer);
                } else if (x > quitButtonRect.x && x < quitButtonRect.x + quitButtonRect.w &&
                           y > quitButtonRect.y && y < quitButtonRect.y + quitButtonRect.h) {
                    running = false; // Quitter le jeu
                }
            }
        }
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, menuBackgroundTexture, NULL, NULL);
        int x, y;
        SDL_GetMouseState(&x, &y);

        // Dessiner le bouton "Jouer"
        if (x > playButtonRect.x && x < playButtonRect.x + playButtonRect.w &&
            y > playButtonRect.y && y < playButtonRect.y + playButtonRect.h) {
            drawRoundedRect(renderer, playButtonRect, 20, hoverColor);
        } else {
            drawRoundedRect(renderer, playButtonRect, 20, buttonColor);
        }

        // Dessiner le bouton "Classement"
        if (x > scoreButtonRect.x && x < scoreButtonRect.x + scoreButtonRect.w &&
            y > scoreButtonRect.y && y < scoreButtonRect.y + scoreButtonRect.h) {
            drawRoundedRect(renderer, scoreButtonRect, 20, hoverColor);
        } else {
            drawRoundedRect(renderer, scoreButtonRect, 20, buttonColor);
        }

        // Dessiner le bouton "Quitter"
        if (x > quitButtonRect.x && x < quitButtonRect.x + quitButtonRect.w &&
            y > quitButtonRect.y && y < quitButtonRect.y + quitButtonRect.h) {
            drawRoundedRect(renderer, quitButtonRect, 20, hoverColor);
        } else {
            drawRoundedRect(renderer, quitButtonRect, 20, buttonColor);
        }

        // Afficher les textes des boutons
        SDL_Rect playTextRect = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 - 95, 100, 50};
        SDL_RenderCopy(renderer, playButtonText, NULL, &playTextRect);
        SDL_Rect scoreTextRect = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 25, 100, 50};
        SDL_RenderCopy(renderer, scoreButtonText, NULL, &scoreTextRect);
        SDL_Rect quitTextRect = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 145, 100, 50};
        SDL_RenderCopy(renderer, quitButtonText, NULL, &quitTextRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    SDL_DestroyTexture(playButtonText);
    SDL_DestroyTexture(scoreButtonText);
    SDL_DestroyTexture(quitButtonText);
    SDL_DestroyTexture(menuBackgroundTexture);
}

void displayScores() {
    MYSQL *con = mysql_init(NULL);
    if (con == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }
    if (mysql_real_connect(con, "localhost", "root", "", "game_db", 0, NULL, 0) == NULL) {
        finish_with_error(con);
    }

    SDL_Color textColor = {255, 255, 255};
    SDL_Color buttonColor = {0, 0, 255};
    SDL_Color hoverColor = {0, 255, 0};
    SDL_Color selectedColor = {255, 165, 0}; // Orange pour le niveau sélectionné

    SDL_Rect titleRect = {SCREEN_WIDTH / 2 - 100, 20, 200, 50};
    SDL_Rect difficultyButtonsRect[3] = {
        {SCREEN_WIDTH / 2 - 300, 80, 180, 40}, // Facile
        {SCREEN_WIDTH / 2 - 90, 80, 180, 40},  // Intermédiaire
        {SCREEN_WIDTH / 2 + 120, 80, 180, 40}  // Difficile
    };
    SDL_Rect scoreRect = {SCREEN_WIDTH / 2 - 100, 140, 200, 50};
    SDL_Rect backButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 100, 200, 50};

    SDL_Texture* titleText = createTextTexture("Classement", textColor);
    SDL_Texture* difficultyTexts[3];
    difficultyTexts[0] = createTextTexture("Facile", textColor);
    difficultyTexts[1] = createTextTexture("Intermediaire", textColor);
    difficultyTexts[2] = createTextTexture("Difficile", textColor);
    SDL_Texture* backButtonText = createTextTexture("Retour", textColor);

    bool running = true;
    SDL_Event event;
    int selectedDifficulty = 0; // 0: Facile, 1: Intermédiaire, 2: Difficile
    const char* difficulties[] = {"Facile", "Intermediaire", "Difficile"};

    // Charger les scores initiaux
    getTopScores(con, difficulties[selectedDifficulty]);

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                
                // Vérifier les clics sur les boutons de difficulté
                for (int i = 0; i < 3; i++) {
                    if (x > difficultyButtonsRect[i].x && x < difficultyButtonsRect[i].x + difficultyButtonsRect[i].w &&
                        y > difficultyButtonsRect[i].y && y < difficultyButtonsRect[i].y + difficultyButtonsRect[i].h) {
                        selectedDifficulty = i;
                        getTopScores(con, difficulties[selectedDifficulty]);
                        break;
                    }
                }

                // Vérifier le clic sur le bouton Retour
                if (x > backButtonRect.x && x < backButtonRect.x + backButtonRect.w &&
                    y > backButtonRect.y && y < backButtonRect.y + backButtonRect.h) {
                    running = false;
                }
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, menuBackgroundTexture, NULL, NULL);

        // Dessiner le tableau de classement
        SDL_Rect tableRect = {SCREEN_WIDTH / 2 - 150, 130, 300, 450};
        SDL_Color tableColor = {0, 0, 0, 128};
        drawRoundedRect(renderer, tableRect, 20, tableColor);

        // Afficher le titre
        SDL_RenderCopy(renderer, titleText, NULL, &titleRect);

        // Afficher les boutons de difficulté
        for (int i = 0; i < 3; i++) {
            if (i == selectedDifficulty) {
                drawRoundedRect(renderer, difficultyButtonsRect[i], 10, selectedColor);
            } else {
                drawRoundedRect(renderer, difficultyButtonsRect[i], 10, buttonColor);
            }
            SDL_RenderCopy(renderer, difficultyTexts[i], NULL, &difficultyButtonsRect[i]);
        }

        // Réinitialiser la position des scores
        scoreRect.y = 140;

        // Afficher les scores
        for (int i = 0; i < 10; i++) {
            if (topScores[i].id != 0) {
                char scoreText[50];
                sprintf(scoreText, "%d. %s - %d secondes", i + 1, topScores[i].nom, topScores[i].time);
                SDL_Texture* scoreTexture = createTextTexture(scoreText, textColor);
                SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
                scoreRect.y += 40;
                SDL_DestroyTexture(scoreTexture);
            }
        }

        // Dessiner le bouton "Retour"
        int x, y;
        SDL_GetMouseState(&x, &y);
        if (x > backButtonRect.x && x < backButtonRect.x + backButtonRect.w &&
            y > backButtonRect.y && y < backButtonRect.y + backButtonRect.h) {
            drawRoundedRect(renderer, backButtonRect, 20, hoverColor);
        } else {
            drawRoundedRect(renderer, backButtonRect, 20, buttonColor);
        }
        SDL_Rect backTextRect = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT - 90, 100, 30};
        SDL_RenderCopy(renderer, backButtonText, NULL, &backTextRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Nettoyer les ressources
    SDL_DestroyTexture(titleText);
    SDL_DestroyTexture(backButtonText);
    for (int i = 0; i < 3; i++) {
        SDL_DestroyTexture(difficultyTexts[i]);
    }
    mysql_close(con);
}

void insertScore(MYSQL *con, const char* nom, int time, const char* difficulte) {
    char query[200];
    sprintf(query, "INSERT INTO Scores (Nom, Time, Difficulte) VALUES ('%s', %d, '%s')", nom, time, difficulte);
    if (mysql_query(con, query)) {
        finish_with_error(con);
    }
    printf("Score inséré : %s - %d s - %s\n", nom, time, difficulte);
}

void displayPauseMenu() {
    bool running = true;
    SDL_Event event;
    SDL_Color textColor = {255, 255, 255};
    SDL_Color buttonColor = {0, 0, 255};
    SDL_Color hoverColor = {0, 255, 0};

    // Rectangle pour le titre
    SDL_Rect titleRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 100, 200, 50};
    SDL_Texture* titleText = createTextTexture("PAUSE", textColor);

    // Rectangles pour les boutons
    SDL_Rect resumeButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, 200, 50};
    SDL_Rect quitButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 60, 200, 50};

    // Textures pour les textes des boutons
    SDL_Texture* resumeText = createTextTexture("Reprendre", textColor);
    SDL_Texture* quitText = createTextTexture("Quitter", textColor);

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                exit(0);
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                    return;
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);

                if (x > resumeButtonRect.x && x < resumeButtonRect.x + resumeButtonRect.w &&
                    y > resumeButtonRect.y && y < resumeButtonRect.y + resumeButtonRect.h) {
                    Mix_PlayChannel(-1, buttonSound, 0);
                    running = false;
                    return;
                }
                else if (x > quitButtonRect.x && x < quitButtonRect.x + quitButtonRect.w &&
                         y > quitButtonRect.y && y < quitButtonRect.y + quitButtonRect.h) {
                    Mix_PlayChannel(-1, buttonSound, 0);
                    running = false;
                    exit(0);
                }
            }
        }

        // Rendre l'écran semi-transparent
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
        SDL_RenderClear(renderer);

        // Afficher le titre
        SDL_RenderCopy(renderer, titleText, NULL, &titleRect);

        // Afficher les boutons
        int x, y;
        SDL_GetMouseState(&x, &y);

        // Bouton Reprendre
        if (x > resumeButtonRect.x && x < resumeButtonRect.x + resumeButtonRect.w &&
            y > resumeButtonRect.y && y < resumeButtonRect.y + resumeButtonRect.h) {
            drawRoundedRect(renderer, resumeButtonRect, 20, hoverColor);
        } else {
            drawRoundedRect(renderer, resumeButtonRect, 20, buttonColor);
        }
        SDL_Rect resumeTextRect = {resumeButtonRect.x + 50, resumeButtonRect.y + 10, 100, 30};
        SDL_RenderCopy(renderer, resumeText, NULL, &resumeTextRect);

        // Bouton Quitter
        if (x > quitButtonRect.x && x < quitButtonRect.x + quitButtonRect.w &&
            y > quitButtonRect.y && y < quitButtonRect.y + quitButtonRect.h) {
            drawRoundedRect(renderer, quitButtonRect, 20, hoverColor);
        } else {
            drawRoundedRect(renderer, quitButtonRect, 20, buttonColor);
        }
        SDL_Rect quitTextRect = {quitButtonRect.x + 50, quitButtonRect.y + 10, 100, 30};
        SDL_RenderCopy(renderer, quitText, NULL, &quitTextRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Nettoyer les ressources
    SDL_DestroyTexture(titleText);
    SDL_DestroyTexture(resumeText);
    SDL_DestroyTexture(quitText);
}

void startGame() {
    player.w = 50;
    player.h = 50;
    player.x = SCREEN_WIDTH / 2 - player.w / 2;
    player.y = SCREEN_HEIGHT / 2 - player.h / 2;
    initBalls();
    startTime = SDL_GetTicks();
    bool running = true;
    SDL_Event event;

    // Jouer la musique de fond
    Mix_PlayMusic(backgroundMusic, -1); // -1 pour jouer en boucle
    
    while (running) {
        // Gestion des événements
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    displayPauseMenu();
                }
            }
        }

        // Gestion des mouvements du joueur
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_UP] && player.y > 0) player.y -= PLAYER_SPEED;
        if (keys[SDL_SCANCODE_DOWN] && player.y + player.h < SCREEN_HEIGHT) player.y += PLAYER_SPEED;
        if (keys[SDL_SCANCODE_LEFT] && player.x > 0) player.x -= PLAYER_SPEED;
        if (keys[SDL_SCANCODE_RIGHT] && player.x + player.w < SCREEN_WIDTH) player.x += PLAYER_SPEED;

        // Mouvement des balles
        if (balls != NULL) {
            moveBalls();
        }

        // Vérification des collisions
        bool collision = false;
        if (balls != NULL) {
            for (int i = 0; i < currentBallCount; i++) {
                if (checkCollision(player, balls[i])) {
                    collision = true;
                    break;
                }
            }
        }

        if (collision) {
            Mix_PlayChannel(-1, collisionSound, 0); // Jouer le son de collision
            displayGameOver(startTime);
            if (balls != NULL) {
                free(balls);
                balls = NULL;
            }
            displayMenu();
            return;
        }

        // Rendu
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
        SDL_RenderCopy(renderer, playerTexture, NULL, &player);

        // Dessiner les balles
        if (balls != NULL) {
            for (int i = 0; i < currentBallCount; i++) {
                SDL_Rect ballRect = {
                    balls[i].x - BALL_RADIUS,
                    balls[i].y - BALL_RADIUS,
                    BALL_RADIUS * 2,
                    BALL_RADIUS * 2
                };
                SDL_RenderCopy(renderer, ballTexture, NULL, &ballRect);
            }
        }

        displayTime(startTime);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Nettoyage
    if (balls != NULL) {
        free(balls);
        balls = NULL;
    }

    // Arrêter la musique
    Mix_HaltMusic();
}

void displayTutorial() {
    bool running = true;
    SDL_Event event;
    SDL_Color textColor = {255, 255, 255};
    SDL_Color buttonColor = {0, 0, 255};
    SDL_Color hoverColor = {0, 255, 0};

    // Rectangle pour le titre
    SDL_Rect titleRect = {SCREEN_WIDTH / 2 - 200, 20, 400, 50};
    SDL_Texture* titleText = createTextTexture("Tutoriel", textColor);

    // Textures pour les instructions
    SDL_Texture* instruction1 = createTextTexture("Utilisez les fleches directionnelles pour vous deplacer", textColor);
    SDL_Texture* instruction2 = createTextTexture("Evitez les balles qui rebondissent", textColor);
    SDL_Texture* instruction3 = createTextTexture("Appuyez sur ECHAP pour mettre le jeu en pause", textColor);
    SDL_Texture* instruction4 = createTextTexture("Survivez le plus longtemps possible !", textColor);

    // Rectangles pour les instructions
    SDL_Rect instruction1Rect = {SCREEN_WIDTH / 2 - 300, 100, 600, 40};
    SDL_Rect instruction2Rect = {SCREEN_WIDTH / 2 - 300, 160, 600, 40};
    SDL_Rect instruction3Rect = {SCREEN_WIDTH / 2 - 300, 220, 600, 40};
    SDL_Rect instruction4Rect = {SCREEN_WIDTH / 2 - 300, 280, 600, 40};

    // Rectangle pour le bouton Commencer
    SDL_Rect startButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 100, 200, 50};
    SDL_Texture* startText = createTextTexture("Commencer", textColor);

    // Animation des flèches
    SDL_Texture* arrowKeys = loadTexture("assets/arrow_keys.png");
    SDL_Rect arrowRect = {SCREEN_WIDTH / 2 - 100, 350, 200, 200};
    float arrowScale = 1.0f;
    bool increasing = true;

    Uint32 lastTime = SDL_GetTicks();
    const float ANIMATION_SPEED = 0.5f;

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Animation des flèches
        if (increasing) {
            arrowScale += ANIMATION_SPEED * deltaTime;
            if (arrowScale >= 1.2f) increasing = false;
        } else {
            arrowScale -= ANIMATION_SPEED * deltaTime;
            if (arrowScale <= 1.0f) increasing = true;
        }

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                exit(0);
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);

                if (x >= startButtonRect.x && x < startButtonRect.x + startButtonRect.w &&
                    y >= startButtonRect.y && y < startButtonRect.y + startButtonRect.h) {
                    Mix_PlayChannel(-1, buttonSound, 0);
                    running = false;
                }
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, menuBackgroundTexture, NULL, NULL);

        // Afficher le titre
        SDL_RenderCopy(renderer, titleText, NULL, &titleRect);

        // Afficher les instructions
        SDL_RenderCopy(renderer, instruction1, NULL, &instruction1Rect);
        SDL_RenderCopy(renderer, instruction2, NULL, &instruction2Rect);
        SDL_RenderCopy(renderer, instruction3, NULL, &instruction3Rect);
        SDL_RenderCopy(renderer, instruction4, NULL, &instruction4Rect);

        // Afficher l'animation des flèches
        if (arrowKeys) {
            SDL_Rect scaledArrowRect = {
                arrowRect.x - (arrowRect.w * (arrowScale - 1.0f)) / 2,
                arrowRect.y - (arrowRect.h * (arrowScale - 1.0f)) / 2,
                arrowRect.w * arrowScale,
                arrowRect.h * arrowScale
            };
            SDL_RenderCopy(renderer, arrowKeys, NULL, &scaledArrowRect);
        }

        // Afficher le bouton Commencer
        int x, y;
        SDL_GetMouseState(&x, &y);
        if (x >= startButtonRect.x && x < startButtonRect.x + startButtonRect.w &&
            y >= startButtonRect.y && y < startButtonRect.y + startButtonRect.h) {
            drawRoundedRect(renderer, startButtonRect, 20, hoverColor);
        } else {
            drawRoundedRect(renderer, startButtonRect, 20, buttonColor);
        }
        SDL_Rect startTextRect = {startButtonRect.x + 50, startButtonRect.y + 10, 100, 30};
        SDL_RenderCopy(renderer, startText, NULL, &startTextRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Nettoyer les ressources
    SDL_DestroyTexture(titleText);
    SDL_DestroyTexture(instruction1);
    SDL_DestroyTexture(instruction2);
    SDL_DestroyTexture(instruction3);
    SDL_DestroyTexture(instruction4);
    SDL_DestroyTexture(startText);
    if (arrowKeys) SDL_DestroyTexture(arrowKeys);
}

void selectDifficulty() {
    bool running = true;
    SDL_Event event;
    SDL_Color textColor = {255, 255, 255};
    SDL_Color buttonColor = {0, 0, 255};
    SDL_Color hoverColor = {0, 255, 0};

    // Rectangle pour le titre
    SDL_Rect titleRect = {SCREEN_WIDTH / 2 - 150, 50, 300, 50};

    // Rectangles pour les boutons
    SDL_Rect easyButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 120, 200, 80};
    SDL_Rect mediumButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, 200, 80};
    SDL_Rect hardButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 120, 200, 80};

    // Textures pour les textes
    SDL_Texture* titleText = createTextTexture("Selectionnez la difficulte", textColor);
    SDL_Texture* easyText = createTextTexture("Facile", textColor);
    SDL_Texture* mediumText = createTextTexture("Intermediaire", textColor);
    SDL_Texture* hardText = createTextTexture("Difficile", textColor);

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                
                if (x > easyButtonRect.x && x < easyButtonRect.x + easyButtonRect.w &&
                    y > easyButtonRect.y && y < easyButtonRect.y + easyButtonRect.h) {
                    currentBallCount = EASY_BALLS;
                    currentBallSpeed = EASY_SPEED;
                    Mix_PlayChannel(-1, buttonSound, 0);
                    displayTutorial();
                    if (balls) free(balls);
                    balls = (Ball*)malloc(currentBallCount * sizeof(Ball));
                    if (!balls) {
                        printf("Erreur d'allocation memoire pour les balles\n");
                        exit(1);
                    }
                    SDL_DestroyTexture(titleText);
                    SDL_DestroyTexture(easyText);
                    SDL_DestroyTexture(mediumText);
                    SDL_DestroyTexture(hardText);
                    startGame();
                    return;
                }
                else if (x > mediumButtonRect.x && x < mediumButtonRect.x + mediumButtonRect.w &&
                         y > mediumButtonRect.y && y < mediumButtonRect.y + mediumButtonRect.h) {
                    currentBallCount = MEDIUM_BALLS;
                    currentBallSpeed = MEDIUM_SPEED;
                    Mix_PlayChannel(-1, buttonSound, 0);
                    displayTutorial();
                    if (balls) free(balls);
                    balls = (Ball*)malloc(currentBallCount * sizeof(Ball));
                    if (!balls) {
                        printf("Erreur d'allocation memoire pour les balles\n");
                        exit(1);
                    }
                    SDL_DestroyTexture(titleText);
                    SDL_DestroyTexture(easyText);
                    SDL_DestroyTexture(mediumText);
                    SDL_DestroyTexture(hardText);
                    startGame();
                    return;
                }
                else if (x > hardButtonRect.x && x < hardButtonRect.x + hardButtonRect.w &&
                         y > hardButtonRect.y && y < hardButtonRect.y + hardButtonRect.h) {
                    currentBallCount = HARD_BALLS;
                    currentBallSpeed = HARD_SPEED;
                    Mix_PlayChannel(-1, buttonSound, 0);
                    displayTutorial();
                    if (balls) free(balls);
                    balls = (Ball*)malloc(currentBallCount * sizeof(Ball));
                    if (!balls) {
                        printf("Erreur d'allocation memoire pour les balles\n");
                        exit(1);
                    }
                    SDL_DestroyTexture(titleText);
                    SDL_DestroyTexture(easyText);
                    SDL_DestroyTexture(mediumText);
                    SDL_DestroyTexture(hardText);
                    startGame();
                    return;
                }
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, menuBackgroundTexture, NULL, NULL);

        // Afficher le titre
        SDL_RenderCopy(renderer, titleText, NULL, &titleRect);

        // Obtenir la position de la souris
        int x, y;
        SDL_GetMouseState(&x, &y);

        // Dessiner les boutons avec effet de survol
        // Bouton Facile
        if (x > easyButtonRect.x && x < easyButtonRect.x + easyButtonRect.w &&
            y > easyButtonRect.y && y < easyButtonRect.y + easyButtonRect.h) {
            drawRoundedRect(renderer, easyButtonRect, 20, hoverColor);
        } else {
            drawRoundedRect(renderer, easyButtonRect, 20, buttonColor);
        }

        // Bouton Intermédiaire
        if (x > mediumButtonRect.x && x < mediumButtonRect.x + mediumButtonRect.w &&
            y > mediumButtonRect.y && y < mediumButtonRect.y + mediumButtonRect.h) {
            drawRoundedRect(renderer, mediumButtonRect, 20, hoverColor);
        } else {
            drawRoundedRect(renderer, mediumButtonRect, 20, buttonColor);
        }

        // Bouton Difficile
        if (x > hardButtonRect.x && x < hardButtonRect.x + hardButtonRect.w &&
            y > hardButtonRect.y && y < hardButtonRect.y + hardButtonRect.h) {
            drawRoundedRect(renderer, hardButtonRect, 20, hoverColor);
        } else {
            drawRoundedRect(renderer, hardButtonRect, 20, buttonColor);
        }

        // Afficher les textes des boutons
        SDL_Rect textRect = {easyButtonRect.x + 50, easyButtonRect.y + 25, 100, 30};
        SDL_RenderCopy(renderer, easyText, NULL, &textRect);
        
        textRect.y = mediumButtonRect.y + 25;
        SDL_RenderCopy(renderer, mediumText, NULL, &textRect);
        
        textRect.y = hardButtonRect.y + 25;
        SDL_RenderCopy(renderer, hardText, NULL, &textRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Nettoyer les textures
    SDL_DestroyTexture(titleText);
    SDL_DestroyTexture(easyText);
    SDL_DestroyTexture(mediumText);
    SDL_DestroyTexture(hardText);
}

void selectCharacter() {
    bool running = true;
    SDL_Event event;
    SDL_Color textColor = {255, 255, 255};
    SDL_Color buttonColor = {0, 0, 255};
    SDL_Color hoverColor = {0, 255, 0};
    SDL_Color selectedColor = {255, 165, 0};

    // Rectangle pour le titre
    SDL_Rect titleRect = {SCREEN_WIDTH / 2 - 200, 20, 400, 50};
    SDL_Texture* titleText = createTextTexture("Selectionnez votre personnage", textColor);

    // Charger toutes les images des personnages
    char* characterFiles[] = {
        "user/astra.png", "user/breach.png", "user/brimstone.png", "user/chamber.png",
        "user/clove.png", "user/cypher.png", "user/fade.png", "user/gekko.png",
        "user/harbor.png", "user/iso.png", "user/jett.png", "user/killjoy.png",
        "user/neon.png", "user/omen.png", "user/phoenix.png", "user/raze.png",
        "user/reyna.png", "user/sage.png", "user/skye.png", "user/sova.png",
        "user/viper.png", "user/vyse.png", "user/waylay.png"
    };
    int numCharacters = sizeof(characterFiles) / sizeof(characterFiles[0]);
    SDL_Texture** characterTextures = malloc(numCharacters * sizeof(SDL_Texture*));
    
    // Variables pour les animations
    float* characterScales = malloc(numCharacters * sizeof(float));
    float* characterPulses = malloc(numCharacters * sizeof(float));
    Uint32* hoverStartTimes = malloc(numCharacters * sizeof(Uint32));
    Uint32* selectStartTimes = malloc(numCharacters * sizeof(Uint32));
    bool* isHovered = malloc(numCharacters * sizeof(bool));
    
    // Initialiser les animations
    for (int i = 0; i < numCharacters; i++) {
        characterScales[i] = 1.0f;
        characterPulses[i] = 0.0f;
        hoverStartTimes[i] = 0;
        selectStartTimes[i] = 0;
        isHovered[i] = false;
    }
    
    // Charger toutes les textures
    for (int i = 0; i < numCharacters; i++) {
        characterTextures[i] = loadTexture(characterFiles[i]);
        if (!characterTextures[i]) {
            printf("Erreur lors du chargement de l'image: %s\n", characterFiles[i]);
        }
    }

    // Calculer la disposition de la grille
    int startX = (SCREEN_WIDTH - (CHARACTERS_PER_ROW * (CHARACTER_SIZE + CHARACTER_PADDING))) / 2;
    int startY = 100;
    SDL_Rect* characterRects = malloc(numCharacters * sizeof(SDL_Rect));
    
    for (int i = 0; i < numCharacters; i++) {
        int row = i / CHARACTERS_PER_ROW;
        int col = i % CHARACTERS_PER_ROW;
        characterRects[i].x = startX + col * (CHARACTER_SIZE + CHARACTER_PADDING);
        characterRects[i].y = startY + row * (CHARACTER_SIZE + CHARACTER_PADDING);
        characterRects[i].w = CHARACTER_SIZE;
        characterRects[i].h = CHARACTER_SIZE;
    }

    // Rectangle pour le bouton Continuer
    SDL_Rect continueButtonRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT - 80, 200, 60};
    SDL_Texture* continueText = createTextTexture("Continuer", textColor);
    bool characterSelected = false;
    int selectedIndex = -1;

    Uint32 lastTime = SDL_GetTicks();
    const float ANIMATION_SPEED = 0.5f;
    const float PULSE_SPEED = 0.02f;
    const float MAX_SCALE = 1.2f;
    const float MIN_SCALE = 1.0f;

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);

                // Vérifier les clics sur les personnages
                for (int i = 0; i < numCharacters; i++) {
                    if (x >= characterRects[i].x && x < characterRects[i].x + characterRects[i].w &&
                        y >= characterRects[i].y && y < characterRects[i].y + characterRects[i].h) {
                        selectedIndex = i;
                        characterSelected = true;
                        strcpy(selectedCharacter, characterFiles[i]);
                        selectStartTimes[i] = currentTime;
                        Mix_PlayChannel(-1, selectSound, 0); // Jouer le son de sélection
                        break;
                    }
                }

                // Vérifier le clic sur le bouton Continuer
                if (characterSelected && x >= continueButtonRect.x && x < continueButtonRect.x + continueButtonRect.w &&
                    y >= continueButtonRect.y && y < continueButtonRect.y + continueButtonRect.h) {
                    Mix_PlayChannel(-1, buttonSound, 0); // Jouer le son du bouton
                    running = false;
                    
                    // Nettoyer les ressources
                    SDL_DestroyTexture(titleText);
                    SDL_DestroyTexture(continueText);
                    for (int i = 0; i < numCharacters; i++) {
                        SDL_DestroyTexture(characterTextures[i]);
                    }
                    free(characterTextures);
                    free(characterRects);
                    free(characterScales);
                    free(characterPulses);
                    free(hoverStartTimes);
                    free(selectStartTimes);
                    free(isHovered);
                    
                    // Charger la nouvelle texture du joueur
                    if (playerTexture) {
                        SDL_DestroyTexture(playerTexture);
                    }
                    playerTexture = loadTexture(selectedCharacter);
                    selectDifficulty();
                    return;
                }
            }
        }

        // Mise à jour des animations
        int x, y;
        SDL_GetMouseState(&x, &y);

        for (int i = 0; i < numCharacters; i++) {
            bool isHovering = (x >= characterRects[i].x && x < characterRects[i].x + characterRects[i].w &&
                             y >= characterRects[i].y && y < characterRects[i].y + characterRects[i].h);

            // Animation de survol
            if (isHovering != isHovered[i]) {
                isHovered[i] = isHovering;
                hoverStartTimes[i] = currentTime;
            }

            // Animation de zoom au survol
            float targetScale = isHovering ? MAX_SCALE : MIN_SCALE;
            characterScales[i] += (targetScale - characterScales[i]) * ANIMATION_SPEED * deltaTime;

            // Animation de pulse pour le personnage sélectionné
            if (i == selectedIndex) {
                characterPulses[i] = sin((currentTime - selectStartTimes[i]) * PULSE_SPEED) * 0.1f + 1.0f;
            } else {
                characterPulses[i] = 1.0f;
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, menuBackgroundTexture, NULL, NULL);

        // Afficher le titre
        SDL_RenderCopy(renderer, titleText, NULL, &titleRect);

        // Afficher tous les personnages avec animations
        for (int i = 0; i < numCharacters; i++) {
            SDL_Rect destRect = characterRects[i];
            float finalScale = characterScales[i] * characterPulses[i];
            
            // Calculer la nouvelle taille et position pour l'effet de zoom
            int newWidth = destRect.w * finalScale;
            int newHeight = destRect.h * finalScale;
            int offsetX = (newWidth - destRect.w) / 2;
            int offsetY = (newHeight - destRect.h) / 2;
            
            SDL_Rect scaledRect = {
                destRect.x - offsetX,
                destRect.y - offsetY,
                newWidth,
                newHeight
            };

            // Afficher l'image du personnage avec un fond orange si sélectionné
            if (i == selectedIndex) {
                SDL_SetRenderDrawColor(renderer, selectedColor.r, selectedColor.g, selectedColor.b, 255);
                SDL_RenderFillRect(renderer, &scaledRect);
            }
            
            if (characterTextures[i]) {
                SDL_RenderCopy(renderer, characterTextures[i], NULL, &scaledRect);
            }
        }

        // Afficher le bouton Continuer
        if (characterSelected) {
            int x, y;
            SDL_GetMouseState(&x, &y);
            if (x >= continueButtonRect.x && x < continueButtonRect.x + continueButtonRect.w &&
                y >= continueButtonRect.y && y < continueButtonRect.y + continueButtonRect.h) {
                drawRoundedRect(renderer, continueButtonRect, 20, hoverColor);
            } else {
                drawRoundedRect(renderer, continueButtonRect, 20, buttonColor);
            }
            SDL_Rect continueTextRect = {continueButtonRect.x + 50, continueButtonRect.y + 15, 100, 30};
            SDL_RenderCopy(renderer, continueText, NULL, &continueTextRect);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Nettoyer les ressources
    SDL_DestroyTexture(titleText);
    SDL_DestroyTexture(continueText);
    for (int i = 0; i < numCharacters; i++) {
        SDL_DestroyTexture(characterTextures[i]);
    }
    free(characterTextures);
    free(characterRects);
    free(characterScales);
    free(characterPulses);
    free(hoverStartTimes);
    free(selectStartTimes);
    free(isHovered);
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 || 
        IMG_Init(IMG_INIT_PNG) == 0 || 
        TTF_Init() == -1 ||
        Mix_Init(MIX_INIT_MP3) == 0) {
        printf("Erreur d'initialisation: %s\n", SDL_GetError());
        return 1;
    }
    window = SDL_CreateWindow("Jeu SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!window || !renderer) {
        printf("Erreur de création de fenêtre/renderer : %s\n", SDL_GetError());
        return 1;
    }
    playerTexture = loadTexture("user/phoenix.png"); // Charger un personnage par défaut
    backgroundTexture = loadTexture("background.png");
    menuBackgroundTexture = loadTexture("menu_background.png");
    ballTexture = loadTexture("Smoke.png");
    if (!playerTexture || !backgroundTexture || !menuBackgroundTexture || !ballTexture) return 1;

    MYSQL *con = mysql_init(NULL);
    if (con == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return 1;
    }
    if (mysql_real_connect(con, "localhost", "root", "", NULL, 0, NULL, 0) == NULL) {
        finish_with_error(con);
    }
    createDatabase(con);
    mysql_close(con);

    initAudio();

    displayMenu();

    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyTexture(menuBackgroundTexture);
    SDL_DestroyTexture(ballTexture); // Nettoyer la texture des balles
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();

    cleanupAudio();
    return 0;
}