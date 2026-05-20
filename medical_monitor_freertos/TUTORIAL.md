# Tutorial Completo: Monitor Médico con FreeRTOS en QEMU

## Guía Paso a Paso desde Cero

Este tutorial te guiará desde la instalación del entorno hasta la ejecución exitosa del monitor médico.

---

## Parte 1: Configuración del Entorno

### Paso 1.1: Instalar Herramientas Necesarias

#### En Ubuntu/Debian:

```bash
# Actualizar repositorios
sudo apt-get update

# Instalar toolchain ARM
sudo apt-get install gcc-arm-none-eabi

# Instalar QEMU para ARM
sudo apt-get install qemu-system-arm

# Instalar herramientas adicionales
sudo apt-get install make git
```

#### En otras distribuciones:

**Fedora/RHEL:**
```bash
sudo dnf install arm-none-eabi-gcc-cs qemu-system-arm make git
```

**Arch Linux:**
```bash
sudo pacman -S arm-none-eabi-gcc qemu-arch-extra make git
```

### Paso 1.2: Verificar Instalación

```bash
# Verificar toolchain ARM
arm-none-eabi-gcc --version
# Debería mostrar: arm-none-eabi-gcc (GNU Arm Embedded Toolchain ...) X.X.X

# Verificar QEMU
qemu-system-arm --version
# Debería mostrar: QEMU emulator version X.X.X

# Verificar Make
make --version
# Debería mostrar: GNU Make X.X

# Verificar Git
git --version
# Debería mostrar: git version X.X.X
```

Si algún comando falla, revisa la instalación de esa herramienta.

---

## Parte 2: Obtener el Proyecto

### Paso 2.1: Descargar/Clonar el Proyecto

Si tienes el proyecto en un repositorio:
```bash
git clone <url-del-repositorio>
cd medical_monitor_freertos
```

Si tienes los archivos directamente:
```bash
cd medical_monitor_freertos
```

### Paso 2.2: Verificar Estructura de Archivos

```bash
ls -la
```

Deberías ver:
```
.
├── README.md
├── Makefile
├── linker_script.ld
├── startup.s
├── qemu_run.sh
├── download_freertos.sh
├── .gitignore
├── include/
│   ├── FreeRTOSConfig.h
│   ├── tasks.h
│   └── sensor_simulator.h
├── src/
│   ├── main.c
│   ├── task_heart_rate.c
│   ├── task_spo2.c
│   ├── task_temperature.c
│   ├── task_alarm.c
│   ├── task_display.c
│   └── sensor_simulator.c
└── build/  (se creará al compilar)
```

---

## Parte 3: Descargar FreeRTOS

### Paso 3.1: Ejecutar Script de Descarga (Recomendado)

```bash
./download_freertos.sh
```

Este script:
1. Descarga FreeRTOS Kernel v10.5.1 desde GitHub
2. Lo coloca en el directorio `freertos/`
3. Verifica que todos los archivos necesarios están presentes

### Paso 3.2: Descarga Manual (Alternativa)

Si el script falla o prefieres hacerlo manualmente:

```bash
# Opción A: Usando git
git clone --depth 1 --branch V10.5.1 \
    https://github.com/FreeRTOS/FreeRTOS-Kernel.git freertos

# Opción B: Descarga directa
wget https://github.com/FreeRTOS/FreeRTOS-Kernel/archive/refs/tags/V10.5.1.tar.gz
tar -xzf V10.5.1.tar.gz
mv FreeRTOS-Kernel-10.5.1 freertos
```

### Paso 3.3: Verificar FreeRTOS

```bash
ls freertos/
```

Deberías ver:
```
include/
portable/
tasks.c
queue.c
list.c
timers.c
event_groups.c
stream_buffer.c
...
```

---

## Parte 4: Compilar el Proyecto

### Paso 4.1: Verificar Dependencias

```bash
make check-deps
```

Este comando verifica que todas las herramientas y archivos necesarios estén disponibles.

### Paso 4.2: Compilar

```bash
make
```

**Salida esperada:**
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

======================================
Build completo: build/monitor.elf
======================================
   text    data     bss     dec     hex filename
  45678    1234    5678   52590    cd6e build/monitor.elf
