/*
 * FreeRTOS Configuration Header
 * 
 * Este archivo configura el comportamiento de FreeRTOS para el proyecto
 * de Monitor Médico en ARM Cortex-M3.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*
 * ============================================================================
 * CONFIGURACIÓN DEL HARDWARE
 * ============================================================================
 */

/*
 * Frecuencia del CPU en Hz
 * LM3S6965 en QEMU opera a 50 MHz
 */
#define configCPU_CLOCK_HZ                      ((unsigned long) 50000000)

/*
 * Frecuencia del tick del scheduler en Hz
 * 1000 Hz = tick cada 1 ms
 * Esto determina la resolución temporal mínima de vTaskDelay()
 */
#define configTICK_RATE_HZ                      ((TickType_t) 1000)

/*
 * Número máximo de prioridades disponibles
 * Rango: 0 (mínima) a 4 (máxima)
 * 
 * Nuestro sistema usa:
 * - Prioridad 4: Alarm (crítica)
 * - Prioridad 3: Heart Rate, SpO2
 * - Prioridad 2: Temperature
 * - Prioridad 1: Display
 * - Prioridad 0: Idle (automática de FreeRTOS)
 */
#define configMAX_PRIORITIES                    (5)

/*
 * Tamaño mínimo del stack para cada tarea (en words, no bytes)
 * 1 word = 4 bytes en ARM Cortex-M3
 * 128 words = 512 bytes
 */
#define configMINIMAL_STACK_SIZE                ((unsigned short) 128)

/*
 * Tamaño total del heap de FreeRTOS
 * Este heap se usa para:
 * - Crear tareas (xTaskCreate)
 * - Crear colas (xQueueCreate)
 * - Crear semáforos y mutexes
 * - pvPortMalloc()
 * 
 * 16KB es suficiente para nuestras 5 tareas y 3 colas
 */
#define configTOTAL_HEAP_SIZE                   ((size_t) (16 * 1024))

/*
 * Longitud máxima del nombre de una tarea
 * Usado para debugging y visualización
 */
#define configMAX_TASK_NAME_LEN                 (16)

/*
 * ============================================================================
 * FUNCIONALIDADES DE FREERTOS
 * ============================================================================
 */

/*
 * Usar tick count de 16 bits (0) o 32 bits (1)
 * 32 bits permite tiempos más largos antes de overflow
 */
#define configUSE_16_BIT_TICKS                  0

/*
 * Habilitar tarea Idle hook
 * Permite ejecutar código cuando no hay tareas listas
 */
#define configUSE_IDLE_HOOK                     0

/*
 * Habilitar tick hook
 * Permite ejecutar código en cada interrupción del tick
 */
#define configUSE_TICK_HOOK                     0

/*
 * Habilitar preemption (desalojo por prioridad)
 * DEBE estar en 1 para sistemas en tiempo real
 * Permite que tareas de mayor prioridad interrumpan tareas de menor prioridad
 */
#define configUSE_PREEMPTION                    1

/*
 * Habilitar mutexes
 * CRÍTICO para nuestro sistema - protege recursos compartidos
 */
#define configUSE_MUTEXES                       1

/*
 * Habilitar semáforos recursivos
 */
#define configUSE_RECURSIVE_MUTEXES             1

/*
 * Habilitar semáforos de conteo
 */
#define configUSE_COUNTING_SEMAPHORES           1

/*
 * Habilitar APIs alternativas de tareas
 */
#define configUSE_ALTERNATIVE_API               0

/*
 * Habilitar notificaciones directas a tareas
 * Mecanismo ligero de sincronización (más eficiente que semáforos)
 */
#define configUSE_TASK_NOTIFICATIONS            1

/*
 * ============================================================================
 * CONFIGURACIÓN DE MEMORIA Y CO-RUTINAS
 * ============================================================================
 */

/*
 * Habilitar co-rutinas
 * No las usamos en este proyecto
 */
#define configUSE_CO_ROUTINES                   0

/*
 * Máximo número de prioridades de co-rutinas
 */
#define configMAX_CO_ROUTINE_PRIORITIES         (2)

/*
 * Esquema de asignación de heap
 * heap_4.c: Permite free() y unifica bloques adyacentes libres
 */
#define configUSE_MALLOC_FAILED_HOOK            1

/*
 * ============================================================================
 * VERIFICACIÓN Y DEBUGGING
 * ============================================================================
 */

/*
 * Verificación de stack overflow
 * Método 2: Verifica patrón al final del stack en cada cambio de contexto
 * CRÍTICO para detectar bugs de desbordamiento de stack
 */
