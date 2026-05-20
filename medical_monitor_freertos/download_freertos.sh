#!/bin/bash
################################################################################
# Script de Descarga de FreeRTOS
# Monitor Médico FreeRTOS
################################################################################

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo ""
echo "=============================================="
echo "  Descarga de FreeRTOS Kernel"
echo "=============================================="
echo ""

# Verificar si FreeRTOS ya existe
if [ -d "freertos" ]; then
    echo -e "${YELLOW}El directorio 'freertos' ya existe.${NC}"
    read -p "¿Deseas reemplazarlo? (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Eliminando directorio anterior..."
        rm -rf freertos
    else
        echo "Operación cancelada."
        exit 0
    fi
fi

# Verificar que git está instalado
if ! command -v git &> /dev/null; then
    echo -e "${RED}ERROR: git no está instalado${NC}"
    echo "Instala con: sudo apt-get install git"
    exit 1
fi

# Opción 1: Clonar repositorio completo (recomendado)
echo -e "${GREEN}Opción 1: Clonar repositorio de FreeRTOS Kernel${NC}"
echo "Clonando desde GitHub..."
echo ""

git clone --depth 1 --branch V10.5.1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git freertos

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ FreeRTOS descargado exitosamente${NC}"
    echo ""
    echo "Archivos descargados en: ./freertos/"
    echo ""
    echo "Ahora puedes compilar el proyecto con:"
    echo "  make"
    echo ""
    echo "O ejecutarlo directamente con:"
    echo "  make run"
    echo ""
else
    echo ""
    echo -e "${RED}ERROR: Fallo al descargar FreeRTOS${NC}"
    echo ""
    echo "Alternativa manual:"
    echo "1. Descarga desde: https://github.com/FreeRTOS/FreeRTOS-Kernel/archive/refs/tags/V10.5.1.tar.gz"
    echo "2. Extrae el archivo"
    echo "3. Renombra la carpeta a 'freertos'"
    echo ""
    exit 1
fi

# Verificar estructura
echo "Verificando estructura de archivos..."
REQUIRED_FILES=(
    "freertos/tasks.c"
    "freertos/queue.c"
    "freertos/list.c"
    "freertos/portable/GCC/ARM_CM3/port.c"
    "freertos/portable/MemMang/heap_4.c"
)

MISSING_FILES=0
for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo -e "${RED}✗ Falta: $file${NC}"
        MISSING_FILES=$((MISSING_FILES + 1))
    else
        echo -e "${GREEN}✓ $file${NC}"
    fi
done

echo ""
if [ $MISSING_FILES -eq 0 ]; then
    echo -e "${GREEN}✓ Todos los archivos necesarios están presentes${NC}"
    echo ""
    echo "El proyecto está listo para compilar."
else
    echo -e "${RED}Faltan $MISSING_FILES archivos necesarios${NC}"
    echo "Verifica la descarga o descarga manualmente desde GitHub"
fi
