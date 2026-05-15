#ifndef TETRIS_TASKS_H
#define TETRIS_TASKS_H

#include "DisplayDriver.h"
#include "TetrisStructures.h"
#include "Globals.h"


// ============================================
// TAREA 1: INPUT HANDLER (Prioridad ALTA)
// ============================================
void vTaskInputHandler(void *pvParameters) {
    InputEvent_t event;
    const TickType_t xDelay = pdMS_TO_TICKS(50); // Poll cada 50ms
    
    while(1) {
        event = EVENT_NONE;
        
        // Leer botones
        if (digitalRead(BUTTON_LEFT) == LOW) {
            event = EVENT_MOVE_LEFT;
        }
        else if (digitalRead(BUTTON_RIGHT) == LOW) {
            event = EVENT_MOVE_RIGHT;
        }
        else if (digitalRead(BUTTON_DOWN) == LOW) {
            event = EVENT_MOVE_DOWN;
        }
        else if (digitalRead(BUTTON_ROTATE) == LOW) {
            event = EVENT_ROTATE;
        }
        else if (digitalRead(BUTTON_DROP) == LOW) {
            event = EVENT_DROP;
        }
        
        // Si hay evento, enviarlo a cola
        if (event != EVENT_NONE) {
            xQueueSend(xInputQueue, &event, 0);
            vTaskDelay(pdMS_TO_TICKS(200)); // Debounce simple
        }
        
        vTaskDelay(xDelay);
    }
}

// ============================================
// TAREA 2: GAME LOGIC (Prioridad MEDIA-ALTA)
// ============================================
void vTaskGameLogic(void *pvParameters) {
    InputEvent_t receivedEvent;
    GameState_t localState;
    
    initializeGame(&localState);
    
    while(1) {
        if (xQueueReceive(xInputQueue, &receivedEvent, 0) == pdTRUE) {
            if (xSemaphoreTake(xGameStateMutex, portMAX_DELAY) == pdTRUE) {
                
                switch(receivedEvent) {
                    case EVENT_MOVE_LEFT:
                        if (canMovePiece(&localState, -1, 0)) localState.currentX--;
                        break;
                    case EVENT_MOVE_RIGHT:
                        if (canMovePiece(&localState, 1, 0)) localState.currentX++;
                        break;
                    case EVENT_MOVE_DOWN:
                        if (canMovePiece(&localState, 0, 1)) localState.currentY++;
                        break;
                    case EVENT_ROTATE:
                        rotatePiece(&localState);
                        break;
                    case EVENT_DROP:
                        dropPiece(&localState);
                        break;
                    case EVENT_PAUSE:
                        localState.paused = !localState.paused;
                        break;
                    default:
                        break;
                }
                
                memcpy(&currentGameState, &localState, sizeof(GameState_t));
                xSemaphoreGive(xGameStateMutex);
                xQueueOverwrite(xRenderQueue, &localState);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ============================================
// TAREA 3: DISPLAY RENDERER (Prioridad BAJA)
// ============================================
void vTaskDisplayRenderer(void *pvParameters) {
    GameState_t stateToRender;
    const TickType_t xDelay = pdMS_TO_TICKS(33);

    while(1) {
        if (xQueuePeek(xRenderQueue, &stateToRender, 0) == pdTRUE) {
            if (xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE) {
                drawGame(&stateToRender);  // dibuja tablero, pieza y score
                xSemaphoreGive(xDisplayMutex);
            }
        }
        vTaskDelay(xDelay);
    }
}


#endif

