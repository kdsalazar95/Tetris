// ============================================
// TETRIS CON FreeRTOS - ESP32
// Mini-taller Sistemas Embebidos
// Instituto Tecnológico de Costa Rica
// ============================================

#include <Arduino.h>
#include <SPI.h>
#include "Globals.h"
#include "HardwareConfig.h"
#include "TetrisStructures.h"
#include "GameLogic.h"
#include "DisplayDriver.h"
#include "TetrisTasks.h"
#include "Globals.h"

// ============================================
// VARIABLES GLOBALES DE FreeRTOS
// ============================================

// Handles de tareas
TaskHandle_t xInputTaskHandle = NULL;
TaskHandle_t xGameLogicTaskHandle = NULL;
TaskHandle_t xRenderTaskHandle = NULL;

// Colas para comunicación entre tareas
QueueHandle_t xInputQueue = NULL;      // Input → Logic
QueueHandle_t xRenderQueue = NULL;     // Logic → Render

// Semáforos para proteger recursos compartidos
SemaphoreHandle_t xDisplayMutex = NULL;
SemaphoreHandle_t xGameStateMutex = NULL;

// Timer para caída automática de piezas
TimerHandle_t xPieceFallTimer = NULL;

// Estado del juego (compartido entre tareas)
GameState_t currentGameState;

// ============================================
// CALLBACK DEL TIMER - CAÍDA AUTOMÁTICA
// ============================================

void vPieceFallTimerCallback(TimerHandle_t xTimer) {
    InputEvent_t fallEvent = EVENT_MOVE_DOWN;
    
    // Enviar evento de caída a la cola de input
    xQueueSend(xInputQueue, &fallEvent, 0);
}

// ============================================
// SETUP - INICIALIZACIÓN
// ============================================

