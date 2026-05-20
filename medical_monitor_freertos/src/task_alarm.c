/*
 * Task Alarm - Sistema de Alarmas Médicas
 * 
 * Tarea de prioridad máxima que monitorea todos los signos vitales
 * y activa alarmas cuando se detectan condiciones críticas.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "tasks.h"
#include <stdio.h>

/*
 * ============================================================================
 * TAREA: ALARM SYSTEM
 * ============================================================================
 * 
 * FUNCIÓN:
 * Sistema crítico de seguridad que detecta condiciones anormales en signos
 * vitales y alerta al personal médico.
 * 
 * COMPORTAMIENTO:
 * 1. Se ejecuta cada 100ms (PERIOD_ALARM_MS)
 * 2. Lee TODAS las colas usando xQueuePeek() (no destructivo)
 * 3. Compara valores contra umbrales críticos
 * 4. Activa alarmas visual/auditiva cuando se detectan anomalías
 * 5. Usa safe_printf() para output protegido por mutex
 * 6. Usa vTaskDelayUntil() para respuesta temporal predecible
 * 
 * PRIORIDAD: 4 (MÁXIMA - Crítica para seguridad del paciente)
 * Justificación:
 * - Debe responder INMEDIATAMENTE a condiciones potencialmente fatales
 * - Taquicardia, hipoxia o hipertermia pueden ser emergencias médicas
 * - Un retraso de segundos puede significar la diferencia entre vida y muerte
 * - NINGUNA otra tarea puede interrumpir el procesamiento de alarmas
 * 
 * PERÍODO: 100ms (el más corto del sistema)
 * Justificación:
 * - Latencia de detección < 100ms es aceptable para eventos médicos
 * - Balance entre responsividad y uso de CPU
 * - Más rápido que las tareas de sensores → siempre ve datos frescos
 * 
 * USO DE xQueuePeek() - CRÍTICO:
 * 
 * Esta tarea usa xQueuePeek() en lugar de xQueueReceive() porque:
 * 
 * 1. MÚLTIPLES CONSUMIDORES:
 *    - Tanto Alarm como Display necesitan leer los mismos datos
 *    - xQueueReceive() CONSUME (elimina) el dato de la cola
 *    - Si Alarm usa xQueueReceive(), Display nunca vería los datos
 * 
 * 2. NO ES UN RACE CONDITION:
 *    - No es una condición de carrera clásica (acceso concurrente)
 *    - Es un problema de ARQUITECTURA: multi-consumidor, single-producer
 *    - La solución NO es agregar locks/mutexes
 *    - La solución ES usar lectura no destructiva (xQueuePeek)
 * 
 * 3. SEMÁNTICA CORRECTA:
 *    - Los datos en las colas representan "estado actual del paciente"
 *    - Múltiples observadores deben poder ver el mismo estado
 *    - El estado se actualiza cuando el sensor produce un nuevo valor
 *    - No cuando un consumidor lo lee
 * 
 * ALTERNATIVAS CONSIDERADAS:
 * 
 * A) Colas separadas (un productor → múltiples colas):
 *    - Productor escribe en Cola_Alarm Y Cola_Display
 *    - Pros: Cada consumidor tiene su cola dedicada
 *    - Contras: 2x memoria, 2x escrituras, más complejidad
 * 
 * B) Memoria compartida + Mutex:
 *    - Variable global protegida por mutex
 *    - Pros: Menor overhead de colas
 *    - Contras: Más propenso a errores, bloqueos, deadlocks
 * 
 * C) xQueuePeek() (IMPLEMENTADO):
 *    - Lectura no destructiva
 *    - Pros: Simple, eficiente, múltiples lectores naturalmente
 *    - Contras: El dato permanece hasta ser sobrescrito
 *    - Veredicto: MEJOR opción para este caso de uso
 * 
 * INTERACCIÓN CON EL SCHEDULER:
 * - Prioridad 4 → SIEMPRE se ejecuta antes que cualquier otra tarea
 * - Período 100ms → se activa 10 veces por segundo
 * - Puede preempt (interrumpir) a Display, Temperature, HR, SpO2
 * - Solo el IDLE task y las ISRs tienen menor prioridad
 * - El scheduler garantiza que Alarm NUNCA espera por tareas de menor prioridad
 */

/**
 * @brief Función de la tarea de Alarm
 * 
 * @param pvParameters Parámetros de inicialización (no usados)
 */
