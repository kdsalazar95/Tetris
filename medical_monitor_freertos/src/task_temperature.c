/*
 * Task Temperature - Monitoreo de Temperatura Corporal
 * 
 * Tarea periódica que simula la lectura de un sensor de temperatura
 * y publica los datos en una cola para consumo por Alarm y Display.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "tasks.h"
#include "sensor_simulator.h"

/*
 * ============================================================================
 * TAREA: TEMPERATURE MONITOR
 * ============================================================================
 * 
 * FUNCIÓN:
 * Monitorea la temperatura corporal del paciente.
 * 
 * COMPORTAMIENTO:
 * 1. Se ejecuta cada 2000ms (PERIOD_TEMPERATURE_MS)
 * 2. Lee el sensor de temperatura
 * 3. Empaqueta el dato con timestamp
 * 4. Escribe en xTemperatureQueue usando xQueueOverwrite()
 * 5. Usa vTaskDelayUntil() para periodicidad exacta
 * 
 * PRIORIDAD: 2 (Media)
 * Justificación: Aunque importante, la temperatura corporal cambia MUY
 * lentamente comparada con HR y SpO2. Una fiebre se desarrolla en horas,
 * no en segundos. Por lo tanto, puede tener menor prioridad sin comprometer
 * la seguridad del paciente.
 * 
 * PERÍODO: 2000ms
 * Justificación:
 * - La temperatura corporal es muy estable (inercia térmica alta)
 * - Cambios significativos toman minutos/horas, no segundos
 * - 2 segundos es más que suficiente para detectar tendencias
 * - Reduce carga del sistema vs períodos más cortos
 * 
 * TECNOLOGÍA DEL SENSOR (Información Médica):
 * 
 * Tipos de sensores de temperatura corporal:
 * 
 * 1. Termistor NTC:
 *    - Resistencia varía con temperatura
 *    - Precisión ±0.1°C
 *    - Respuesta rápida (segundos)
 *    - Usado en monitores médicos
 * 
 * 2. Termopar:
 *    - Voltaje proporcional a diferencia de temperatura
 *    - Muy preciso y estable
 *    - Más costoso
 * 
 * 3. Infrarrojo (sin contacto):
 *    - Mide radiación térmica
 *    - No invasivo
 *    - Menos preciso que sensores de contacto
 * 
 * Valores normales de temperatura:
 * - Oral: 36.5-37.5°C
 * - Rectal: 37.0-38.0°C (más precisa)
 * - Axilar: 36.0-37.0°C
 * - Timpánica: 36.5-37.5°C
 * 
 * Nuestro simulador asume medición oral/axilar.
 * 
 * CONDICIONES ANORMALES:
 * - Hipotermia: <35.5°C (peligrosa <35°C)
 * - Febrícula: 37.5-38.0°C
 * - Fiebre: >38.0°C
 * - Fiebre alta: >39.0°C
 * - Hipertermia: >40.0°C (emergencia)
 * 
 * INTERACCIÓN CON EL SCHEDULER:
 * - Prioridad 2 (menor que HR y SpO2)
 * - Si HR o SpO2 están listas, se ejecutan primero
 * - El scheduler la ejecutará cuando no haya tareas de mayor prioridad
 * - El período largo (2000ms) significa que rara vez compite por CPU
 * - vTaskDelayUntil() garantiza muestreo periódico exacto
 */

/**
 * @brief Función de la tarea de Temperature
 * 
 * @param pvParameters Parámetros de inicialización (no usados)
 */
void vTaskTemperature(void *pvParameters) {
    (void)pvParameters;  /* Unused parameter */
    
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(PERIOD_TEMPERATURE_MS);
    TemperatureData_t tempData;
    
    /* Inicializar xLastWakeTime con el tiempo actual */
    xLastWakeTime = xTaskGetTickCount();
    
    /* Mensaje de inicio */
    safe_printf("[Temperature] Tarea iniciada\n");
    
    /* Loop principal de la tarea */
    for (;;) {
        /*
         * PASO 1: Leer sensor de temperatura
         * 
         * En un sistema real, esto involucraría:
         * - Configurar ADC para leer termistor/termopar
         * - Aplicar ecuación de Steinhart-Hart (para termistor):
         *   1/T = A + B*ln(R) + C*(ln(R))^3
         *   donde R es resistencia del termistor
         * - Convertir de Kelvin a Celsius
         * - Aplicar calibración específica del sensor
         * - Promediar múltiples lecturas para reducir ruido
         * 
         * Nuestro simulador genera valores realistas con:
         * - Rango normal: 36.0-37.5°C
         * - Ocasionalmente fiebre: 38.0-39.0°C
         * - Precisión: ±0.1°C (típica de termómetros digitales)
         */
        tempData.celsius = sensor_read_temperature();
        
        /*
         * PASO 2: Agregar timestamp
         * 
         * Aunque la temperatura cambia lentamente, el timestamp permite:
         * - Graficar tendencias a largo plazo
         * - Detectar fiebre en desarrollo (subida gradual)
         * - Correlacionar con eventos (medicación antitérmica)
         * - Validar que el dato es reciente
         */
        tempData.timestamp = xTaskGetTickCount();
        
        /*
         * PASO 3: Publicar dato en la cola
         * 
         * xQueueOverwrite() es ideal aquí porque:
         * - La temperatura cambia MUY lentamente
         * - Solo nos interesa el valor actual, no histórico
         * - Las tareas consumidoras usan xQueuePeek() (lectura no destructiva)
         * - Garantiza que siempre tenemos el dato más reciente
         * - No bloqueamos si nadie ha leído el valor anterior
         * 
         * Alternativa no usada:
         * - xQueueSend() con timeout: Podría bloquear, retrasando lecturas
         * - Buffer circular: Overhead innecesario para datos tan estables
         */
        xQueueOverwrite(xTemperatureQueue, &tempData);
        
        /*
         * PASO 4: Esperar hasta el próximo período
         * 
         * vTaskDelayUntil() mantiene periodicidad exacta de 2000ms.
         * 
         * Consideraciones de diseño:
         * 
         * ¿Por qué 2000ms y no más?
         * - Balance entre recursos y responsividad
         * - 2s permite detección de fiebre en desarrollo (aumento gradual)
         * - Suficiente para alarma oportuna sin desperdiciar CPU
         * 
         * ¿Por qué no menos?
         * - La temperatura no cambia significativamente en <2 segundos
         * - Períodos más cortos desperdiciarían CPU sin beneficio médico
         * - Los estándares médicos no requieren muestreo más frecuente
         * 
         * ¿Por qué vTaskDelayUntil() y no vTaskDelay()?
         * - Aunque la temperatura es estable, queremos:
         *   · Trends a largo plazo sin drift
         *   · Sincronización predecible con otras tareas
         *   · Cumplimiento con buenas prácticas de RTOS
         */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
    
    /* Esta línea nunca se alcanza (tarea infinita) */
}
