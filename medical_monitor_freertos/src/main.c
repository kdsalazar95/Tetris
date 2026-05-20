/*
 * Main - Monitor Médico con FreeRTOS
 * 
 * Punto de entrada del sistema. Inicializa hardware, crea recursos de RTOS
 * (colas, mutexes, tareas) y lanza el scheduler.
 */

#include <stdio.h>
#include <stdarg.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "tasks.h"
#include "sensor_simulator.h"

/*
 * ============================================================================
 * VARIABLES GLOBALES - RECURSOS COMPARTIDOS
 * ============================================================================
 */

/* Colas para comunicación entre tareas */
QueueHandle_t xHeartRateQueue = NULL;
QueueHandle_t xSpO2Queue = NULL;
QueueHandle_t xTemperatureQueue = NULL;

/* Mutex para protección de UART */
SemaphoreHandle_t xUARTMutex = NULL;

/*
 * ============================================================================
 * CONFIGURACIÓN DEL HARDWARE (QEMU LM3S6965)
 * ============================================================================
 */

/* Dirección del UART0 en LM3S6965 */
#define UART0_BASE              0x4000C000
#define UART0_DR                (*(volatile unsigned int *)(UART0_BASE + 0x000))
#define UART0_FR                (*(volatile unsigned int *)(UART0_BASE + 0x018))
#define UART0_FR_TXFF           (1 << 5)        /* TX FIFO Full flag */

/**
 * @brief Inicialización básica del sistema
 * 
 * En QEMU, el hardware ya está configurado. Esta función es un placeholder
 * para configuración adicional si se necesita.
 */
void SystemInit(void) {
    /* QEMU ya inicializa el UART0 */
    /* En hardware real, aquí se configurarían:
     * - Relojes del sistema
     * - Periféricos (UART, GPIO, ADC, etc.)
     * - Interrupciones
     */
}

/**
 * @brief Envía un carácter por UART0
 * 
 * Función de bajo nivel para output serial. Usada por printf.
 * 
 * @param c Carácter a enviar
 */
void uart_putc(char c) {
    /* Esperar si el FIFO de transmisión está lleno */
    while (UART0_FR & UART0_FR_TXFF);
    
    /* Escribir carácter al registro de datos */
    UART0_DR = c;
}

/**
 * @brief Implementación de _write para redirección de printf
 * 
 * Esta función es llamada por la biblioteca C cuando se usa printf.
 * Redirige la salida al UART0.
 * 
 * @param file Descriptor de archivo (ignorado)
 * @param ptr Buffer con datos a escribir
 * @param len Longitud del buffer
 * @return Número de bytes escritos
 */
int _write(int file, char *ptr, int len) {
    int i;
    (void)file;  /* Unused parameter */
    
    for (i = 0; i < len; i++) {
        /* Convertir LF a CR+LF para terminales */
        if (ptr[i] == '\n') {
            uart_putc('\r');
        }
        uart_putc(ptr[i]);
    }
    
    return len;
}

/*
 * ============================================================================
 * FUNCIONES DE UTILIDAD
 * ============================================================================
 */

/**
 * @brief Printf protegido por mutex
 * 
 * Adquiere el mutex de UART antes de imprimir para prevenir que múltiples
 * tareas intercalen sus mensajes.
 * 
 * @param format String de formato estilo printf
 * @param ... Argumentos variables
 */
void safe_printf(const char *format, ...) {
    va_list args;
    
    /* Adquirir mutex (esperar indefinidamente si está ocupado) */
    if (xUARTMutex != NULL) {
        xSemaphoreTake(xUARTMutex, portMAX_DELAY);
    }
    
    /* Imprimir con formato */
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    /* Liberar mutex */
    if (xUARTMutex != NULL) {
        xSemaphoreGive(xUARTMutex);
    }
}

/*
 * ============================================================================
 * CREACIÓN DE RECURSOS DE FREERTOS
 * ============================================================================
 */

/**
 * @brief Crea todas las colas del sistema
 * 
 * Inicializa las colas de comunicación entre tareas productoras (sensores)
 * y consumidoras (Alarm y Display).
 * 
 * Tamaño de colas = 1:
 * - Solo necesitamos el último valor
 * - Usamos xQueuePeek() para lectura no destructiva
 * - Múltiples consumidores pueden leer el mismo valor
 * 
 * @return pdPASS si todas las colas se crearon correctamente
 */
