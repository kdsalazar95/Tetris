#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "TetrisStructures.h"
#include <Arduino.h>

// ============================================
// DEFINICIONES DE PIEZAS TETRIS (del original líneas 46-61)
// ============================================

const byte figures[7][4][4] = {
  // I (Cyan)
  {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}}, 
  // J (Blue)
  {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
  // L (Orange)
  {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
  // O (Yellow)
  {{1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},
  // S (Green)
  {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},
  // T (Purple)
  {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
  // Z (Red)
  {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}}
};

// ============================================
// FUNCIONES AUXILIARES (del original líneas 184-192)
// ============================================

// Obtener bloque con rotación aplicada
int getBlock(int type, int rot, int x, int y) {
  switch (rot) {
    case 0: return figures[type][y][x];
    case 1: return figures[type][3-x][y];  // Rotación 90°
    case 2: return figures[type][3-y][3-x]; // Rotación 180°
    case 3: return figures[type][x][3-y];  // Rotación 270°
  }
  return 0;
}

// ============================================
// DETECCIÓN DE COLISIONES (del original líneas 169-182)
// ============================================

bool checkCollision(GameState_t *state, int x, int y, int rot) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (getBlock(state->currentType, rot, j, i)) { 
        int fieldX = x + j;
        int fieldY = y + i;

        // Verificar límites horizontales y verticales
        if (fieldX < 0 || fieldX >= 10 || fieldY >= 20) {
          return true; // Colisión con bordes
        }
        
        // Verificar colisión con bloques existentes
        if (fieldY >= 0 && state->board[fieldY][fieldX] != 0) {
          return true;
        }
      }
    }
  }
  return false;
}

// ============================================
// MOVIMIENTO (del original líneas 152-159)
// ============================================

bool canMovePiece(GameState_t *state, int dx, int dy) {
  int newX = state->currentX + dx;
  int newY = state->currentY + dy;
  
  return !checkCollision(state, newX, newY, state->currentRot);
}

bool movePiece(GameState_t *state, int dx, int dy) {
  if (canMovePiece(state, dx, dy)) {
    state->currentX += dx;
    state->currentY += dy;
    return true;
  }
  return false;
}

// ============================================
// ROTACIÓN (del original líneas 161-167)
// ============================================

void rotatePiece(GameState_t *state) {
  int nextRot = (state->currentRot + 1) % 4;
  
  if (!checkCollision(state, state->currentX, state->currentY, nextRot)) {
    state->currentRot = nextRot;
    // Sonido de rotación (opcional)
    // tone(BUZZER_PIN, 800, 20);
  }
}

// ============================================
// FIJAR PIEZA AL TABLERO (del original líneas 194-207)
// ============================================

void lockPiece(GameState_t *state) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (getBlock(state->currentType, state->currentRot, j, i)) {
        int fieldX = state->currentX + j;
        int fieldY = state->currentY + i;
        
        if (fieldY >= 0) {
          // Guardar tipo de pieza + 1 (para colores)
          state->board[fieldY][fieldX] = state->currentType + 1;
        }
      }
    }
  }
  // Sonido de bloqueo (opcional)
  // tone(BUZZER_PIN, 200, 50);
}

// ============================================
// LIMPIAR LÍNEAS COMPLETAS (del original líneas 209-239)
// ============================================

void clearLines(GameState_t *state) {
  int linesCleared = 0;
  
  for (int y = 19; y >= 0; y--) {
    bool full = true;
    
    // Verificar si la línea está completa
    for (int x = 0; x < 10; x++) {
      if (state->board[y][x] == 0) {
        full = false;
        break;
      }
    }
    
    if (full) {
      // Bajar todas las líneas superiores
      for (int k = y; k > 0; k--) {
        for (int x = 0; x < 10; x++) {
          state->board[k][x] = state->board[k-1][x];
        }
      }
      
      // Limpiar línea superior
      for (int x = 0; x < 10; x++) {
        state->board[0][x] = 0;
      }
      
      y++; // Re-verificar esta línea
      linesCleared++;
    }
  }

  // Actualizar puntuación y velocidad
  if (linesCleared > 0) {
    state->score += linesCleared * 100;
    state->totalLines += linesCleared;
    
    // Aumentar velocidad cada 500 puntos
    state->dropInterval = max(100, 500 - (state->score / 500) * 50);
    
    // Actualizar nivel
    state->level = (state->totalLines / 10) + 1;
    
    // Sonido de líneas completas (opcional)
    // tone(BUZZER_PIN, 1500, 200);
  }
}

// ============================================
// GENERAR NUEVA PIEZA (del original líneas 241-247)
// ============================================

void spawnPiece(GameState_t *state) {
  state->currentType = state->nextType;
  state->nextType = random(0, 7); // Siguiente pieza aleatoria
  state->currentRot = 0;
  state->currentX = 10 / 2 - 2;   // Centro del tablero
  state->currentY = -1;            // Arriba del tablero visible
  
  // Verificar Game Over
  if (checkCollision(state, state->currentX, state->currentY, state->currentRot)) {
    state->gameOver = true;
  }
}

// ============================================
// HARD DROP (caída instantánea)
// ============================================

void dropPiece(GameState_t *state) {
  // Bajar hasta que colisione
  while (!checkCollision(state, state->currentX, state->currentY + 1, state->currentRot)) {
    state->currentY++;
  }
  
  // Fijar pieza inmediatamente
  lockPiece(state);
  clearLines(state);
  spawnPiece(state);
}

// ============================================
// INICIALIZAR JUEGO (del original líneas 249-261)
// ============================================

void initializeGame(GameState_t *state) {
  // Limpiar tablero
  for (int y = 0; y < 20; y++) {
    for (int x = 0; x < 10; x++) {
      state->board[y][x] = 0;
    }
  }
  
  // Reset variables
  state->score = 0;
  state->level = 1;
  state->totalLines = 0;
  state->dropInterval = 500;  // 500ms inicial
  state->gameOver = false;
  state->paused = false;
  
  // Generar primera pieza
  state->nextType = random(0, 7);
  spawnPiece(state);
}

#endif