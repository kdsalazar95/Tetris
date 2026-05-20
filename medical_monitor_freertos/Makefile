# ============================================================================
# Makefile para Monitor Médico con FreeRTOS
# ARM Cortex-M3 en QEMU (LM3S6965)
# ============================================================================

# Nombre del proyecto
PROJECT = monitor

# Directorios
BUILD_DIR = build
SRC_DIR = src
INC_DIR = include
FREERTOS_DIR = freertos

# Herramientas del toolchain ARM
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE = $(PREFIX)size

# ============================================================================
# ARCHIVOS FUENTE
# ============================================================================

# Archivos de startup y sistema
STARTUP_SRC = startup.s

# Archivos fuente del proyecto
PROJECT_SRCS = \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/task_heart_rate.c \
	$(SRC_DIR)/task_spo2.c \
	$(SRC_DIR)/task_temperature.c \
	$(SRC_DIR)/task_alarm.c \
	$(SRC_DIR)/task_display.c \
	$(SRC_DIR)/sensor_simulator.c

# Archivos fuente de FreeRTOS
FREERTOS_SRCS = \
	$(FREERTOS_DIR)/tasks.c \
	$(FREERTOS_DIR)/queue.c \
	$(FREERTOS_DIR)/list.c \
	$(FREERTOS_DIR)/timers.c \
	$(FREERTOS_DIR)/event_groups.c \
	$(FREERTOS_DIR)/stream_buffer.c \
	$(FREERTOS_DIR)/portable/GCC/ARM_CM3/port.c \
	$(FREERTOS_DIR)/portable/MemMang/heap_4.c

# ============================================================================
# ARCHIVOS OBJETO
# ============================================================================

# Convertir .c a .o en BUILD_DIR
PROJECT_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(PROJECT_SRCS))
FREERTOS_OBJS = $(patsubst $(FREERTOS_DIR)/%.c,$(BUILD_DIR)/freertos_%.o,$(FREERTOS_SRCS))
STARTUP_OBJ = $(BUILD_DIR)/startup.o

# Todos los objetos
OBJS = $(STARTUP_OBJ) $(PROJECT_OBJS) $(FREERTOS_OBJS)

# ============================================================================
# DIRECTORIOS DE INCLUDE
# ============================================================================

INCLUDES = \
	-I$(INC_DIR) \
	-I$(FREERTOS_DIR)/include \
	-I$(FREERTOS_DIR)/portable/GCC/ARM_CM3

# ============================================================================
# FLAGS DE COMPILACIÓN
# ============================================================================

# Flags comunes para ARM Cortex-M3
ARCH_FLAGS = \
	-mcpu=cortex-m3 \
	-mthumb \
	-mfloat-abi=soft

# Flags de optimización y debugging
OPT_FLAGS = \
	-O2 \
	-g3 \
	-ffunction-sections \
	-fdata-sections

# Flags de warnings
WARN_FLAGS = \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-unused-parameter

# Defines
DEFINES = \
	-DQEMU \
	-DGCC_ARMCM3

# Flags completos de compilación C
CFLAGS = \
	$(ARCH_FLAGS) \
	$(OPT_FLAGS) \
	$(WARN_FLAGS) \
	$(DEFINES) \
	$(INCLUDES) \
	-std=c99

# Flags de ensamblador
ASFLAGS = \
	$(ARCH_FLAGS)

# ============================================================================
# FLAGS DE LINKEO
# ============================================================================

# Linker script
LDSCRIPT = linker_script.ld

# Flags de linkeo
LDFLAGS = \
	$(ARCH_FLAGS) \
	-T$(LDSCRIPT) \
	-Wl,--gc-sections \
	-Wl,-Map=$(BUILD_DIR)/$(PROJECT).map \
	-Wl,--print-memory-usage \
	--specs=nano.specs \
	--specs=nosys.specs

# Librerías
LIBS = \
	-lc \
	-lm \
	-lgcc

# ============================================================================
# TARGETS
# ============================================================================

# Target por defecto
.PHONY: all
all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).bin
	@echo ""
	@echo "======================================"
	@echo "Build completo: $(BUILD_DIR)/$(PROJECT).elf"
	@echo "======================================"
	@$(SIZE) $(BUILD_DIR)/$(PROJECT).elf

# Crear el directorio build si no existe
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/freertos

# ============================================================================
# COMPILACIÓN DE STARTUP (ASSEMBLY)
# ============================================================================

$(BUILD_DIR)/startup.o: $(STARTUP_SRC) | $(BUILD_DIR)
	@echo "Building startup.s..."
	$(CC) $(ASFLAGS) -c $< -o $@