BaseType_t xCreateQueues(void) {
    /* Crear cola de Heart Rate */
    xHeartRateQueue = xQueueCreate(1, sizeof(HeartRateData_t));
    if (xHeartRateQueue == NULL) {
        printf("ERROR: No se pudo crear xHeartRateQueue\n");
        return pdFAIL;
    }
    
    /* Crear cola de SpO2 */
    xSpO2Queue = xQueueCreate(1, sizeof(SpO2Data_t));
    if (xSpO2Queue == NULL) {
        printf("ERROR: No se pudo crear xSpO2Queue\n");
        return pdFAIL;
    }
    
    /* Crear cola de Temperature */
    xTemperatureQueue = xQueueCreate(1, sizeof(TemperatureData_t));
    if (xTemperatureQueue == NULL) {
        printf("ERROR: No se pudo crear xTemperatureQueue\n");
        return pdFAIL;
    }
    
    return pdPASS;
}

/**
 * @brief Crea el mutex de UART
 * 
 * El mutex protege el acceso al UART0 para prevenir corrupción de mensajes
 * cuando múltiples tareas intentan imprimir simultáneamente.
 * 
 * @return pdPASS si el mutex se creó correctamente
 */
BaseType_t xCreateMutexes(void) {
    /* Crear mutex para protección de UART */
    xUARTMutex = xSemaphoreCreateMutex();
    if (xUARTMutex == NULL) {
        printf("ERROR: No se pudo crear xUARTMutex\n");
        return pdFAIL;
    }
    
    return pdPASS;
}

/**
 * @brief Crea todas las tareas del sistema
 * 
 * Inicializa las 5 tareas del monitor médico con sus prioridades y
 * configuraciones específicas.
 * 
 * @return pdPASS si todas las tareas se crearon correctamente
 */
BaseType_t xCreateTasks(void) {
    BaseType_t xStatus;
    
    /* Crear tarea de Heart Rate */
    xStatus = xTaskCreate(
        vTaskHeartRate,                     /* Función de la tarea */
        "HeartRate",                        /* Nombre descriptivo */
        configSENSOR_TASK_STACK_SIZE,       /* Stack size */
        NULL,                               /* Parámetros */
        PRIORITY_HEART_RATE,                /* Prioridad */
        NULL                                /* Handle (no necesario) */
    );
    if (xStatus != pdPASS) {
        printf("ERROR: No se pudo crear tarea HeartRate\n");
        return pdFAIL;
    }
    
    /* Crear tarea de SpO2 */
    xStatus = xTaskCreate(
        vTaskSpO2,
        "SpO2",
        configSENSOR_TASK_STACK_SIZE,
        NULL,
        PRIORITY_SPO2,
        NULL
    );
    if (xStatus != pdPASS) {
        printf("ERROR: No se pudo crear tarea SpO2\n");
        return pdFAIL;
    }
    
    /* Crear tarea de Temperature */
    xStatus = xTaskCreate(
        vTaskTemperature,
        "Temperature",
        configSENSOR_TASK_STACK_SIZE,
        NULL,
        PRIORITY_TEMPERATURE,
        NULL
    );
    if (xStatus != pdPASS) {
        printf("ERROR: No se pudo crear tarea Temperature\n");
        return pdFAIL;
    }
    
    /* Crear tarea de Alarm */
    xStatus = xTaskCreate(
        vTaskAlarm,
        "Alarm",
        configALARM_TASK_STACK_SIZE,
        NULL,
        PRIORITY_ALARM,
        NULL
    );
    if (xStatus != pdPASS) {
        printf("ERROR: No se pudo crear tarea Alarm\n");
        return pdFAIL;
    }
    
    /* Crear tarea de Display */
    xStatus = xTaskCreate(
        vTaskDisplay,
        "Display",
        configDISPLAY_TASK_STACK_SIZE,
        NULL,
        PRIORITY_DISPLAY,
        NULL
    );
    if (xStatus != pdPASS) {
        printf("ERROR: No se pudo crear tarea Display\n");
        return pdFAIL;
    }
    
    return pdPASS;
}

/*
 * ============================================================================
 * HOOKS DE FREERTOS
 * ============================================================================
 */