void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=================================");
    Serial.println("  FreeRTOS TETRIS - Iniciando");
    Serial.println("=================================\n");
    
    // --- 1. INICIALIZAR HARDWARE ---
    Serial.println("[SETUP] Inicializando hardware...");
    
    initButtons();
    initDisplay();
    
    // Seed para números aleatorios
    randomSeed(analogRead(0));
    
    Serial.println("[SETUP] Hardware OK");
    
    // --- 2. CREAR COLAS ---
    Serial.println("[SETUP] Creando colas...");
    
    xInputQueue = xQueueCreate(10, sizeof(InputEvent_t));
    xRenderQueue = xQueueCreate(1, sizeof(GameState_t));
    
    if (xInputQueue == NULL || xRenderQueue == NULL) {
        Serial.println("[ERROR] No se pudieron crear las colas!");
        while(1) {
            delay(1000); // Halt
        }
    }
    
    Serial.println("[SETUP] Colas creadas:");
    Serial.println("  - xInputQueue: 10 elementos");
    Serial.println("  - xRenderQueue: 1 elemento");
    
    // --- 3. CREAR SEMÁFOROS ---
    Serial.println("[SETUP] Creando semáforos...");
    
    xDisplayMutex = xSemaphoreCreateMutex();
    xGameStateMutex = xSemaphoreCreateMutex();
    
    if (xDisplayMutex == NULL || xGameStateMutex == NULL) {
        Serial.println("[ERROR] No se pudieron crear los semáforos!");
        while(1) {
            delay(1000);
        }
    }
    
    Serial.println("[SETUP] Semáforos creados:");
    Serial.println("  - xDisplayMutex");
    Serial.println("  - xGameStateMutex");
    
    // --- 4. INICIALIZAR ESTADO DEL JUEGO ---
    Serial.println("[SETUP] Inicializando juego...");
    
    if (xSemaphoreTake(xGameStateMutex, portMAX_DELAY) == pdTRUE) {
        initializeGame(&currentGameState);
        xSemaphoreGive(xGameStateMutex);
    }
    
    Serial.println("[SETUP] Juego inicializado");
    
    // --- 5. CREAR TAREAS ---
    Serial.println("[SETUP] Creando tareas FreeRTOS...");
    
    // TAREA 1: Input Handler (Prioridad ALTA - 3)
    BaseType_t result1 = xTaskCreatePinnedToCore(
        vTaskInputHandler,          // Función de la tarea
        "InputTask",                // Nombre descriptivo
        4096,                       // Stack size (bytes)
        NULL,                       // Parámetros
        3,                          // Prioridad (0-24, mayor = más prioritario)
        &xInputTaskHandle,          // Handle de la tarea
        1                           // Core 1 (ESP32 dual-core)
    );
    
    if (result1 != pdPASS) {
        Serial.println("[ERROR] No se pudo crear InputTask!");
        while(1) delay(1000);
    }
    Serial.println("  ✓ InputTask (Prioridad 3, Core 1)");
    
    // TAREA 2: Game Logic (Prioridad MEDIA-ALTA - 2)
    BaseType_t result2 = xTaskCreatePinnedToCore(
        vTaskGameLogic,
        "LogicTask",
        8192,                       // Más stack para lógica
        NULL,
        2,
        &xGameLogicTaskHandle,
        1                           // Core 1
    );
    
    if (result2 != pdPASS) {
        Serial.println("[ERROR] No se pudo crear LogicTask!");
        while(1) delay(1000);
    }
    Serial.println("  ✓ LogicTask (Prioridad 2, Core 1)");
    
    // TAREA 3: Display Renderer (Prioridad BAJA - 1)
    BaseType_t result3 = xTaskCreatePinnedToCore(
        vTaskDisplayRenderer,
        "RenderTask",
        4096,
        NULL,
        1,
        &xRenderTaskHandle,
        0                           // Core 0 (separar rendering)
    );
    
    if (result3 != pdPASS) {
        Serial.println("[ERROR] No se pudo crear RenderTask!");
        while(1) delay(1000);
    }
    Serial.println("  ✓ RenderTask (Prioridad 1, Core 0)");
    
    // --- 6. CREAR TIMER SOFTWARE ---
    Serial.println("[SETUP] Creando timer de caída...");
    
    xPieceFallTimer = xTimerCreate(
        "FallTimer",                        // Nombre
        pdMS_TO_TICKS(500),                // Periodo inicial: 500ms
        pdTRUE,                            // Auto-reload (periódico)
        (void *)0,                         // Timer ID
        vPieceFallTimerCallback            // Callback
    );
    
    if (xPieceFallTimer == NULL) {
        Serial.println("[ERROR] No se pudo crear el timer!");
        while(1) delay(1000);
    }
    
    // Iniciar timer
    if (xTimerStart(xPieceFallTimer, 0) != pdPASS) {
        Serial.println("[ERROR] No se pudo iniciar el timer!");
        while(1) delay(1000);
    }
    
    Serial.println("  ✓ FallTimer (500ms, auto-reload)");
    
    // --- 7. FINALIZACIÓN ---
    Serial.println("\n[SETUP] Inicialización completa!");
    Serial.println("=================================");
    Serial.println("  TAREAS ACTIVAS:");
    Serial.println("  - InputTask: Lectura de botones");
    Serial.println("  - LogicTask: Lógica del juego");
    Serial.println("  - RenderTask: Dibujo en pantalla");
    Serial.println("  - FallTimer: Caída automática");
    Serial.println("=================================\n");
    
    delay(1000); // Pausa para leer el mensaje
}

// ============================================
// LOOP - VACÍO (FreeRTOS maneja todo)
// ============================================

