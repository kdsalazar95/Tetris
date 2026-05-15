#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>
#include "TetrisStructures.h"

// ============================================
// DECLARACIONES GLOBALES (extern)
// ============================================

// Handles de tareas
extern TaskHandle_t xInputTaskHandle;
extern TaskHandle_t xGameLogicTaskHandle;
extern TaskHandle_t xRenderTaskHandle;

// Colas
extern QueueHandle_t xInputQueue;   // Input → Logic
extern QueueHandle_t xRenderQueue;  // Logic → Render

// Semáforos
extern SemaphoreHandle_t xDisplayMutex;
extern SemaphoreHandle_t xGameStateMutex;

// Timer
extern TimerHandle_t xPieceFallTimer;

// Estado del juego
extern GameState_t currentGameState;

#endif