void vTaskAlarm(void *pvParameters) {
    (void)pvParameters;  /* Unused parameter */
    
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(PERIOD_ALARM_MS);
    
    /* Estructuras para almacenar datos leídos */
    HeartRateData_t hrData;
    SpO2Data_t spo2Data;
    TemperatureData_t tempData;
    
    /* Flags de estado de alarmas */
    uint8_t hrAlarmActive = 0;
    uint8_t spo2AlarmActive = 0;
    uint8_t tempAlarmActive = 0;
    
    /* Inicializar xLastWakeTime con el tiempo actual */
    xLastWakeTime = xTaskGetTickCount();
    
    /* Mensaje de inicio */
    safe_printf("[Alarm] Sistema de alarmas iniciado\n");
    
    /* Loop principal de la tarea */
    for (;;) {
        /*
         * PASO 1: Leer datos de Heart Rate usando xQueuePeek()
         * 
         * xQueuePeek() vs xQueueReceive():
         * 
         * xQueuePeek(queue, &buffer, timeout):
         *   - Lee el dato PERO NO lo elimina de la cola
         *   - Otros pueden leer el mismo dato
         *   - Retorna pdTRUE si hay dato, pdFALSE si la cola está vacía
         * 
         * xQueueReceive(queue, &buffer, timeout):
         *   - Lee el dato Y lo elimina de la cola
         *   - El próximo lector NO verá este dato
         *   - ¡INCORRECTO para nuestro caso de uso!
         */
        if (xQueuePeek(xHeartRateQueue, &hrData, 0) == pdTRUE) {
            /*
             * Verificar umbrales de Heart Rate
             * 
             * Condiciones anormales:
             * - Taquicardia: HR > 120 bpm
             * - Bradicardia: HR < 50 bpm
             */
            if (hrData.bpm > HR_THRESHOLD_HIGH) {
                /* Taquicardia detectada */
                if (!hrAlarmActive) {
                    safe_printf("\n*** ALARMA: TAQUICARDIA (HR=%.1f bpm) ***\n\n", 
                               hrData.bpm);
                    hrAlarmActive = 1;
                }
            } else if (hrData.bpm < HR_THRESHOLD_LOW) {
                /* Bradicardia detectada */
                if (!hrAlarmActive) {
                    safe_printf("\n*** ALARMA: BRADICARDIA (HR=%.1f bpm) ***\n\n", 
                               hrData.bpm);
                    hrAlarmActive = 1;
                }
            } else {
                /* HR normal - desactivar alarma si estaba activa */
                if (hrAlarmActive) {
                    hrAlarmActive = 0;
                }
            }
        }
        
        /*
         * PASO 2: Leer datos de SpO2 usando xQueuePeek()
         * 
         * Timeout = 0 → no bloquea
         * Si la cola está vacía (sensor aún no ha publicado), simplemente
         * no verificamos esta iteración y continuamos.
         */
        if (xQueuePeek(xSpO2Queue, &spo2Data, 0) == pdTRUE) {
            /*
             * Verificar umbrales de SpO2
             * 
             * Condiciones anormales:
             * - Hipoxia: SpO2 < 90%
             * 
             * Niveles de severidad:
             * - 90-94%: Hipoxia leve
             * - 85-89%: Hipoxia moderada
             * - <85%: Hipoxia severa (emergencia)
             */
            if (spo2Data.percentage < SPO2_THRESHOLD_LOW) {
                /* Hipoxia detectada */
                if (!spo2AlarmActive) {
                    safe_printf("\n*** ALARMA: HIPOXIA (SpO2=%.1f%%) ***\n\n", 
                               spo2Data.percentage);
                    spo2AlarmActive = 1;
                }
            } else {
                /* SpO2 normal - desactivar alarma si estaba activa */
                if (spo2AlarmActive) {
                    spo2AlarmActive = 0;
                }
            }
        }
        
        /*
         * PASO 3: Leer datos de Temperature usando xQueuePeek()
         */
        if (xQueuePeek(xTemperatureQueue, &tempData, 0) == pdTRUE) {
            /*
             * Verificar umbrales de Temperature
             * 
             * Condiciones anormales:
             * - Fiebre: Temp > 38.0°C
             * - Hipotermia: Temp < 35.5°C
             * 
             * Niveles de severidad:
             * - 38.0-38.9°C: Fiebre leve
             * - 39.0-40.0°C: Fiebre alta
             * - >40.0°C: Hipertermia (emergencia)
             * - <35.0°C: Hipotermia severa (emergencia)
             */
            if (tempData.celsius > TEMP_THRESHOLD_HIGH) {
                /* Fiebre detectada */
                if (!tempAlarmActive) {
                    safe_printf("\n*** ALARMA: FIEBRE (Temp=%.1f°C) ***\n\n", 
                               tempData.celsius);
                    tempAlarmActive = 1;
                }
            } else if (tempData.celsius < TEMP_THRESHOLD_LOW) {
                /* Hipotermia detectada */
                if (!tempAlarmActive) {
                    safe_printf("\n*** ALARMA: HIPOTERMIA (Temp=%.1f°C) ***\n\n", 
                               tempData.celsius);
                    tempAlarmActive = 1;
                }
            } else {
                /* Temperatura normal - desactivar alarma si estaba activa */
                if (tempAlarmActive) {
                    tempAlarmActive = 0;
                }
            }
        }
        
        /*
         * En un sistema real, aquí también se:
         * - Activarían salidas físicas (LEDs, buzzer)
         * - Enviarían notificaciones a estación de enfermería
         * - Registrarían eventos en log
         * - Escalarían alarmas según severidad
         * - Implementarían lógica de silenciamiento temporal
         */
        
        /*
         * PASO 4: Esperar hasta el próximo período
         * 
         * vTaskDelayUntil() garantiza que la verificación de alarmas
         * ocurre exactamente cada 100ms, sin drift acumulativo.
         * 
         * Esto es CRÍTICO porque:
         * - Garantiza latencia máxima de detección de 100ms
         * - Cumple con requisitos de tiempo real duro
         * - Permite cumplir estándares médicos (IEC 60601)
         */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
    
    /* Esta línea nunca se alcanza (tarea infinita) */
}
