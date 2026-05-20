# Monitor Médico con FreeRTOS en QEMU

## Descripción del Proyecto

Este es un sistema embebido en tiempo real que simula un monitor médico completo, ejecutándose sobre ARM Cortex-M3 en QEMU. El proyecto implementa monitorización de signos vitales usando FreeRTOS como sistema operativo en tiempo real.

### Características Principales

- **Plataforma**: ARM Cortex-M3 emulado en QEMU
- **RTOS**: FreeRTOS
- **Arquitectura**: Multi-tarea con comunicación mediante colas y sincronización con mutexes
- **Monitorización**: Heart Rate (HR), Saturación de Oxígeno (SpO2), Temperatura
- **Sistema de Alarmas**: Detección automática de condiciones críticas
- **Display**: Visualización continua de datos médicos

## Arquitectura del Sistema

### Tareas Implementadas

| Tarea | Prioridad | Período | Función |
|-------|-----------|---------|---------|
| Heart Rate | 3 | 500 ms | Monitorea frecuencia cardíaca |
| SpO2 | 3 | 1000 ms | Monitorea saturación de oxígeno |
| Temperature | 2 | 2000 ms | Monitorea temperatura corporal |
| Alarm | 4 | 100 ms | Sistema de alertas (prioridad máxima) |
| Display | 1 | 1000 ms | Visualización de datos |

### Prioridades y Justificación

**Alarm (Prioridad 4 - Máxima)**: Debe responder inmediatamente a condiciones críticas. Un retraso en la detección de taquicardia o hipoxia puede ser fatal.

**Heart Rate y SpO2 (Prioridad 3)**: Son signos vitales críticos que requieren monitorización frecuente. Cambios rápidos en estos valores pueden indicar emergencias.

**Temperature (Prioridad 2)**: Aunque importante, la temperatura corporal cambia más lentamente y no requiere la misma urgencia de respuesta.

**Display (Prioridad 1 - Mínima)**: La visualización es importante para el usuario pero no crítica para la seguridad del paciente. Puede tolerar pequeños retrasos sin consecuencias.

### Comunicación Entre Tareas

#### Arquitectura de Colas

```
[Heart Rate Task] ──┐
                     ├──> [Cola HR] ──┬──> [Alarm Task] (xQueuePeek)
                     │                 │
[SpO2 Task] ────────┼──> [Cola SpO2] ─┼──> [Display Task] (xQueuePeek)
                     │                 │
[Temperature Task] ──┴──> [Cola Temp] ─┘
```

#### Problema de Arquitectura Productor-Consumidor

**IMPORTANTE**: Este proyecto usa `xQueuePeek()` en lugar de `xQueueReceive()` en las tareas Display y Alarm.

**¿Por qué?**

- **Display** y **Alarm** son ambos consumidores de los mismos datos
- Si Display usa `xQueueReceive()`, **consume** (elimina) el dato de la cola
- Alarm nunca vería ese dato → **pérdida de datos críticos**
- Esto NO es un race condition clásico, sino un **problema de arquitectura multi-consumidor**

**Solución Implementada**:
- `xQueuePeek()` permite **leer sin consumir**
- Múltiples tareas pueden leer el mismo dato
- Los datos permanecen en la cola hasta que el productor los sobrescriba

**Alternativas Consideradas**:
1. **Broadcast Pattern**: Cada productor escribe en múltiples colas (una por consumidor)
   - Pros: Cada consumidor tiene su cola dedicada
   - Contras: Mayor uso de memoria, más complejidad
   
2. **Shared Memory con Mutex**: Variables globales protegidas
   - Pros: Menor overhead
   - Contras: Requiere sincronización cuidadosa, más propenso a errores

3. **xQueuePeek()**: Lectura no destructiva (IMPLEMENTADO)
   - Pros: Simple, eficiente, múltiples lectores
   - Contras: El dato permanece hasta ser sobrescrito

## Estructura de Archivos

