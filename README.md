# Tutorial Guiado: FreeRTOS vs Bare-Metal usando QEMU
## Duración: 30-45 minutos

---

## 📋 Información del Tutorial

**Objetivo:** Implementar un sistema simple de monitoreo de temperatura primero en bare-metal y luego convertirlo a FreeRTOS, comparando ambos enfoques de forma práctica.

**Duración estimada:** 30-45 minutos  
**Dificultad:** Intermedia  
**Prerequisitos:** 
- Conocimientos básicos de C
- Familiaridad con compilación en Linux
- Conceptos básicos de sistemas embebidos

---

## 🛠️ Materiales y Software Necesarios

### Software Requerido
```bash
# Verificar instalación antes de comenzar
gcc-arm-none-eabi --version    # Compilador ARM
qemu-system-arm --version       # Emulador QEMU
make --version                  # Sistema de build
```

### Archivos del Repositorio
```
mini-taller-freertos/
├── bare-metal/
│   ├── main.c              # ⚠️ Completaremos aquí
│   ├── Makefile            # ✅ Ya configurado
│   └── common/             # ✅ Drivers incluidos
├── freertos/
│   ├── main.c              # ⚠️ Completaremos aquí
│   ├── tasks/              # ⚠️ Completaremos aquí
│   ├── Makefile            # ✅ Ya configurado
│   └── FreeRTOS/           # ✅ Kernel incluido
└── scripts/
    ├── run-baremetal.sh    # Script de ejecución
    └── run-freertos.sh     # Script de ejecución
```

### Hardware Emulado
- **MCU:** ARM Cortex-M3 (LM3S6965EVB en QEMU)
- **Memoria:** 64KB RAM, 256KB Flash
- **Periféricos:** UART0 (terminal virtual)

---

## 🎯 Parte 1: Implementación Bare-Metal (15 minutos)

### Contexto
En bare-metal, todo se ejecuta en un solo loop infinito. Los delays bloquean todo el sistema.

### Paso 1: Estructura del Main Loop (5 min)

Abrir `bare-metal/main.c` y completar el loop principal:

```c
// bare-metal/main.c

#include <stdint.h>
#include "uart.h"
#include "sensor_sim.h"

// Función de delay bloqueante
void delay_ms(uint32_t ms) {
    // Busy waiting - BLOQUEA TODO
    for(volatile uint32_t i = 0; i < ms * 1000; i++);
}

int main(void) {
    // Inicialización
    uart_init();
    sensor_init();
    
    uart_puts("=== Sistema Bare-Metal Iniciado ===\r\n");
    
    uint32_t contador = 0;
    
    // Loop infinito principal
    while(1) {
        // TAREA 1: Leer sensor (cada iteración)
        float temperatura = sensor_read();
        uart_printf("[%lu] Temperatura: %.1f C\r\n", contador, temperatura);
        
        // TAREA 2: Control de LED basado en temperatura
        if(temperatura > 30.0) {
            uart_puts("    LED: ON (temperatura alta)\r\n");
        } else {
            uart_puts("    LED: OFF\r\n");
        }
        
        // TAREA 3: Logger periódico (cada 3 iteraciones)
        if(contador % 3 == 0) {
            uart_printf("    [LOG] Sistema activo - Lecturas: %lu\r\n", contador);
        }
        
        contador++;
        
        // ⚠️ DELAY BLOQUEANTE - Todo se detiene aquí
        delay_ms(1000);  // 1 segundo
    }
    
    return 0;
}
```

**Puntos clave a notar:**
- ✅ Código simple y secuencial
- ❌ `delay_ms()` bloquea TODO el sistema
- ❌ Difícil agregar nuevas tareas sin romper el timing
- ❌ CPU al 100% todo el tiempo (busy-waiting)

### Paso 2: Compilar y Ejecutar (3 min)

```bash
# Navegar al directorio
cd bare-metal

# Compilar
make clean
make

# Ejecutar en QEMU
../scripts/run-baremetal.sh

# Para salir de QEMU: Ctrl+A, luego X
```

