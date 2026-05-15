#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <TFT_eSPI.h>
#include "TetrisStructures.h"

// ============================================
// CONFIGURACIÓN DEL DISPLAY (del original líneas 1-44)
// ============================================

TFT_eSPI tft = TFT_eSPI(); 
TFT_eSprite spr = TFT_eSprite(&tft); 

// --- DIMENSIONES (del original líneas 14-30) ---
#define SCREEN_W      240
#define SCREEN_H      320
#define BLOCK_SIZE    14    // Tamaño de cada bloque en píxeles
#define FIELD_W       10    // Ancho del tablero (bloques)
#define FIELD_H       20    // Alto del tablero (bloques)

// Posición del tablero (izquierda)
#define OFFSET_X      10
#define OFFSET_Y      20

// Panel de UI (derecha)
#define UI_X          (OFFSET_X + (FIELD_W * BLOCK_SIZE) + 10)
#define UI_W          (SCREEN_W - UI_X - 5)
#define NEXT_X        (UI_X + 15)  // Posición de "Next Piece"
#define NEXT_Y        60

// --- COLORES (del original líneas 32-44) ---
#define C_BLACK       0x0000
#define C_WHITE       0xFFFF
#define C_GRAY        0x528A
#define C_DARKGRAY    0x2124  // Fondo del panel UI
#define C_BORDER      0xAD55
#define C_CYAN        0x07FF  // I
#define C_BLUE        0x001F  // J
#define C_ORANGE      0xFD20  // L
#define C_YELLOW      0xFFE0  // O
#define C_GREEN       0x07E0  // S
#define C_PURPLE      0xF81F  // T
#define C_RED         0xF800  // Z

// === 1. DECLARACIONES GLOBALES ===
extern TFT_eSPI tft;
extern TFT_eSprite spr;

// === 2. PROTOTIPOS (Vital para evitar errores de Scope) ===
void initDisplay();
void drawBlock(int x, int y, uint16_t color);
void drawGame(GameState_t *state);
void drawGameOver(GameState_t *state);
const uint16_t colors[8] = {
  C_BLACK, C_CYAN, C_BLUE, C_ORANGE, 
  C_YELLOW, C_GREEN, C_PURPLE, C_RED
};

// ============================================
// INICIALIZACIÓN (del original líneas 89-96)
// ============================================

void initDisplay() {
  tft.init();
  tft.setRotation(2);  // Portrait mode
  
  // Crear sprite para double buffering
  spr.setColorDepth(8);
  spr.createSprite(SCREEN_W, SCREEN_H);
  
  Serial.println("Display inicializado");
}

// ============================================
// DIBUJAR BLOQUE (del original líneas 321-325)
// ============================================

void drawBlock(int x, int y, uint16_t color) {
  // Bloque con borde 3D
  spr.fillRect(x, y, BLOCK_SIZE - 1, BLOCK_SIZE - 1, color); 
  spr.drawRect(x, y, BLOCK_SIZE - 1, BLOCK_SIZE - 1, C_WHITE); 
  spr.drawRect(x + 2, y + 2, BLOCK_SIZE - 5, BLOCK_SIZE - 5, color);
}

// ============================================
// DIBUJAR JUEGO COMPLETO (del original líneas 263-319)
// ============================================

void drawGame(GameState_t *state) {
  // Limpiar pantalla
  spr.fillSprite(C_BLACK); 
  
  // ========== PANEL UI (DERECHA) ==========
  spr.fillRect(UI_X, 0, UI_W, SCREEN_H, C_DARKGRAY);
  spr.drawRect(UI_X, 0, UI_W, SCREEN_H, C_GRAY);
  
  // Texto "NEXT"
  spr.setTextColor(C_WHITE, C_DARKGRAY);
  spr.setTextDatum(MC_DATUM);
  spr.drawString("NEXT", UI_X + UI_W/2, 35, 2);
  
  // Recuadro para siguiente pieza
  spr.drawRect(UI_X + 10, 50, UI_W - 20, 70, C_WHITE);
  
  // Dibujar siguiente pieza
  extern const byte figures[7][4][4];  // De GameLogic.h
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) { 
      if (figures[state->nextType][i][j]) { 
        drawBlock(NEXT_X + j * BLOCK_SIZE, 
                  NEXT_Y + i * BLOCK_SIZE, 
                  colors[state->nextType + 1]);
      }
    }
  }
  
  // Texto "SCORE"
  spr.drawString("SCORE", UI_X + UI_W/2, 160, 2);
  
  // Mostrar puntuación
  spr.setTextFont(4); 
  spr.drawString(String(state->score), UI_X + UI_W/2, 190);
  spr.setTextFont(1); 
  
  // Créditos (opcional)
  spr.setTextColor(C_GRAY, C_DARKGRAY);
  spr.drawString("FreeRTOS", UI_X + UI_W/2, 280, 2);
  spr.drawString("TETRIS", UI_X + UI_W/2, 300, 2);
  
  // ========== TABLERO DE JUEGO ==========
  
  // Borde del tablero
  spr.drawRect(OFFSET_X - 2, OFFSET_Y - 2, 
               FIELD_W * BLOCK_SIZE + 4, 
               FIELD_H * BLOCK_SIZE + 4, 
               C_BORDER);
  
  // Dibujar bloques fijos del tablero
  for (int y = 0; y < FIELD_H; y++) {
    for (int x = 0; x < FIELD_W; x++) {
      if (state->board[y][x] != 0) {
        // Bloque ocupado
        drawBlock(OFFSET_X + x * BLOCK_SIZE, 
                  OFFSET_Y + y * BLOCK_SIZE, 
                  colors[state->board[y][x]]);
      } else { 
        // Punto de grid (opcional)
        spr.drawPixel(OFFSET_X + x * BLOCK_SIZE + BLOCK_SIZE/2, 
                     OFFSET_Y + y * BLOCK_SIZE + BLOCK_SIZE/2, 
                     0x18E3);
      }
    }
  }
  
  // ========== PIEZA ACTUAL ==========
  
  extern int getBlock(int type, int rot, int x, int y); // De GameLogic.h
  
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (getBlock(state->currentType, state->currentRot, j, i)) {
        int drawX = OFFSET_X + (state->currentX + j) * BLOCK_SIZE;
        int drawY = OFFSET_Y + (state->currentY + i) * BLOCK_SIZE;
        
        // Solo dibujar si está visible
        if (drawY >= OFFSET_Y) {
          drawBlock(drawX, drawY, colors[state->currentType + 1]);
        }
      }
    }
  }

  // Enviar sprite a pantalla (double buffering)
  spr.pushSprite(0, 0);
}

// ============================================
// GAME OVER (del original líneas 327-340)
// ============================================

void drawGameOver(GameState_t *state) {
  spr.fillSprite(C_BLACK);
  
  spr.setTextColor(C_RED, C_BLACK);
  spr.setTextDatum(MC_DATUM);
  spr.drawString("GAME OVER", SCREEN_W / 2, SCREEN_H / 2 - 20, 4);
  
  spr.setTextColor(C_WHITE, C_BLACK);
  spr.drawString("Final Score", SCREEN_W / 2, SCREEN_H / 2 + 20, 2);
  spr.drawString(String(state->score), SCREEN_W / 2, SCREEN_H / 2 + 45, 4);
  
  spr.drawString("Press Rotate", SCREEN_W / 2, SCREEN_H - 40, 2);
  spr.drawString("to Restart", SCREEN_W / 2, SCREEN_H - 20, 2);
  
  spr.pushSprite(0, 0);
}

#endif
