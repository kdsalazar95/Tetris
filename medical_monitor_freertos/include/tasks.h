/*
 * Tasks Header - Monitor Médico FreeRTOS
 * 
 * Declaraciones de todas las tareas del sistema, estructuras de datos
 * y recursos compartidos.
 */

#ifndef TASKS_H
#define TASKS_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/*
 * ============================================================================
 * ESTRUCTURAS DE DATOS MÉDICOS
 * ============================================================================
 */

/**
 * @brief Estructura para datos de frecuencia cardíaca
 * 
 * Contiene el valor en latidos por minuto (bpm) y timestamp
 */
typedef struct {
    float bpm;                  /* Frecuencia cardíaca en bpm */
    TickType_t timestamp;       /* Momento de la medición */
} HeartRateData_t;

/**
 * @brief Estructura para datos de saturación de oxígeno
 * 
 * Contiene el porcentaje de SpO2 y timestamp
 */
typedef struct {
    float percentage;           /* Saturación de O2 en % */
    TickType_t timestamp;       /* Momento de la medición */
} SpO2Data_t;

/**
 * @brief Estructura para datos de temperatura
 * 
 * Contiene la temperatura en grados Celsius y timestamp
 */
typedef struct {
    float celsius;              /* Temperatura en °C */
    TickType_t timestamp;       /* Momento de la medición */
} TemperatureData_t;

/**
 * @brief Tipo de alarma médica
 */
typedef enum {
    ALARM_NONE = 0,
    ALARM_TACHYCARDIA,          /* Frecuencia cardíaca alta */
    ALARM_BRADYCARDIA,          /* Frecuencia cardíaca baja */
    ALARM_HYPOXIA,              /* Saturación de oxígeno baja */
    ALARM_FEVER,                /* Temperatura alta */
    ALARM_HYPOTHERMIA           /* Temperatura baja */
} AlarmType_t;

/**
 * @brief Estructura de estado de alarma
 */
typedef struct {
    AlarmType_t type;           /* Tipo de alarma activa */
    uint8_t active;             /* 1 si la alarma está activa */
    char message[64];           /* Mensaje descriptivo de la alarma */
} AlarmState_t;

/*
 * ============================================================================
 * UMBRALES DE ALARMAS
 * ============================================================================
 */

/* Umbrales de frecuencia cardíaca (bpm) */
#define HR_THRESHOLD_HIGH           120.0f      /* Taquicardia si HR > 120 */
#define HR_THRESHOLD_LOW            50.0f       /* Bradicardia si HR < 50 */

/* Umbrales de saturación de oxígeno (%) */
#define SPO2_THRESHOLD_LOW          90.0f       /* Hipoxia si SpO2 < 90% */

/* Umbrales de temperatura (°C) */
#define TEMP_THRESHOLD_HIGH         38.0f       /* Fiebre si Temp > 38°C */
#define TEMP_THRESHOLD_LOW          35.5f       /* Hipotermia si Temp < 35.5°C */

/*
 * ============================================================================
 * PRIORIDADES DE TAREAS
 * ============================================================================
 * 
 * Las prioridades determinan el orden de ejecución cuando múltiples tareas
 * están listas. Mayor número = mayor prioridad.
 */

#define PRIORITY_DISPLAY            1           /* Prioridad baja - visualización */
#define PRIORITY_TEMPERATURE        2           /* Prioridad media-baja */
#define PRIORITY_HEART_RATE         3           /* Prioridad media-alta */
#define PRIORITY_SPO2               3           /* Prioridad media-alta */
#define PRIORITY_ALARM              4           /* Prioridad máxima - seguridad crítica */

/*
 * ============================================================================
 * PERÍODOS DE TAREAS (en milisegundos)
 * ============================================================================
 * 
 * Definen cada cuánto tiempo se ejecuta cada tarea periódica
 */

#define PERIOD_HEART_RATE_MS        500         /* Heart rate cada 500ms */
#define PERIOD_SPO2_MS              1000        /* SpO2 cada 1 segundo */
#define PERIOD_TEMPERATURE_MS       2000        /* Temperatura cada 2 segundos */
#define PERIOD_ALARM_MS             100         /* Alarm cada 100ms (rápida respuesta) */
#define PERIOD_DISPLAY_MS           1000        /* Display cada 1 segundo */

/*
 * ============================================================================
 * COLAS GLOBALES
 * ============================================================================
 * 
 * Estas colas permiten la comunicación entre tareas productoras (sensores)
 * y tareas consumidoras (Alarm y Display).
 * 
 * IMPORTANTE: Usamos xQueuePeek() en lugar de xQueueReceive() para permitir
 * que múltiples consumidores lean los mismos datos sin destruirlos.
 */