```
medical_monitor_freertos/
├── README.md                  # Este archivo
├── Makefile                   # Sistema de compilación
├── linker_script.ld          # Script del linker para Cortex-M3
├── startup.s                  # Código de arranque ARM
├── qemu_run.sh               # Script para ejecutar en QEMU
├── .gitignore                # Archivos a ignorar en git
├── include/
│   ├── FreeRTOSConfig.h      # Configuración de FreeRTOS
│   ├── tasks.h               # Declaraciones de tareas
│   └── sensor_simulator.h    # Simulador de sensores
├── src/
│   ├── main.c                # Punto de entrada
│   ├── task_heart_rate.c     # Tarea de frecuencia cardíaca
│   ├── task_spo2.c           # Tarea de saturación O2
│   ├── task_temperature.c    # Tarea de temperatura
│   ├── task_alarm.c          # Tarea de alarmas
│   ├── task_display.c        # Tarea de display
│   └── sensor_simulator.c    # Implementación de simuladores
├── freertos/                 # Fuentes de FreeRTOS (descargar)
└── build/                    # Archivos generados
    ├── *.o
    ├── monitor.elf
    └── monitor.bin
```

## Requisitos Previos

### Herramientas Necesarias

```bash
# Toolchain ARM
sudo apt-get install gcc-arm-none-eabi

# QEMU para ARM
sudo apt-get install qemu-system-arm

# Herramientas de desarrollo
sudo apt-get install make git
```

### Descargar FreeRTOS

```bash
# Clonar FreeRTOS
git clone https://github.com/FreeRTOS/FreeRTOS-Kernel.git freertos

# O descargar release específico
wget https://github.com/FreeRTOS/FreeRTOS-Kernel/archive/refs/tags/V10.5.1.tar.gz
tar -xzf V10.5.1.tar.gz
mv FreeRTOS-Kernel-10.5.1 freertos
```

## Compilación

```bash
# Compilar proyecto
make

# Limpiar archivos generados
make clean

# Recompilar todo
make clean all
```

### Salida Esperada de Compilación

```
Building startup.s...
Building main.c...
Building task_heart_rate.c...
Building task_spo2.c...
Building task_temperature.c...
Building task_alarm.c...
Building task_display.c...
Building sensor_simulator.c...
Building FreeRTOS core...
Linking monitor.elf...
Creating monitor.bin...
Build complete: build/monitor.elf
```

## Ejecución en QEMU

```bash
# Dar permisos de ejecución
chmod +x qemu_run.sh

# Ejecutar
./qemu_run.sh
```

### Comando QEMU Manual

```bash
qemu-system-arm \
  -machine lm3s6965evb \
  -cpu cortex-m3 \
  -kernel build/monitor.elf \
  -nographic \
  -serial mon:stdio
```

## Salida Esperada del Sistema

```
=== Monitor Medico FreeRTOS ===
Sistema inicializado
Inicializando sistema...

[HeartRate] Tarea iniciada
[SpO2] Tarea iniciada
[Temperature] Tarea iniciada
[Alarm] Sistema de alarmas iniciado
[Display] Tarea iniciada

[Monitor] HR: 75.2 bpm | SpO2: 97.1% | Temp: 36.8°C
[Monitor] HR: 76.1 bpm | SpO2: 96.8% | Temp: 36.9°C
[Monitor] HR: 128.5 bpm | SpO2: 97.2% | Temp: 36.7°C

*** ALARMA: TAQUICARDIA (HR=128.5 bpm) ***

[Monitor] HR: 130.2 bpm | SpO2: 85.3% | Temp: 36.8°C

*** ALARMA: TAQUICARDIA (HR=130.2 bpm) ***
*** ALARMA: HIPOXIA (SpO2=85.3%) ***

[Monitor] HR: 78.3 bpm | SpO2: 97.5% | Temp: 38.2°C

*** ALARMA: FIEBRE (Temp=38.2°C) ***
```

## Detalles Técnicos de Implementación

### Startup (startup.s)

El código de arranque implementa:

1. **Vector Table**: Tabla de vectores de interrupción completa para Cortex-M3
2. **Reset_Handler**: Inicializa el sistema al arrancar
   - Copia sección `.data` de FLASH a RAM
   - Inicializa sección `.bss` a cero
   - Llama a `__libc_init_array()` para constructores C++
   - Llama a `SystemInit()` para configuración del sistema
   - Salta a `main()`
3. **Default_Handler**: Manejador por defecto para interrupciones no implementadas