```

### Paso 4.3: Solución de Problemas de Compilación

**Error: "freertos/tasks.c: No such file or directory"**
- **Causa**: FreeRTOS no está descargado
- **Solución**: Ejecuta `./download_freertos.sh`

**Error: "arm-none-eabi-gcc: command not found"**
- **Causa**: Toolchain ARM no instalado
- **Solución**: `sudo apt-get install gcc-arm-none-eabi`

**Error: "region 'FLASH' overflowed"**
- **Causa**: El código es muy grande
- **Solución**: Ya está optimizado con `-O2`. Esto no debería ocurrir.

**Error: "undefined reference to '__aeabi_*'"**
- **Causa**: Falta libgcc
- **Solución**: Ya está incluido en el Makefile con `-lgcc`

---

## Parte 5: Ejecutar en QEMU

### Paso 5.1: Ejecutar con Script (Recomendado)

```bash
./qemu_run.sh
```

### Paso 5.2: Ejecutar Manualmente

```bash
make run
```

O directamente:
```bash
qemu-system-arm \
    -machine lm3s6965evb \
    -cpu cortex-m3 \
    -kernel build/monitor.elf \
    -nographic \
    -serial mon:stdio
```

### Paso 5.3: Salida Esperada

```
===================================
  Monitor Medico FreeRTOS v1.0
  ARM Cortex-M3 en QEMU
===================================

Inicializando sistema...
Creando colas...
Creando mutexes...
Creando tareas...

Sistema inicializado correctamente
Iniciando scheduler...

[HeartRate] Tarea iniciada
[SpO2] Tarea iniciada
[Temperature] Tarea iniciada
[Alarm] Sistema de alarmas iniciado
[Display] Tarea iniciada

[Monitor] HR: 75.2 bpm | SpO2: 97.1% | Temp: 36.8°C
[Monitor] HR: 76.1 bpm | SpO2: 96.8% | Temp: 36.9°C
[Monitor] HR: 78.5 bpm | SpO2: 97.2% | Temp: 36.7°C
[Monitor] HR: 128.3 bpm | SpO2: 97.5% | Temp: 36.8°C

*** ALARMA: TAQUICARDIA (HR=128.3 bpm) ***

[Monitor] HR: 130.1 bpm | SpO2: 87.2% | Temp: 36.9°C

*** ALARMA: TAQUICARDIA (HR=130.1 bpm) ***
*** ALARMA: HIPOXIA (SpO2=87.2%) ***

[Monitor] HR: 77.8 bpm | SpO2: 98.1% | Temp: 38.3°C

*** ALARMA: FIEBRE (Temp=38.3°C) ***
```

### Paso 5.4: Salir de QEMU

**Método 1 (recomendado):**
1. Presiona `Ctrl-A`
2. Luego presiona `X`

**Método 2:**
- Cierra la terminal

**Método 3:**
- En el monitor QEMU, escribe: `quit`

---

## Parte 6: Entendiendo el Sistema

### Arquitectura de Tareas

```
┌─────────────────────────────────────────────────────┐
│                  FreeRTOS Scheduler                 │
│              (Prioridad 0-4, Tick 1ms)             │
└─────────────────────────────────────────────────────┘
                         │
         ┌───────────────┼───────────────┐
         │               │               │
    ┌────▼────┐    ┌────▼────┐    ┌────▼────┐
    │  Heart  │    │  SpO2   │    │  Temp   │
    │  Rate   │    │ Monitor │    │ Monitor │
    │  P:3    │    │  P:3    │    │  P:2    │
    │ 500ms   │    │ 1000ms  │    │ 2000ms  │
    └────┬────┘    └────┬────┘    └────┬────┘
         │              │              │
         │   Produce    │   Produce    │   Produce
         ▼              ▼              ▼
    ┌────────┐    ┌─────────┐    ┌──────────┐
    │HR Queue│    │SpO2Queue│    │TempQueue │
    │Size: 1 │    │ Size: 1 │    │ Size: 1  │
    └────┬───┘    └────┬────┘    └────┬─────┘
         │             │              │
         │  Peek       │  Peek        │  Peek
         └─────────┬───┴──────┬───────┘
                   │          │
              ┌────▼────┐ ┌───▼─────┐
              │  Alarm  │ │ Display │
              │  P:4    │ │  P:1    │
              │ 100ms   │ │ 1000ms  │
              └─────────┘ └─────────┘
```

### Flujo de Datos

1. **Producción** (Tareas de Sensores):
   - Leen sensor simulado
   - Empaquetan dato + timestamp
   - Escriben con `xQueueOverwrite()` (sobrescribe anterior)

2. **Consumo** (Alarm y Display):
   - Leen con `xQueuePeek()` (NO destructivo)
   - Procesan datos independientemente
   - Ambas ven los mismos valores

### ¿Por qué xQueuePeek() y no xQueueReceive()?

**Problema:**
```
Si Display usa xQueueReceive():
  1. Sensor escribe HR=130 (taquicardia)
  2. Display ejecuta primero, consume el dato
  3. Alarm ejecuta después, cola vacía
  4. ¡Alarma nunca se activa! 🚨 PELIGRO