**Salida esperada:**
```
=== Sistema Bare-Metal Iniciado ===
[0] Temperatura: 25.3 C
    LED: OFF
    [LOG] Sistema activo - Lecturas: 0
[1] Temperatura: 26.1 C
    LED: OFF
[2] Temperatura: 27.8 C
    LED: OFF
[3] Temperatura: 28.5 C
    LED: OFF
    [LOG] Sistema activo - Lecturas: 3
...
```

### Paso 3: Experimento - Problema de Concurrencia (2 min)

**Pregunta guiada:** *"¿Qué pasa si queremos agregar una tarea que parpadee un LED cada 200ms?"*

**Intento (NO lo implementen, solo analicen):**
```c
while(1) {
    // ... código anterior ...
    
    // Nueva tarea: parpadeo rápido
    led2_toggle();
    delay_ms(200);  // ⚠️ Esto rompe el timing de todo
}
```

**Resultado:** El sensor ya NO se lee cada 1 segundo. Todo se desincroniza.

**Conclusión:** En bare-metal, agregar tareas concurrentes es problemático.

### ✅ Checkpoint #1 (5 min)
- [ ] ¿Compila sin errores?
- [ ] ¿Ves la salida en QEMU?
- [ ] ¿Entiendes por qué el delay es bloqueante?
- [ ] ¿Notas la limitación para multitarea?

---

## 🎯 Parte 2: Conversión a FreeRTOS (25 minutos)

### Contexto
FreeRTOS divide el trabajo en **tasks independientes**. Cada task puede bloquearse sin afectar a las demás.

### Paso 4: Arquitectura de Tasks (2 min)

Vamos a crear 3 tasks independientes:

```
┌─────────────────────────────────────────┐
│  Task Sensor (Prioridad 3 - Alta)      │
│  Lee sensor cada 1 segundo             │
│  Envía datos a Queue                   │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│  Task Logger (Prioridad 2 - Media)     │
│  Recibe datos de Queue                 │
│  Imprime logs periódicos               │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│  Task LED (Prioridad 1 - Baja)         │
│  Lee temperatura de Queue              │
│  Controla LED según umbral             │
└─────────────────────────────────────────┘
```

### Paso 5: Implementar Task del Sensor (6 min)

Abrir `freertos/tasks/sensor_task.c`:

```c
// freertos/tasks/sensor_task.c

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "sensor_task.h"
#include "sensor_sim.h"
#include "uart.h"

// Queue externa (declarada en main.c)
extern QueueHandle_t xTemperaturaQueue;

void vTaskSensor(void *pvParameters) {
    float temperatura;
    
    while(1) {
        // Leer sensor
        temperatura = sensor_read();
        
        // Enviar a queue (no bloqueante)
        if(xQueueSend(xTemperaturaQueue, &temperatura, 0) == pdPASS) {
            uart_printf("[SENSOR] Enviado: %.1f C\r\n", temperatura);
        } else {
            uart_puts("[SENSOR] ERROR: Queue llena\r\n");
        }
        
        // ⚠️ DELAY NO BLOQUEANTE - Libera CPU para otras tasks
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 segundo
        
        // Durante el delay, otras tasks pueden ejecutarse
    }
}
```

**Diferencia clave:** `vTaskDelay()` pone esta task en estado BLOCKED y libera el CPU para otras tasks.

### Paso 6: Implementar Task del Logger (6 min)

Abrir `freertos/tasks/logger_task.c`:

```c
// freertos/tasks/logger_task.c

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "logger_task.h"
#include "uart.h"

extern QueueHandle_t xTemperaturaQueue;

void vTaskLogger(void *pvParameters) {
    float temp_recibida;
    uint32_t contador = 0;
    
    while(1) {
        // Recibir de queue (bloqueante con timeout)
        if(xQueueReceive(xTemperaturaQueue, &temp_recibida, 
                         pdMS_TO_TICKS(100)) == pdPASS) {
            
            // Procesar dato recibido
            uart_printf("[LOGGER] Temp: %.1f C\r\n", temp_recibida);
            contador++;
            
            // Log periódico cada 3 lecturas
            if(contador % 3 == 0) {
                uart_printf("[LOGGER] Total lecturas: %lu\r\n", contador);
            }
        }
        
        // Esta task se ejecuta solo cuando hay datos
        // No consume CPU innecesariamente
    }
}
```

