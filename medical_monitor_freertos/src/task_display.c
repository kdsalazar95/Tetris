/*
 * Task Display - Interfaz de Visualización del Monitor
 * 
 * Tarea de baja prioridad que muestra todos los signos vitales
 * en la interfaz UART del monitor médico.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "tasks.h"
#include <stdio.h>

/*
 * ============================================================================
 * TAREA: DISPLAY
 * ============================================================================
 * 
 * FUNCIÓN:
 * Proporciona la interfaz de usuario del monitor médico, mostrando todos
 * los signos vitales en tiempo real a través del puerto UART.
 * 
 * COMPORTAMIENTO:
 * 1. Se ejecuta cada 1000ms (PERIOD_DISPLAY_MS)
 * 2. Lee TODAS las colas usando xQueuePeek() (no destructivo)
 * 3. Formatea los datos en un display coherente
 * 4. Imprime usando safe_printf() (protegido por mutex)
 * 5. Usa vTaskDelayUntil() para actualización periódica
 * 
 * PRIORIDAD: 1 (MÍNIMA - No crítica para seguridad)
 * Justificación:
 * - La visualización es importante para el usuario, NO para el paciente
 * - Un retraso en el display no pone en riesgo la vida del paciente
 * - Las alarmas (prioridad 4) se activan independientemente del display
 * - El monitoreo continúa aunque el display se retrase
 * - Puede ser interrumpida por CUALQUIER otra tarea sin consecuencias
 * 
 * PERÍODO: 1000ms
 * Justificación:
 * - 1 actualización por segundo es suficiente para lectura humana
 * - El ojo humano no puede procesar actualizaciones más rápidas útilmente
 * - Reduce parpadeo en la pantalla
 * - Minimiza uso de CPU para tarea no crítica
 * 
 * USO DE xQueuePeek() - CRÍTICO:
 * 
 * Esta tarea DEBE usar xQueuePeek() en lugar de xQueueReceive() porque:
 * 
 * 1. PROBLEMA DE ARQUITECTURA MULTI-CONSUMIDOR:
 *    - Display y Alarm necesitan leer los MISMOS datos
 *    - Las colas tienen tamaño 1 (solo el valor más reciente)
 *    - Si Display usa xQueueReceive(), CONSUME el dato
 *    - Alarm (que se ejecuta cada 100ms) podría no ver ese dato
 *    - Resultado: PÉRDIDA DE DATOS CRÍTICOS DE SEGURIDAD
 * 
 * 2. NO ES UN RACE CONDITION:
 *    - No es acceso concurrente a recurso compartido
 *    - No se soluciona con locks, mutexes o semáforos
 *    - Es un problema de SEMÁNTICA de comunicación
 *    - La solución correcta es lectura no destructiva
 * 
 * 3. EJEMPLO DEL PROBLEMA:
 * 
 *    Sin xQueuePeek() (INCORRECTO):
 *    --------------------------------
 *    t=0ms:   Sensor escribe HR=130 bpm (taquicardia) en cola
 *    t=50ms:  Display ejecuta, llama xQueueReceive() → lee HR=130, DESTRUYE dato
 *    t=100ms: Alarm ejecuta, llama xQueueReceive() → cola VACÍA, no detecta alarma!
 *    
 *    ¡FALLO CRÍTICO! La alarma nunca se activó.
 * 
 *    Con xQueuePeek() (CORRECTO):
 *    ----------------------------
 *    t=0ms:   Sensor escribe HR=130 bpm en cola
 *    t=50ms:  Display ejecuta, llama xQueuePeek() → lee HR=130, dato PERMANECE
 *    t=100ms: Alarm ejecuta, llama xQueuePeek() → lee HR=130, ¡ALARMA ACTIVADA!
 *    
 *    ✓ Ambas tareas ven el dato. Sistema funciona correctamente.
 * 
 * 4. COMPORTAMIENTO DE xQueuePeek():
 *    - Lee el dato pero NO lo elimina
 *    - El dato permanece hasta que el productor escribe un nuevo valor
 *    - xQueueOverwrite() del productor sobrescribe el dato anterior
 *    - Múltiples consumidores pueden leer el mismo valor
 * 
 * INTERACCIÓN CON EL SCHEDULER:
 * - Prioridad 1 (la más baja, excepto Idle)
 * - Se ejecuta solo cuando NO hay tareas de mayor prioridad listas
 * - Si Alarm (prioridad 4) está lista, Display espera
 * - Si HR, SpO2, Temp (prioridades 2-3) están listas, Display espera
 * - Esto es CORRECTO: visualización no es crítica para seguridad
 * - vTaskDelayUntil() garantiza actualización regular cuando tiene oportunidad
 */

/**
 * @brief Función de la tarea de Display
 * 
 * @param pvParameters Parámetros de inicialización (no usados)
 */
