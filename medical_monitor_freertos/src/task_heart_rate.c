/*
 * Task Heart Rate - Monitoreo de Frecuencia Cardíaca
 * 
 * Tarea periódica que simula la lectura de un sensor de frecuencia cardíaca
 * y publica los datos en una cola para consumo por Alarm y Display.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "tasks.h"
#include "sensor_simulator.h"

/*
 * ============================================================================
 * TAREA: HEART RATE MONITOR
 * ============================================================================
 * 
 * FUNCIÓN:
 * Monitorea la frecuencia cardíaca del paciente en tiempo real.
 * 
 * COMPORTAMIENTO:
 * 1. Se ejecuta cada 500ms (PERIOD_HEART_RATE_MS)
 * 2. Lee el sensor de frecuencia cardíaca
 * 3. Empaqueta el dato con timestamp
 * 4. Escribe en xHeartRateQueue usando xQueueOverwrite()
 * 5. Usa vTaskDelayUntil() para periodicidad exacta
 * 
 * PRIORIDAD: 3 (Alta)
 * Justificación: La frecuencia cardíaca es un signo vital crítico.
 * Cambios rápidos pueden indicar emergencias (arritmias, shock).
 * 
 * PERÍODO: 500ms
 * Justificación: Permite detectar cambios rápidos en HR.
 * Más frecuente que SpO2 porque la HR puede variar más rápidamente.
 * 
 * USO DE xQueueOverwrite():
 * En lugar de xQueueSend(), usamos xQueueOverwrite() que:
 * - Siempre escribe el dato, sobrescribiendo el anterior si la cola está llena
 * - No bloquea nunca (no espera espacio disponible)
 * - Garantiza que siempre tenemos el dato MÁS RECIENTE
 * 
 * Esto es apropiado porque:
 * - Solo nos interesa el valor actual (no histórico)
 * - Las tareas consumidoras usan xQueuePeek() (lectura no destructiva)
 * - Previene pérdida de datos antiguos bloqueando al productor
 * 
 * INTERACCIÓN CON EL SCHEDULER:
 * - El scheduler despierta esta tarea cada 500ms exactamente
 * - Si una tarea de mayor prioridad (Alarm) está lista, se ejecuta primero
 * - El uso de vTaskDelayUntil() previene drift temporal acumulativo
 * - La alta prioridad (3) asegura que la lectura no se retrase mucho
 */

/**
 * @brief Función de la tarea de Heart Rate
 * 
 * @param pvParameters Parámetros de inicialización (no usados)
 */
void vTaskHeartRate(void *pvParameters) {
    (void)pvParameters;  /* Unused parameter */
    
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(PERIOD_HEART_RATE_MS);
    HeartRateData_t hrData;
    
    /* Inicializar xLastWakeTime con el tiempo actual */
    xLastWakeTime = xTaskGetTickCount();
    
    /* Mensaje de inicio */
    safe_printf("[HeartRate] Tarea iniciada\n");
    
    /* Loop principal de la tarea */
    for (;;) {
        /*
         * PASO 1: Leer sensor de frecuencia cardíaca
         * 
         * En un sistema real, esto sería:
         * - Configurar ADC para muestrear señal ECG
         * - Procesar señal (filtrado, detección de picos R)
         * - Calcular BPM desde intervalos R-R
         * 
         * Aquí usamos el simulador que genera valores realistas
         */
        hrData.bpm = sensor_read_heart_rate();
        
        /*
         * PASO 2: Agregar timestamp
         * 
         * El timestamp permite:
         * - Detectar datos obsoletos
         * - Correlacionar eventos entre sensores
         * - Calcular latencias del sistema
         */
        hrData.timestamp = xTaskGetTickCount();
        
        /*
         * PASO 3: Publicar dato en la cola
         * 
         * xQueueOverwrite() garantiza que el dato se escribe siempre.
         * Si la cola estaba llena, sobrescribe el valor anterior.
         * Esto es correcto porque solo nos interesa el valor MÁS RECIENTE.
         */
        xQueueOverwrite(xHeartRateQueue, &hrData);
        
        /*
         * PASO 4: Esperar hasta el próximo período
         * 
         * vTaskDelayUntil() vs vTaskDelay():
         * 
         * vTaskDelay(500ms):
         *   - Espera 500ms desde AHORA
         *   - Acumula drift si el código toma tiempo
         *   - Período real > 500ms
         * 
         * vTaskDelayUntil(&xLastWakeTime, 500ms):
         *   - Espera hasta xLastWakeTime + 500ms
         *   - Compensa el tiempo de ejecución del código
         *   - Período real = exactamente 500ms
         *   - CRUCIAL para muestreo periódico exacto
         * 
         * Para monitoreo médico, necesitamos muestreo periódico EXACTO,
         * por lo que siempre usamos vTaskDelayUntil().
         */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
    
    /* Esta línea nunca se alcanza (tarea infinita) */
}