### Paso 7: Implementar Task del LED (5 min)

Abrir `freertos/tasks/led_task.c`:

```c
// freertos/tasks/led_task.c

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "led_task.h"
#include "uart.h"

extern QueueHandle_t xTemperaturaQueue;

void vTaskLED(void *pvParameters) {
    float temp_actual;
    
    while(1) {
        // Leer sin consumir de la queue (peek)
        if(xQueuePeek(xTemperaturaQueue, &temp_actual, 
                      pdMS_TO_TICKS(100)) == pdPASS) {
            
            // Control del LED
            if(temp_actual > 30.0) {
                uart_puts("[LED] ON (temp alta)\r\n");
            } else {
                uart_puts("[LED] OFF\r\n");
            }
        }
        
        // Chequear cada 500ms
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

### Paso 8: Configurar el Main (4 min)

Abrir `freertos/main.c`:

```c
// freertos/main.c

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "uart.h"
#include "sensor_sim.h"
#include "tasks/sensor_task.h"
#include "tasks/logger_task.h"
#include "tasks/led_task.h"

// Queue global para comunicación entre tasks
QueueHandle_t xTemperaturaQueue;

int main(void) {
    // Inicialización hardware
    uart_init();
    sensor_init();
    
    uart_puts("=== Sistema FreeRTOS Iniciado ===\r\n");
    
    // Crear Queue (5 elementos, tamaño float)
    xTemperaturaQueue = xQueueCreate(5, sizeof(float));
    
    if(xTemperaturaQueue == NULL) {
        uart_puts("ERROR: No se pudo crear Queue\r\n");
        while(1);  // Error fatal
    }
    
    // Crear Tasks con diferentes prioridades
    xTaskCreate(vTaskSensor,  // Función
                "Sensor",     // Nombre
                256,          // Stack size (words)
                NULL,         // Parámetros
                3,            // Prioridad ALTA
                NULL);        // Handle
    
    xTaskCreate(vTaskLogger, "Logger", 256, NULL, 2, NULL);  // Media
    xTaskCreate(vTaskLED,    "LED",    256, NULL, 1, NULL);  // Baja
    
    uart_puts("Tasks creadas, iniciando scheduler...\r\n");
    
    // Iniciar Scheduler (NUNCA retorna)
    vTaskStartScheduler();
    
    // No debería llegar aquí
    uart_puts("ERROR: Scheduler falló\r\n");
    while(1);
    
    return 0;
}
```

**Puntos clave:**
- Queue de 5 elementos para comunicación thread-safe
- 3 tasks con prioridades diferentes
- El scheduler decide automáticamente qué task ejecutar

### Paso 9: Compilar y Ejecutar (2 min)

```bash
# Navegar al directorio
cd ../freertos

# Compilar
make clean
make

# Ejecutar en QEMU
../scripts/run-freertos.sh
```

**Salida esperada:**
```
=== Sistema FreeRTOS Iniciado ===
Tasks creadas, iniciando scheduler...
[SENSOR] Enviado: 25.1 C
[LOGGER] Temp: 25.1 C
[LED] OFF
[SENSOR] Enviado: 26.3 C
[LOGGER] Temp: 26.3 C
[LED] OFF
[SENSOR] Enviado: 27.5 C
[LOGGER] Temp: 27.5 C
[LOGGER] Total lecturas: 3
[LED] OFF
[SENSOR] Enviado: 31.2 C
[LOGGER] Temp: 31.2 C
[LED] ON (temp alta)
...
```

### ✅ Checkpoint #2 (Tiempo restante ~5 min)
- [ ] ¿Compila FreeRTOS sin errores?
- [ ] ¿Las 3 tasks se ejecutan correctamente?
- [ ] ¿La queue funciona (datos se transfieren)?
- [ ] ¿El LED responde a la temperatura?
- [ ] ¿Notas que el sistema es más modular?

---

## 📊 Comparación Rápida

### Tabla Comparativa

| Aspecto | Bare-Metal | FreeRTOS |
|---------|-----------|----------|
| **Estructura** | 1 loop infinito | 3 tasks independientes |
| **Delays** | Bloqueantes (busy-wait) | No bloqueantes (yield CPU) |
| **Agregar tareas** | Difícil, rompe timing | Fácil, solo crear task |
| **Comunicación** | Variables globales | Queue thread-safe |
| **Uso de CPU** | 100% siempre | Solo cuando hay trabajo |
| **Complejidad código** | Baja | Media |
| **Escalabilidad** | Limitada | Alta |

### Diagrama de Ejecución

**Bare-Metal:**
```
Main Loop: [Sensor][Delay][LED][Delay][Logger][Delay][Sensor]...
           └─────── Todo secuencial, delays bloquean ──────┘