extern QueueHandle_t xHeartRateQueue;           /* Cola de datos de HR */
extern QueueHandle_t xSpO2Queue;                /* Cola de datos de SpO2 */
extern QueueHandle_t xTemperatureQueue;         /* Cola de datos de temperatura */

/*
 * ============================================================================
 * MUTEX PARA PROTECCIÓN DE UART
 * ============================================================================
 * 
 * Múltiples tareas escriben a UART (printf). El mutex previene que los
 * mensajes se intercalen y se corrompan.
 */

extern SemaphoreHandle_t xUARTMutex;

/*
 * ============================================================================
 * DECLARACIONES DE FUNCIONES DE TAREAS
 * ============================================================================
 */

/**
 * @brief Tarea de monitoreo de frecuencia cardíaca
 * 
 * Función:
 * - Lee sensor de HR cada 500ms
 * - Escribe dato en xHeartRateQueue
 * - Simula variación fisiológica realista
 * 
 * Prioridad: 3 (alta)
 * Período: 500ms
 * 
 * @param pvParameters Parámetros de inicialización (no usados)
 */
void vTaskHeartRate(void *pvParameters);

/**
 * @brief Tarea de monitoreo de saturación de oxígeno
 * 
 * Función:
 * - Lee sensor de SpO2 cada 1000ms
 * - Escribe dato en xSpO2Queue
 * - Simula variación fisiológica realista
 * 
 * Prioridad: 3 (alta)
 * Período: 1000ms
 * 
 * @param pvParameters Parámetros de inicialización (no usados)
 */
void vTaskSpO2(void *pvParameters);

/**
 * @brief Tarea de monitoreo de temperatura
 * 
 * Función:
 * - Lee sensor de temperatura cada 2000ms
 * - Escribe dato en xTemperatureQueue
 * - Simula variación fisiológica realista
 * 
 * Prioridad: 2 (media)
 * Período: 2000ms
 * 
 * @param pvParameters Parámetros de inicialización (no usados)
 */
void vTaskTemperature(void *pvParameters);

/**
 * @brief Tarea de sistema de alarmas
 * 
 * Función:
 * - Monitorea todas las colas cada 100ms usando xQueuePeek()
 * - Compara valores contra umbrales críticos
 * - Activa alarmas cuando se detectan condiciones anormales
 * - Imprime alertas en UART (protegido con mutex)
 * 
 * Prioridad: 4 (máxima - crítica para seguridad)
 * Período: 100ms
 * 
 * ¿Por qué xQueuePeek()?
 * - No consume los datos de las colas
 * - Permite que Display también los lea
 * - Previene pérdida de datos críticos
 * 
 * @param pvParameters Parámetros de inicialización (no usados)
 */
void vTaskAlarm(void *pvParameters);

/**
 * @brief Tarea de visualización
 * 
 * Función:
 * - Lee todas las colas cada 1000ms usando xQueuePeek()
 * - Formatea y muestra datos médicos en UART
 * - Proporciona interfaz de usuario del monitor
 * 
 * Prioridad: 1 (baja - no crítica)
 * Período: 1000ms
 * 
 * ¿Por qué xQueuePeek()?
 * - No consume los datos que Alarm necesita
 * - Evita problema de arquitectura multi-consumidor
 * - Display puede tolerar ver datos ligeramente antiguos
 * 
 * @param pvParameters Parámetros de inicialización (no usados)
 */
void vTaskDisplay(void *pvParameters);

/*
 * ============================================================================
 * FUNCIONES DE UTILIDAD
 * ============================================================================
 */

/**
 * @brief Imprime mensaje protegido por mutex
 * 
 * Adquiere el mutex de UART antes de imprimir y lo libera después.
 * Previene corrupción de mensajes cuando múltiples tareas imprimen.
 * 
 * @param format String de formato estilo printf
 * @param ... Argumentos variables
 */
void safe_printf(const char *format, ...);

/**
 * @brief Crea todas las colas del sistema
 * 
 * Inicializa:
 * - xHeartRateQueue
 * - xSpO2Queue
 * - xTemperatureQueue
 * 
 * @return pdPASS si todas las colas se crearon exitosamente, pdFAIL si no
 */
BaseType_t xCreateQueues(void);

/**
 * @brief Crea el mutex de UART
 * 
 * Inicializa xUARTMutex para protección de printf
 * 
 * @return pdPASS si el mutex se creó exitosamente, pdFAIL si no
 */
BaseType_t xCreateMutexes(void);

/**
 * @brief Crea todas las tareas del sistema
 * 
 * Inicializa las 5 tareas con sus prioridades y stacks correspondientes
 * 
 * @return pdPASS si todas las tareas se crearon exitosamente, pdFAIL si no
 */
BaseType_t xCreateTasks(void);

#endif /* TASKS_H */