#define configCHECK_FOR_STACK_OVERFLOW          2

/*
 * Usar estadísticas de runtime
 * Permite medir tiempo de CPU usado por cada tarea
 */
#define configGENERATE_RUN_TIME_STATS           0

/*
 * Usar trace facility
 * Permite debugging avanzado del scheduler
 */
#define configUSE_TRACE_FACILITY                1

/*
 * Usar formato de lista de tareas
 */
#define configUSE_STATS_FORMATTING_FUNCTIONS    1

/*
 * Incluir funciones de query del estado de tareas
 */
#define configUSE_TASK_FPU_SUPPORT              0

/*
 * ============================================================================
 * CONFIGURACIÓN DE APIs OPCIONALES
 * ============================================================================
 */

/*
 * APIs de timer por software
 */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            (configMINIMAL_STACK_SIZE * 2)

/*
 * Incluir APIs de vTaskDelay
 */
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_vTaskDelayUntil                 1

/*
 * Incluir APIs de tareas
 */
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_eTaskGetState                   1

/*
 * ============================================================================
 * CONFIGURACIÓN DE INTERRUPCIONES PARA CORTEX-M
 * ============================================================================
 */

/*
 * Cortex-M tiene prioridades de interrupción configurables
 * Valores más bajos = mayor prioridad
 * 
 * El scheduler usa las interrupciones PendSV y SysTick
 */

/*
 * Número de bits de prioridad implementados en el hardware
 * LM3S6965 implementa 3 bits = 8 niveles de prioridad (0-7)
 */
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS                     __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS                     3
#endif

/*
 * Prioridad más baja que puede usar una interrupción que llama a API de FreeRTOS
 * Interrupciones con prioridad MENOR (mayor número) que esto no pueden
 * llamar funciones de FreeRTOS de forma segura
 * 
 * Valor: 5 (en escala de 8 niveles)
 * Interrupciones 0-4: NO pueden llamar API de FreeRTOS
 * Interrupciones 5-7: SÍ pueden llamar API de FreeRTOS
 */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         7

/*
 * Prioridad de la interrupción del kernel (SysTick y PendSV)
 * Debe ser la MÁS BAJA posible para no interferir con interrupciones críticas
 */
#define configKERNEL_INTERRUPT_PRIORITY                 (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/*
 * Máxima prioridad desde la cual se puede llamar a API de FreeRTOS
 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY            (5 << (8 - configPRIO_BITS))

/*
 * ============================================================================
 * MACROS DE ASSERT Y DEBUG
 * ============================================================================
 */

/*
 * Definir macro de assert para debugging
 * En producción, esto podría enviar información de debug por UART
 */
extern void vAssertCalled(const char *pcFile, unsigned long ulLine);
#define configASSERT(x)     if((x) == 0) vAssertCalled(__FILE__, __LINE__)

/*
 * ============================================================================
 * MAPEO DE HANDLERS DE INTERRUPCIÓN
 * ============================================================================
 * 
 * FreeRTOS requiere que ciertas interrupciones apunten a sus handlers
 * Estos deben coincidir con los nombres en startup.s
 */

/*
 * SVC (Supervisor Call) - usado para iniciar el scheduler
 */
#define vPortSVCHandler                         SVC_Handler

/*
 * PendSV - usado para cambios de contexto
 */
#define xPortPendSVHandler                      PendSV_Handler

/*
 * SysTick - genera el tick del scheduler
 */
#define xPortSysTickHandler                     SysTick_Handler

/*
 * ============================================================================
 * CONFIGURACIONES ESPECÍFICAS DEL PROYECTO
 * ============================================================================
 */

/*
 * Tamaño de las colas de datos médicos
 * Solo necesitamos guardar el último valor (tamaño 1)
 * porque usamos xQueuePeek() para lectura no destructiva
 */
#define configMEDICAL_QUEUE_SIZE                1

/*
 * Período de verificación de alarmas (en ms)
 * Alarm task se ejecuta cada 100ms
 */
#define configALARM_CHECK_PERIOD_MS             100

/*
 * Stack size para cada tipo de tarea (en words)
 */
#define configSENSOR_TASK_STACK_SIZE            (configMINIMAL_STACK_SIZE * 2)
#define configALARM_TASK_STACK_SIZE             (configMINIMAL_STACK_SIZE * 2)
#define configDISPLAY_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE * 3)

#endif /* FREERTOS_CONFIG_H */