```

**FreeRTOS:**
```
Sensor Task:  │Work│───BLOCKED───│Work│───BLOCKED───│
Logger Task:  ───│Work│──BLOCKED────│Work│──────────│
LED Task:     ─────│Work│───BLOCKED────│Work│───────│
Idle Task:    │idle│idle│idle│idle│idle│idle│idle│idle│
              ↑ Cambios de contexto del scheduler
```

---

## 🐛 Troubleshooting - Errores Comunes

### Error 1: No compila - "undefined reference to vTaskDelay"
**Causa:** No se enlazó correctamente FreeRTOS  
**Solución:** Verificar que el Makefile incluya todos los archivos .c de FreeRTOS

### Error 2: Sistema se cuelga inmediatamente
**Causa:** Stack overflow en alguna task  
**Solución:** Aumentar tamaño de stack en `xTaskCreate()`
```c
xTaskCreate(vTask, "Task", 512, NULL, 1, NULL);  // Era 256, ahora 512
```

### Error 3: Queue siempre vacía
**Causa:** Task sensor no está enviando o logger lee muy rápido  
**Solución:** Verificar retorno de `xQueueSend()` y agregar prints de debug

### Error 4: Salida desordenada en UART
**Causa:** Múltiples tasks escribiendo simultáneamente (race condition)  
**Solución:** En un sistema real, usar mutex para proteger UART. Por ahora es normal ver entrecruzamiento ocasional.

---

## 🎯 Resultados Esperados

Al finalizar este tutorial habrás:

✅ Implementado un sistema bare-metal funcional  
✅ Identificado las limitaciones del polling y delays bloqueantes  
✅ Convertido el sistema a arquitectura de tasks con FreeRTOS  
✅ Usado queues para comunicación thread-safe  
✅ Comprendido delays bloqueantes vs no bloqueantes  
✅ Comparado ambos enfoques prácticamente  

---

## 🏆 Reto Práctico: Agregar Tarea de Parpadeo

### Objetivo del Reto

Demostrar la ventaja de FreeRTOS agregando una nueva funcionalidad que sería muy difícil en bare-metal.
Descripción
Agregar una 4ta task que parpadee un LED2 cada 200ms, sin modificar ni afectar las 3 tasks existentes.
---
### 🔴 Reto en Bare-Metal: ¿Por qué es difícil?
Intenta mentalmente agregar esto al código bare-metal:
```bash
while(1) {
    // Sensor (1000ms)
    temperatura = sensor_read();
    uart_printf("Temp: %.1f\r\n", temperatura);
    
    // LED control
    if(temperatura > 30.0) { /* ... */ }
    
    // Logger cada 3 iteraciones
    if(contador % 3 == 0) { /* ... */ }
    
    // ⚠️ NUEVO: Parpadeo cada 200ms
    led2_toggle();
    delay_ms(200);  // ← Esto ROMPE todo el timing
    
    // Ya NO es 1 segundo entre lecturas de sensor
}
```
__Problema:__ El delay de 200ms hace que el sensor se lea cada 200ms en vez de 1000ms. Para mantener ambos timings necesitarías:

- Variables de conteo manual
- Lógica condicional compleja
- Mucho código difícil de mantener

### Reto en FreeRTOS: ¡Simple!

__Paso 1:__ Crear nueva tarea en freertos/tasks/blink_task.c

```bash
// freertos/tasks/blink_task.c