void loop() {
    // El loop() ya no se usa en aplicaciones FreeRTOS.
    // Todas las tareas se ejecutan mediante el scheduler.
    // Sin embargo, el loop() debe existir para compatibilidad con Arduino.
    
    // Solo esperamos para no consumir CPU
    vTaskDelay(pdMS_TO_TICKS(1000)); // Sleep 1 segundo
    
    // Opcional: Monitoreo de estado
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 10000) { // Cada 10 segundos
        Serial.println("[MONITOR] Sistema FreeRTOS funcionando...");
        Serial.printf("  Score: %d | Level: %d | Game Over: %s\n", 
                     currentGameState.score, 
                     currentGameState.level,
                     currentGameState.gameOver ? "YES" : "NO");
        lastPrint = millis();
    }
}

// ============================================
// INFORMACIÓN ADICIONAL
// ============================================

/*
 * DIAGRAMA DE ARQUITECTURA:
 * 
 *   [BOTONES] 
 *       ↓
 *   InputTask (P=3, Core 1)
 *       ↓ (xInputQueue)
 *   LogicTask (P=2, Core 1)
 *       ↓ (xRenderQueue)
 *   RenderTask (P=1, Core 0)
 *       ↓
 *   [DISPLAY]
 * 
 *   FallTimer → InputTask (cada 500ms)
 * 
 * RECURSOS COMPARTIDOS:
 * - xDisplayMutex: Protege acceso al TFT
 * - xGameStateMutex: Protege currentGameState
 * 
 * COLAS:
 * - xInputQueue: Eventos de botones (10 elementos)
 * - xRenderQueue: Estado del juego (1 elemento, overwrite)
 */

/*
 * CONFIGURACIÓN DEL HARDWARE:
 * 
 * Display ILI9341 (SPI):
 * - MOSI → GPIO 23
 * - MISO → GPIO 19
 * - SCK  → GPIO 18
 * - CS   → GPIO 15
 * - DC   → GPIO 2
 * - RST  → GPIO 4
 * 
 * Botones (INPUT_PULLUP):
 * - LEFT   → GPIO 18
 * - RIGHT  → GPIO 8
 * - DOWN   → GPIO 17
 * - ROTATE → GPIO 16
 * 
 * Buzzer (opcional):
 * - BUZZER → GPIO 14
 * 
 * NOTA: Ajusta los pines en HardwareConfig.h según tu hardware
 */

/*
 * COMPILACIÓN:
 * 
 * Arduino IDE:
 * 1. Herramientas → Placa → ESP32 Dev Module
 * 2. Herramientas → Flash Size → 4MB
 * 3. Herramientas → Partition Scheme → Default
 * 4. Subir
 * 
 * PlatformIO:
 * platform = espressif32
 * board = esp32dev
 * framework = arduino
 * lib_deps = 
 *     bodmer/TFT_eSPI@^2.5.0
 */

/*
 * DEBUGGING:
 * 
 * Para ver mensajes de debug, abre el Serial Monitor a 115200 baudios.
 * Verás:
 * - Inicialización de tareas
 * - Eventos procesados
 * - Errores (si los hay)
 * - Monitor de estado (cada 10 seg)
 */

/*
 * AJUSTE DE VELOCIDAD:
 * 
 * La velocidad inicial es 500ms (configurado en xTimerCreate).
 * La velocidad aumenta automáticamente en GameLogic.h → clearLines()
 * según la fórmula: dropInterval = max(100, 500 - (score / 500) * 50)
 * 
 * Para cambiar velocidad inicial, modifica el valor en pdMS_TO_TICKS(500)
 */

/*
 * PARA EL MINI-TALLER:
 * 
 * DEMOSTRACIÓN:
 * 1. Explicar arquitectura de tareas (diagrama arriba)
 * 2. Mostrar código de una tarea (ej: InputTask)
 * 3. Ejecutar juego y mostrar Serial Monitor
 * 4. Mostrar cómo se comunican tareas (colas)
 * 5. Demostrar prioridades (comentar RenderTask temporalmente)
 * 
 * EJERCICIO PROPUESTO:
 * - Agregar una 4ta tarea para mostrar nivel en display
 * - Modificar prioridades y observar diferencias
 * - Agregar segunda cola para efectos de sonido
 */
