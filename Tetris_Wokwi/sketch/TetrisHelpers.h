#ifndef TETRIS_HELPERS_H
#define TETRIS_HELPERS_H

// Inicializar juego
void initializeGame(GameState_t *state) {
    memset(state->board, 0, sizeof(state->board));
    state->currentX = 5;
    state->currentY = 0;
    state->score = 0;
    state->level = 1;
    state->gameOver = false;
    state->paused = false;
    spawnNewPiece(state);
}

// Verificar si puede mover pieza
bool canMovePiece(GameState_t *state, int deltaX, int deltaY) {
    int newX = state->currentX + deltaX;
    int newY = state->currentY + deltaY;
    
    // Verificar límites y colisiones
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (state->currentPiece[i][j]) {
                int boardX = newX + j;
                int boardY = newY + i;
                
                // Fuera de límites
                if (boardX < 0 || boardX >= 10 || boardY >= 20) {
                    return false;
                }
                
                // Colisión con pieza existente
                if (boardY >= 0 && state->board[boardY][boardX]) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

// Rotar pieza
void rotatePiece(GameState_t *state) {
    int temp[4][4];
    
    // Rotación 90 grados horario
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp[i][j] = state->currentPiece[3-j][i];
        }
    }
    
    // Verificar si la rotación es válida
    // (guardar estado temporal y verificar)
    int originalPiece[4][4];
    memcpy(originalPiece, state->currentPiece, sizeof(originalPiece));
    memcpy(state->currentPiece, temp, sizeof(temp));
    
    if (!canMovePiece(state, 0, 0)) {
        // Revertir rotación
        memcpy(state->currentPiece, originalPiece, sizeof(originalPiece));
    }
}

// Hacer caer pieza hasta el fondo
void dropPiece(GameState_t *state) {
    while (canMovePiece(state, 0, 1)) {
        state->currentY++;
    }
    lockPiece(state);
}

// Fijar pieza al tablero
void lockPiece(GameState_t *state) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (state->currentPiece[i][j]) {
                int boardY = state->currentY + i;
                int boardX = state->currentX + j;
                if (boardY >= 0 && boardY < 20 && boardX >= 0 && boardX < 10) {
                    state->board[boardY][boardX] = state->currentPiece[i][j];
                }
            }
        }
    }
    
    clearLines(state);
    spawnNewPiece(state);
}

// Limpiar líneas completas
void clearLines(GameState_t *state) {
    int linesCleared = 0;
    
    for (int y = 19; y >= 0; y--) {
        bool fullLine = true;
        
        for (int x = 0; x < 10; x++) {
            if (state->board[y][x] == 0) {
                fullLine = false;
                break;
            }
        }
        
        if (fullLine) {
            linesCleared++;
            
            // Bajar todas las líneas superiores
            for (int yy = y; yy > 0; yy--) {
                for (int x = 0; x < 10; x++) {
                    state->board[yy][x] = state->board[yy-1][x];
                }
            }
            
            // Limpiar línea superior
            for (int x = 0; x < 10; x++) {
                state->board[0][x] = 0;
            }
            
            y++; // Re-check esta línea
        }
    }
    
    // Actualizar score
    if (linesCleared > 0) {
        int points[] = {0, 100, 300, 500, 800};
        state->score += points[linesCleared] * state->level;
    }
}

// Generar nueva pieza
void spawnNewPiece(GameState_t *state) {
    // Piezas Tetris (I, O, T, S, Z, J, L)
    // Aquí deberías usar el código del repo original
    // o implementar tu propia lógica de generación
    
    state->currentX = 3;
    state->currentY = 0;
    
    // Verificar game over
    if (!canMovePiece(state, 0, 0)) {
        state->gameOver = true;
    }
}

#endif