void vTaskDisplay(void *pvParameters) {
    (void)pvParameters;  /* Unused parameter */
    
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(PERIOD_DISPLAY_MS);
    
    /* Estructuras para almacenar datos leídos */
    HeartRateData_t hrData;
    SpO2Data_t spo2Data;
    TemperatureData_t tempData;
    
    /* Flags para tracking de disponibilidad de datos */
    uint8_t hrAvailable = 0;
    uint8_t spo2Available = 0;
    uint8_t tempAvailable = 0;
    
    /* Inicializar xLastWakeTime con el tiempo actual */
    xLastWakeTime = xTaskGetTickCount();
    
    /* Mensaje de inicio */
    safe_printf("[Display] Tarea iniciada\n\n");
    
    /* Loop principal de la tarea */
    for (;;) {
        /*
         * PASO 1: Leer datos de Heart Rate usando xQueuePeek()
         * 
         * CRÍTICO: Usar xQueuePeek(), NO xQueueReceive()
         * 
         * Razón: No queremos CONSUMIR el dato que Alarm necesita.
         * Si usáramos xQueueReceive() y el dato se eliminara antes de que
         * Alarm lo lea, podríamos perder una alarma crítica.
         */
        if (xQueuePeek(xHeartRateQueue, &hrData, 0) == pdTRUE) {
            hrAvailable = 1;
        }
        
        /*
         * PASO 2: Leer datos de SpO2 usando xQueuePeek()
         * 
         * Timeout = 0 → no bloqueante
         * Si el sensor aún no ha publicado datos, simplemente no mostramos
         * ese valor en esta iteración.
         */
        if (xQueuePeek(xSpO2Queue, &spo2Data, 0) == pdTRUE) {
            spo2Available = 1;
        }
        
        /*
         * PASO 3: Leer datos de Temperature usando xQueuePeek()
         */
        if (xQueuePeek(xTemperatureQueue, &tempData, 0) == pdTRUE) {
            tempAvailable = 1;
        }
        
        /*
         * PASO 4: Formatear y mostrar el display
         * 
         * En un sistema real, esto podría ser:
         * - Display LCD/OLED con gráficos de onda
         * - Panel táctil con múltiples vistas
         * - Interfaz web para monitoreo remoto
         * 
         * Aquí usamos UART con formato simple pero claro.
         */
        
        /* Usar safe_printf() para protección con mutex */
        safe_printf("[Monitor] ");
        
        /* Heart Rate */
        if (hrAvailable) {
            safe_printf("HR: %.1f bpm | ", hrData.bpm);
        } else {
            safe_printf("HR: --.- bpm | ");
        }
        
        /* SpO2 */
        if (spo2Available) {
            safe_printf("SpO2: %.1f%% | ", spo2Data.percentage);
        } else {
            safe_printf("SpO2: --.-%% | ");
        }
        
        /* Temperature */
        if (tempAvailable) {
            safe_printf("Temp: %.1f°C\n", tempData.celsius);
        } else {
            safe_printf("Temp: --.°C\n");
        }
        
        /*
         * MEJORAS FUTURAS:
         * 
         * 1. Mostrar tendencias:
         *    - Flechas ↑↓ indicando si el valor está subiendo o bajando
         *    - Requiere almacenar valores históricos
         * 
         * 2. Códigos de color (si terminal lo soporta):
         *    - Verde: valores normales
         *    - Amarillo: valores en límite
         *    - Rojo: valores anormales
         * 
         * 3. Gráficos de onda:
         *    - ECG simulado para heart rate
         *    - Plethysmograph para SpO2
         *    - Requiere framebuffer o display gráfico
         * 
         * 4. Información adicional:
         *    - Estado de batería
         *    - Tiempo desde última medición
         *    - Estado de conexión de sensores
         *    - Alarmas activas
         * 
         * 5. Múltiples modos de vista:
         *    - Vista compacta (1 línea)
         *    - Vista detallada (múltiples líneas)
         *    - Vista de tendencias (gráficos)
         */
        
        /*
         * PASO 5: Esperar hasta el próximo período
         * 
         * vTaskDelayUntil() garantiza actualización regular del display
         * cada 1000ms exactamente.
         * 
         * Consideraciones:
         * 
         * - ¿Por qué 1000ms?
         *   · Frecuencia adecuada para lectura humana
         *   · No tan rápido que cause parpadeo molesto
         *   · No tan lento que parezca no responsivo
         * 
         * - ¿Por qué vTaskDelayUntil()?
         *   · Previene drift acumulativo
         *   · Actualización predecible y regular
         *   · Mejor experiencia de usuario (no errático)
         * 
         * - ¿Qué pasa si la tarea se retrasa?
         *   · Si tareas de mayor prioridad consumen mucho CPU
         *   · vTaskDelayUntil() se "pone al día" en la próxima oportunidad
         *   · Puede saltar frames si el retraso es muy grande
         *   · Esto es ACEPTABLE para display (no crítico)
         */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
    
    /* Esta línea nunca se alcanza (tarea infinita) */
}
