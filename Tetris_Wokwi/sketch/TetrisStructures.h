#ifndef TETRIS_STRUCTURES_H
#define TETRIS_STRUCTURES_H

// Eventos de input
typedef enum {
    EVENT_MOVE_LEFT,
    EVENT_MOVE_RIGHT,
    EVENT_MOVE_DOWN,
    EVENT_ROTATE,
    EVENT_DROP,
    EVENT_PAUSE,
    EVENT_NONE
} InputEvent_t;

// Estado completo del juego
typedef struct {
    int board[20][10];        // Tablero 20x10
    int currentPiece[4][4];   // NO USADO (usamos figures[] en GameLogic)
    int currentX;             // Posición X actual
    int currentY;             // Posición Y actual
    int currentType;          // ← AGREGAR: Tipo de pieza actual (0-6)
    int currentRot;           // ← AGREGAR: Rotación actual (0-3)
    int nextType;             // ← AGREGAR: Siguiente pieza
    int score;                // Puntuación
    int level;                // Nivel
    int totalLines;           // Líneas totales limpiadas
    int dropInterval;         // ← AGREGAR: Velocidad de caída (ms)
    bool gameOver;            // Flag de game over
    bool paused;              // Flag de pausa
} GameState_t;

#endif