/**
 * @brief Hook llamado cuando malloc falla
 * 
 * FreeRTOS llama a esta función si pvPortMalloc() no puede asignar memoria.
 * Típicamente ocurre si configTOTAL_HEAP_SIZE es muy pequeño.
 */
void vApplicationMallocFailedHook(void) {
    printf("\n!!! ERROR CRÍTICO: Malloc failed - heap insuficiente !!!\n");
    printf("Aumentar configTOTAL_HEAP_SIZE en FreeRTOSConfig.h\n");
    
    /* Halt el sistema */
    taskDISABLE_INTERRUPTS();
    while(1);
}

/**
 * @brief Hook llamado cuando se detecta stack overflow
 * 
 * FreeRTOS llama a esta función cuando detecta desbordamiento de stack
 * en una tarea (si configCHECK_FOR_STACK_OVERFLOW está habilitado).
 * 
 * @param xTask Handle de la tarea que desbordó
 * @param pcTaskName Nombre de la tarea
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;  /* Unused parameter */
    
    printf("\n!!! ERROR CRÍTICO: Stack overflow en tarea: %s !!!\n", pcTaskName);
    printf("Aumentar stack size de la tarea en tasks.h\n");
    
    /* Halt el sistema */
    taskDISABLE_INTERRUPTS();
    while(1);
}

/**
 * @brief Hook para función assert
 * 
 * Llamada cuando configASSERT() falla. Útil para debugging.
 * 
 * @param pcFile Archivo donde ocurrió el assert
 * @param ulLine Línea donde ocurrió el assert
 */
void vAssertCalled(const char *pcFile, unsigned long ulLine) {
    printf("\n!!! ASSERT FAILED !!!\n");
    printf("File: %s\n", pcFile);
    printf("Line: %lu\n", ulLine);
    
    taskDISABLE_INTERRUPTS();
    while(1);
}

/*
 * ============================================================================
 * MAIN - PUNTO DE ENTRADA
 * ============================================================================
 */

/**
 * @brief Función principal del sistema
 * 
 * Secuencia de inicialización:
 * 1. Imprimir banner de inicio
 * 2. Inicializar simulador de sensores
 * 3. Crear colas de comunicación
 * 4. Crear mutexes de sincronización
 * 5. Crear todas las tareas
 * 6. Iniciar el scheduler de FreeRTOS
 * 
 * Una vez iniciado el scheduler, main() nunca retorna.
 * El control pasa al scheduler que gestiona las tareas.
 * 
 * @return Nunca retorna (scheduler toma el control)
 */
int main(void) {
    /* Banner de inicio */
    printf("\n");
    printf("===================================\n");
    printf("  Monitor Medico FreeRTOS v1.0\n");
    printf("  ARM Cortex-M3 en QEMU\n");
    printf("===================================\n");
    printf("\n");
    
    /* Inicializar simulador de sensores */
    printf("Inicializando sistema...\n");
    sensor_simulator_init();
    
    /* Crear recursos de FreeRTOS */
    printf("Creando colas...\n");
    if (xCreateQueues() != pdPASS) {
        printf("ERROR FATAL: Fallo al crear colas\n");
        while(1);
    }
    
    printf("Creando mutexes...\n");
    if (xCreateMutexes() != pdPASS) {
        printf("ERROR FATAL: Fallo al crear mutexes\n");
        while(1);
    }
    
    printf("Creando tareas...\n");
    if (xCreateTasks() != pdPASS) {
        printf("ERROR FATAL: Fallo al crear tareas\n");
        while(1);
    }
    
    printf("\n");
    printf("Sistema inicializado correctamente\n");
    printf("Iniciando scheduler...\n");
    printf("\n");
    
    /* Iniciar el scheduler de FreeRTOS */
    /* Esta función NO retorna - el scheduler toma el control */
    vTaskStartScheduler();
    
    /* Si llegamos aquí, hubo un error al iniciar el scheduler */
    printf("\n!!! ERROR CRÍTICO: Scheduler no pudo iniciar !!!\n");
    printf("Posibles causas:\n");
    printf("- configTOTAL_HEAP_SIZE muy pequeño\n");
    printf("- Error en configuración de interrupciones\n");
    
    /* Loop infinito */
    while(1);
    
    return 0;
}
