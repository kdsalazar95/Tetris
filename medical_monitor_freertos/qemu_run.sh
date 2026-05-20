#!/bin/bash
################################################################################
# Script de Ejecución en QEMU
# Monitor Médico FreeRTOS - ARM Cortex-M3
################################################################################

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Banner
echo ""
echo "=============================================="
echo "  Monitor Médico FreeRTOS"
echo "  Ejecutando en QEMU (ARM Cortex-M3)"
echo "=============================================="
echo ""

# Verificar que el binario existe
if [ ! -f "build/monitor.elf" ]; then
    echo -e "${RED}ERROR: build/monitor.elf no encontrado${NC}"
    echo "Ejecuta 'make' primero para compilar el proyecto"
    exit 1
fi

# Verificar que QEMU está instalado
if ! command -v qemu-system-arm &> /dev/null; then
    echo -e "${RED}ERROR: qemu-system-arm no está instalado${NC}"
    echo "Instala con: sudo apt-get install qemu-system-arm"
    exit 1
fi

# Información de uso
echo -e "${YELLOW}Instrucciones:${NC}"
echo "  - El monitor se ejecutará en modo consola"
echo "  - Verás los datos de signos vitales en tiempo real"
echo "  - Las alarmas se mostrarán cuando se detecten condiciones anormales"
echo ""
echo -e "${YELLOW}Para salir:${NC}"
echo "  - Presiona: Ctrl-A, luego X"
echo "  - O cierra la terminal"
echo ""
echo -e "${GREEN}Iniciando QEMU...${NC}"
echo ""
sleep 2

# Ejecutar QEMU
# Parámetros:
#   -machine lm3s6965evb : Placa objetivo (LM3S6965 Evaluation Board)
#   -cpu cortex-m3       : Procesador ARM Cortex-M3
#   -kernel              : Archivo ELF a ejecutar
#   -nographic           : Modo consola (sin ventana gráfica)
#   -serial mon:stdio    : Redirigir UART0 a stdio

qemu-system-arm \
    -machine lm3s6965evb \
    -cpu cortex-m3 \
    -kernel build/monitor.elf \
    -nographic \
    -serial mon:stdio

# Si QEMU termina normalmente
echo ""
echo -e "${GREEN}QEMU finalizado${NC}"