```

**Solución:**
```
Con xQueuePeek():
  1. Sensor escribe HR=130
  2. Display lee (peek), dato permanece
  3. Alarm lee (peek), detecta taquicardia
  4. ✓ Ambas tareas ven el dato
```

---

## Parte 7: Depuración

### Ver Información de Tamaño

```bash
make size
```

Muestra uso de memoria FLASH y RAM.

### Generar Disassembly

```bash
make disasm
```

Genera `build/monitor.disasm` con código desensamblado.

### Debug con GDB

**Terminal 1: Iniciar QEMU en modo debug**
```bash
make debug
```

**Terminal 2: Conectar GDB**
```bash
arm-none-eabi-gdb build/monitor.elf

(gdb) target remote localhost:1234
(gdb) break main
(gdb) continue
(gdb) next
(gdb) print hrData
```

---

## Parte 8: Modificaciones y Experimentos

### Cambiar Umbrales de Alarma

Edita `include/tasks.h`:

```c
// Valores originales
#define HR_THRESHOLD_HIGH    120.0f
#define SPO2_THRESHOLD_LOW   90.0f
#define TEMP_THRESHOLD_HIGH  38.0f

// Experimenta con:
#define HR_THRESHOLD_HIGH    100.0f  // Más sensible
#define SPO2_THRESHOLD_LOW   95.0f   // Más estricto
```

Recompila: `make clean all`

### Cambiar Prioridades

Edita `include/tasks.h`:

```c
// ¿Qué pasa si Display tiene mayor prioridad que Alarm?
#define PRIORITY_DISPLAY    4  // Era 1
#define PRIORITY_ALARM      1  // Era 4
```

**Resultado esperado:** Las alarmas se retrasarán peligrosamente.

### Cambiar Períodos

Edita `include/tasks.h`:

```c
// Heart rate más rápido
#define PERIOD_HEART_RATE_MS  100  // Era 500
```

Observa cómo cambia la carga del sistema.

---

## Parte 9: Preguntas Frecuentes

**Q: ¿Por qué usar vTaskDelayUntil() en lugar de vTaskDelay()?**

A: `vTaskDelayUntil()` garantiza periodicidad EXACTA. Si una tarea debe ejecutarse cada 500ms, `vTaskDelay(500)` resulta en `500ms + tiempo_de_ejecución`, acumulando drift. `vTaskDelayUntil()` compensa automáticamente.

**Q: ¿Qué pasa si una tarea se demora más que su período?**

A: El scheduler la ejecutará lo antes posible, pero "perderá" ciclos. Por eso las tareas deben completarse rápido.

**Q: ¿Por qué Alarm tiene prioridad máxima?**

A: Las alarmas son críticas para seguridad del paciente. Un retraso puede ser fatal. SIEMPRE deben ejecutarse primero.

**Q: ¿Puedo agregar más sensores?**

A: Sí. Sigue el patrón:
1. Define estructura de datos en `tasks.h`
2. Crea cola global
3. Implementa tarea productora
4. Actualiza Alarm y Display para consumir

**Q: ¿Cómo pruebo sin QEMU?**

A: Necesitas hardware ARM Cortex-M3 real (ej: STM32F1) y adaptar:
- Configuración de pines y periféricos
- UART real (no simulado)
- Sensores físicos (ADC, I2C, SPI)

---

## Parte 10: Recursos Adicionales

### Documentación

- [FreeRTOS Official Docs](https://www.freertos.org/Documentation/RTOS_book.html)
- [ARM Cortex-M3 Guide](https://developer.arm.com/documentation/ddi0337/latest/)
- [QEMU ARM Documentation](https://www.qemu.org/docs/master/system/arm/lm3s6965evb.html)

### Libros Recomendados

- "Mastering the FreeRTOS Real Time Kernel" - Richard Barry
- "The Definitive Guide to ARM Cortex-M3" - Joseph Yiu

### Estándares Médicos

- IEC 60601-1: Equipos médicos eléctricos
- ISO 13485: Sistemas de gestión de calidad para dispositivos médicos

---

## Conclusión

Has creado un sistema de monitor médico funcional en tiempo real. Ahora entiendes:

✓ Arquitectura de sistemas RTOS  
✓ Comunicación entre tareas (colas)  
✓ Sincronización (mutexes)  
✓ Prioridades y scheduling  
✓ Diseño de sistemas embebidos seguros  

**¡Felicitaciones! 🎉**

---

## Soporte

Si encuentras problemas:

1. Verifica que seguiste todos los pasos
2. Lee los mensajes de error cuidadosamente
3. Revisa la sección de "Solución de Problemas"
4. Consulta la documentación de FreeRTOS

**Errores comunes y soluciones ya están documentados en cada sección.**
