/*
 * Task SpO2 - Monitoreo de Saturación de Oxígeno
 * 
 * Tarea periódica que simula la lectura de un sensor de pulsioximetría
 * y publica los datos en una cola para consumo por Alarm y Display.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "tasks.h"
#include "sensor_simulator.h"

/*
 * ============================================================================
 * TAREA: SPO2 MONITOR
 * ============================================================================
 * 
 * FUNCIÓN:
 * Monitorea la saturación de oxígeno en sangre (SpO2) del paciente.
 * 
 * COMPORTAMIENTO:
 * 1. Se ejecuta cada 1000ms (PERIOD_SPO2_MS)
 * 2. Lee el sensor de pulsioximetría
 * 3. Empaqueta el dato con timestamp
 * 4. Escribe en xSpO2Queue usando xQueueOverwrite()
 * 5. Usa vTaskDelayUntil() para periodicidad exacta
 * 
 * PRIORIDAD: 3 (Alta)
 * Justificación: SpO2 es un signo vital crítico. Niveles bajos de oxígeno
 * (hipoxia) pueden causar daño cerebral en minutos. Requiere monitoreo
 * continuo y respuesta rápida.
 * 
 * PERÍODO: 1000ms
 * Justificación: La saturación de oxígeno es más estable que HR pero
 * igualmente crítica. 1 segundo es suficiente para detectar desaturaciones
 * mientras se conservan recursos del sistema.
 * 
 * TECNOLOGÍA DEL SENSOR (Información Médica):
 * 
 * Un pulsioxímetro mide SpO2 usando dos longitudes de onda de luz:
 * - Luz roja (660nm): Absorbida diferentemente por Hb y HbO2
 * - Luz infrarroja (940nm): Similar absorción diferencial
 * 
 * La ratio de absorción permite calcular el porcentaje de hemoglobina
 * oxigenada vs total. Es no invasivo y en tiempo real.
 * 
 * Valores normales:
 * - 95-100%: Normal
 * - 90-94%: Hipoxia leve
 * - <90%: Hipoxia significativa (ALARMA)
 * - <85%: Hipoxia severa (EMERGENCIA)
 * 
 * INTERACCIÓN CON EL SCHEDULER:
 * - Misma prioridad que Heart Rate (ambas críticas)
 * - Período más largo (1000ms vs 500ms) → consume menos CPU
 * - Si ambas están listas simultáneamente, el scheduler elige por FIFO
 * - vTaskDelayUntil() previene drift, crucial para trends a largo plazo
 */

/**
 * @brief Función de la tarea de SpO2
 * 
 * @param pvParameters Parámetros de inicialización (no usados)
 */
void vTaskSpO2(void *pvParameters) {
    (void)pvParameters;  /* Unused parameter */
    
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(PERIOD_SPO2_MS);
    SpO2Data_t spo2Data;
    
    /* Inicializar xLastWakeTime con el tiempo actual */
    xLastWakeTime = xTaskGetTickCount();
    
    /* Mensaje de inicio */
    safe_printf("[SpO2] Tarea iniciada\n");
    
    /* Loop principal de la tarea */
    for (;;) {
        /*
         * PASO 1: Leer sensor de pulsioximetría
         * 
         * En un sistema real, esto involucraría:
         * - Controlar LEDs rojo e infrarrojo
         * - Muestrear fotodetector con ADC
         * - Sincronizar con pulso cardíaco
         * - Aplicar algoritmo de ratio de ratios (R/IR)
         * - Compensar por movimiento y luz ambiente
         * - Promediar múltiples ciclos cardíacos
         * 
         * Aquí el simulador genera valores realistas con ocasionales
         * desaturaciones para testear el sistema de alarmas.
         */
        spo2Data.percentage = sensor_read_spo2();
        
        /*
         * PASO 2: Agregar timestamp
         * 
         * Crítico para:
         * - Correlación temporal con eventos (cambios en HR, T)
         * - Detección de tendencias (desaturación progresiva)
         * - Validación de datos (descartar lecturas obsoletas)
         */
        spo2Data.timestamp = xTaskGetTickCount();
        
        /*
         * PASO 3: Publicar dato en la cola
         * 
         * xQueueOverwrite() es apropiado aquí porque:
         * - SpO2 cambia lentamente (no necesitamos histórico)
         * - Queremos el valor MÁS RECIENTE siempre
         * - Las tareas consumidoras (Alarm, Display) usan xQueuePeek()
         * - No bloqueamos si Display no ha leído el valor anterior
         */
        xQueueOverwrite(xSpO2Queue, &spo2Data);
        
        /*
         * PASO 4: Esperar hasta el próximo período
         * 
         * vTaskDelayUntil() mantiene periodicidad exacta de 1000ms.
         * 
         * Ventajas en monitoreo médico:
         * - Permite detección confiable de trends
         * - Facilita correlación temporal entre sensores
         * - Evita aliasing en procesamiento de señales
         * - Cumple con requisitos regulatorios de muestreo
         * 
         * Si usáramos vTaskDelay(), el período real sería:
         *   T_real = 1000ms + T_ejecución_código
         * 
         * Con vTaskDelayUntil():
         *   T_real = exactamente 1000ms (el scheduler compensa)
         */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
    
    /* Esta línea nunca se alcanza (tarea infinita) */
}