### Linker Script (linker_script.ld)

Define el mapa de memoria:

- **FLASH (256KB)**: Código ejecutable y datos constantes
- **RAM (64KB)**: Datos variables, heap y stack

Secciones:
- `.text`: Código ejecutable
- `.rodata`: Datos de solo lectura
- `.data`: Variables inicializadas (copiadas de FLASH)
- `.bss`: Variables no inicializadas (cero)
- `._heap`: Heap para malloc de FreeRTOS
- `._stack`: Stack del sistema

### FreeRTOS Configuration

Parámetros clave:
- `configCPU_CLOCK_HZ`: 50MHz (frecuencia del sistema)
- `configTICK_RATE_HZ`: 1000 (tick cada 1ms)
- `configMAX_PRIORITIES`: 5 (0-4, donde 4 es máxima)
- `configTOTAL_HEAP_SIZE`: 16KB
- `configUSE_MUTEXES`: Habilitado
- `configCHECK_FOR_STACK_OVERFLOW`: Detección de desbordamiento

### Simulador de Sensores

Genera datos realistas con variación:
- **Heart Rate**: 60-100 bpm (normal), ocasionalmente 120-140 (taquicardia)
- **SpO2**: 95-100% (normal), ocasionalmente 85-94% (hipoxia)
- **Temperature**: 36.0-37.5°C (normal), ocasionalmente 38-39°C (fiebre)

Usa un generador pseudoaleatorio simple para simular variabilidad fisiológica.

## Depuración

### Símbolos de Depuración

El Makefile incluye `-g` para generar símbolos de depuración.

### GDB con QEMU

```bash
# Terminal 1: Iniciar QEMU en modo depuración
qemu-system-arm -machine lm3s6965evb -cpu cortex-m3 \
  -kernel build/monitor.elf -nographic -s -S

# Terminal 2: Conectar GDB
arm-none-eabi-gdb build/monitor.elf
(gdb) target remote localhost:1234
(gdb) break main
(gdb) continue
```

### Verificación de Stack Overflow

FreeRTOS incluye detección de desbordamiento de stack. Si ocurre:

```c
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    // El sistema llamará a esta función
    printf("STACK OVERFLOW en tarea: %s\n", pcTaskName);
    while(1); // Halt
}
```

## Problemas Comunes y Soluciones

### Error: "undefined reference to `__aeabi_*`"

**Causa**: Falta libgcc para operaciones aritméticas ARM.

**Solución**: El Makefile incluye `-lgcc` en el linker.

### Error: "region 'FLASH' overflowed"

**Causa**: El código es demasiado grande para la memoria FLASH.

**Solución**: 
- Optimizar código (`-Os`)
- Reducir `configTOTAL_HEAP_SIZE`
- Eliminar código innecesario

### QEMU no arranca o se congela

**Causa**: Problema en startup o configuración incorrecta.

**Solución**:
- Verificar que `Reset_Handler` esté correcto
- Revisar configuración de stack en linker script
- Usar `-d guest_errors` en QEMU para ver errores

### Tareas no se ejecutan

**Causa**: Prioridades incorrectas o stack insuficiente.

**Solución**:
- Aumentar `configMINIMAL_STACK_SIZE`
- Verificar llamada a `vTaskStartScheduler()`
- Revisar que las tareas no entren en bucle infinito sin delay

## Extensiones Futuras

1. **Interfaz UART Real**: Conectar a puerto serial real
2. **Almacenamiento de Datos**: Guardar histórico en memoria flash simulada
3. **Red de Sensores**: Múltiples pacientes monitoreados
4. **Algoritmos de IA**: Detección predictiva de eventos críticos
5. **Display Gráfico**: Usar framebuffer para gráficos de ondas
6. **Comunicación Inalámbrica**: Simular Bluetooth/WiFi para telemetría

## Referencias

- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
- [ARM Cortex-M3 Technical Reference](https://developer.arm.com/documentation/ddi0337/latest/)
- [QEMU ARM Documentation](https://www.qemu.org/docs/master/system/arm/lm3s6965evb.html)

## Licencia

Este proyecto es material educativo. FreeRTOS está bajo licencia MIT.

## Autor

Tutorial creado para demostración de sistemas embebidos en tiempo real.