# ============================================================================
# COMPILACIÓN DE ARCHIVOS DEL PROYECTO
# ============================================================================

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "Building $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# COMPILACIÓN DE ARCHIVOS DE FREERTOS
# ============================================================================

# Regla para archivos en el directorio raíz de FreeRTOS
$(BUILD_DIR)/freertos_%.o: $(FREERTOS_DIR)/%.c | $(BUILD_DIR)
	@echo "Building $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para archivos en portable/GCC/ARM_CM3
$(BUILD_DIR)/freertos_port.o: $(FREERTOS_DIR)/portable/GCC/ARM_CM3/port.c | $(BUILD_DIR)
	@echo "Building FreeRTOS port..."
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para archivos en portable/MemMang
$(BUILD_DIR)/freertos_heap_4.o: $(FREERTOS_DIR)/portable/MemMang/heap_4.c | $(BUILD_DIR)
	@echo "Building FreeRTOS heap_4..."
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# LINKEO
# ============================================================================

$(BUILD_DIR)/$(PROJECT).elf: $(OBJS) $(LDSCRIPT)
	@echo "Linking $(PROJECT).elf..."
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

# ============================================================================
# GENERACIÓN DE BINARIO
# ============================================================================

$(BUILD_DIR)/$(PROJECT).bin: $(BUILD_DIR)/$(PROJECT).elf
	@echo "Creating $(PROJECT).bin..."
	$(OBJCOPY) -O binary $< $@

# ============================================================================
# UTILIDADES
# ============================================================================

# Limpiar archivos generados
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)

# Mostrar información de tamaño
.PHONY: size
size: $(BUILD_DIR)/$(PROJECT).elf
	$(SIZE) $<

# Generar disassembly
.PHONY: disasm
disasm: $(BUILD_DIR)/$(PROJECT).elf
	$(OBJDUMP) -d $< > $(BUILD_DIR)/$(PROJECT).disasm
	@echo "Disassembly guardado en $(BUILD_DIR)/$(PROJECT).disasm"

# Ejecutar en QEMU
.PHONY: run
run: $(BUILD_DIR)/$(PROJECT).elf
	@echo "Ejecutando en QEMU..."
	@echo "Presiona Ctrl-A X para salir"
	@echo ""
	qemu-system-arm \
		-machine lm3s6965evb \
		-cpu cortex-m3 \
		-kernel $< \
		-nographic \
		-serial mon:stdio

# Debug con QEMU (espera conexión GDB)
.PHONY: debug
debug: $(BUILD_DIR)/$(PROJECT).elf
	@echo "Iniciando QEMU en modo debug..."
	@echo "Conectar con: arm-none-eabi-gdb $(BUILD_DIR)/$(PROJECT).elf"
	@echo "Luego: target remote localhost:1234"
	@echo ""
	qemu-system-arm \
		-machine lm3s6965evb \
		-cpu cortex-m3 \
		-kernel $< \
		-nographic \
		-serial mon:stdio \
		-s -S

# Verificar dependencias
.PHONY: check-deps
check-deps:
	@echo "Verificando dependencias..."
	@which $(CC) > /dev/null || (echo "ERROR: $(CC) no encontrado"; exit 1)
	@which qemu-system-arm > /dev/null || (echo "ERROR: qemu-system-arm no encontrado"; exit 1)
	@test -d $(FREERTOS_DIR) || (echo "ERROR: Directorio FreeRTOS no encontrado en $(FREERTOS_DIR)"; exit 1)
	@echo "✓ Todas las dependencias están instaladas"

# Ayuda
.PHONY: help
help:
	@echo "Monitor Médico FreeRTOS - Makefile"
	@echo ""
	@echo "Targets disponibles:"
	@echo "  all        - Compilar proyecto completo (default)"
	@echo "  clean      - Limpiar archivos generados"
	@echo "  size       - Mostrar tamaño del binario"
	@echo "  disasm     - Generar disassembly"
	@echo "  run        - Ejecutar en QEMU"
	@echo "  debug      - Ejecutar en QEMU modo debug (puerto 1234)"
	@echo "  check-deps - Verificar que todas las dependencias estén instaladas"
	@echo "  help       - Mostrar esta ayuda"
	@echo ""
	@echo "Ejemplos:"
	@echo "  make              - Compilar todo"
	@echo "  make clean all    - Recompilar desde cero"
	@echo "  make run          - Compilar y ejecutar"

# ============================================================================
# DEPENDENCIAS
# ============================================================================

# Hacer que todos los .o dependan de los headers
$(PROJECT_OBJS): $(wildcard $(INC_DIR)/*.h)
$(FREERTOS_OBJS): $(wildcard $(FREERTOS_DIR)/include/*.h) $(wildcard $(INC_DIR)/*.h)

# Evitar borrado de archivos intermedios
.SECONDARY: $(OBJS)