#include "FreeRTOS.h"
#include "task.h"
#include "blink_task.h"
#include "uart.h"

void vTaskBlink(void *pvParameters) {
    uint8_t estado = 0;
    
    while(1) {
        // Alternar LED2
        if(estado) {
            uart_puts("[BLINK] LED2: ON\r\n");
        } else {
            uart_puts("[BLINK] LED2: OFF\r\n");
        }
        estado = !estado;
        
        // Delay de 200ms - NO afecta otras tasks
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
```
__Paso 2:__  Agregar al ``main.c``

```bash
// En freertos/main.c

#include "tasks/blink_task.h"  // Agregar include

int main(void) {
    // ... código existente ...
    
    // Crear las 3 tasks originales
    xTaskCreate(vTaskSensor, "Sensor", 256, NULL, 3, NULL);
    xTaskCreate(vTaskLogger, "Logger", 256, NULL, 2, NULL);
    xTaskCreate(vTaskLED,    "LED",    256, NULL, 1, NULL);
    
    // ✅ NUEVA TAREA - Una sola línea
    xTaskCreate(vTaskBlink,  "Blink",  128, NULL, 1, NULL);
    
    // Iniciar scheduler
    vTaskStartScheduler();
    
    // ...
}
```
¡Eso es todo! Las otras 3 tasks siguen funcionando exactamente igual.

### 📊 Resultado Esperado

```bash
[SENSOR] Enviado: 25.1 C
[LOGGER] Temp: 25.1 C
[LED] OFF
[BLINK] LED2: ON
[BLINK] LED2: OFF
[BLINK] LED2: ON
[BLINK] LED2: OFF
[SENSOR] Enviado: 26.3 C    ← Sigue siendo cada 1 seg
[LOGGER] Temp: 26.3 C
[BLINK] LED2: ON
[LED] OFF
...

```

Observa que:

- El sensor sigue leyendo cada 1 segundo
- El logger funciona normalmente
- El LED responde a temperatura
- El parpadeo ocurre cada 200ms independientemente

### 💭 Reflexión Final

**Esta es la ventaja clave de FreeRTOS:**

| Bare-Metal | FreeRTOS |
|-----------|----------|
| Agregar función = reescribir lógica completa | Agregar función = nueva task (4 líneas) |
| Mantener timings = código complejo | Scheduler maneja automáticamente |
| Escalar = cada vez más difícil | Escalar = crear más tasks |

**Pregunta para los participantes:**
> *"¿Qué pasaría si ahora queremos agregar una 5ta task? ¿Y una 6ta?"*

**Respuesta:** En FreeRTOS, ¡solo crear más tasks! En bare-metal, el código se volvería inmanejable.
---

## 📚 Recursos Adicionales

### Documentación
- [FreeRTOS.org](https://www.freertos.org/) - Documentación oficial
- [QEMU ARM Docs](https://www.qemu.org/docs/master/system/target-arm.html)

### Libros Recomendados
- "Mastering the FreeRTOS Real Time Kernel" - Gratuito en FreeRTOS.org
- "The Definitive Guide to ARM Cortex-M" - Joseph Yiu

### Para Profundizar
- Semáforos y mutexes
- Software timers
- Task notifications
- Memory management (heap_4, heap_5)
- Prioridad invertida y priority inheritance

---

## 🎓 Conclusión

**¿Cuándo usar Bare-Metal?**
- Aplicaciones muy simples
- Recursos extremadamente limitados
- Timing ultra-crítico (microsegundos)

**¿Cuándo usar FreeRTOS?**
- Múltiples tareas concurrentes
- Sistema que crecerá en complejidad
- Necesidad de respuesta rápida a eventos
- Desarrollo modular en equipo

**Lección clave:** FreeRTOS agrega ~5KB de overhead pero facilita enormemente el desarrollo de sistemas complejos y escalables.

---

**Fin del Tutorial Guiado**  
**Duración total:** 30-45 minutos  
**¡Felicitaciones por completar el tutorial!**
