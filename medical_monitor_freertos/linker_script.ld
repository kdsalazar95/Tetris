/*
 * Linker Script para ARM Cortex-M3 (LM3S6965)
 * 
 * Define el mapa de memoria y las secciones para el sistema embebido.
 * Compatible con QEMU machine lm3s6965evb.
 */

/* Punto de entrada del programa */
ENTRY(Reset_Handler)

/* Tamaño del stack principal (8KB) */
_stack_size = 0x2000;

/* Definición de regiones de memoria */
MEMORY
{
    /* FLASH: 256KB - Almacena código y datos constantes */
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
    
    /* RAM: 64KB - Almacena datos variables, heap y stack */
    RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 64K
}

/* Definición de secciones */
SECTIONS
{
    /* 
     * Sección .text - Código ejecutable
     * Se ubica en FLASH (memoria no volátil)
     */
    .text :
    {
        /* Vector table debe estar al inicio */
        KEEP(*(.vector_table))
        
        /* Código de startup */
        *(.text.Reset_Handler)
        *(.text.Default_Handler)
        
        /* Todo el código restante */
        *(.text*)
        
        /* Funciones de inicialización de C++ */
        *(.init)
        *(.fini)
        
        /* Manejo de excepciones C++ */
        *(.eh_frame)
        
        /* Alineamiento a 4 bytes */
        . = ALIGN(4);
        
    } > FLASH

    /*
     * Sección .rodata - Datos de solo lectura
     * Constantes, strings literales, etc.
     */
    .rodata :
    {
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /*
     * Array de inicialización (constructores C++)
     */
    .preinit_array :
    {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } > FLASH

    /*
     * Símbolos para inicialización de .data
     * _sidata: Inicio de datos en FLASH (fuente)
     */
    _sidata = LOADADDR(.data);

    /*
     * Sección .data - Datos inicializados
     * Se carga desde FLASH pero se ejecuta desde RAM
     */
    .data :
    {
        /* Símbolo de inicio de .data en RAM */
        _sdata = .;
        
        /* Todos los datos inicializados */
        *(.data*)
        
        . = ALIGN(4);
        
        /* Símbolo de fin de .data en RAM */
        _edata = .;
        
    } > RAM AT> FLASH

    /*
     * Sección .bss - Datos no inicializados
     * Se inicializa a cero en startup
     */
    .bss :
    {
        /* Símbolo de inicio de .bss */
        _sbss = .;
        __bss_start__ = _sbss;
        
        /* Todos los datos no inicializados */
        *(.bss*)
        *(COMMON)
        
        . = ALIGN(4);
        
        /* Símbolo de fin de .bss */
        _ebss = .;
        __bss_end__ = _ebss;
        
    } > RAM

    /*
     * Sección ._heap - Heap para malloc/FreeRTOS
     * Usado por el gestor de memoria de FreeRTOS
     */
    ._heap :
    {
        . = ALIGN(8);
        __heap_start__ = .;
        __end__ = .;
        end = __end__;
        
        /* Reservar espacio para heap (el resto de RAM menos el stack) */
        . = ORIGIN(RAM) + LENGTH(RAM) - _stack_size;
        
        __heap_end__ = .;
        __HeapLimit = __heap_end__;
        
    } > RAM

    /*
     * Sección ._stack - Stack del sistema
     * Crece hacia abajo desde el final de la RAM
     */
    ._stack :
    {
        . = ALIGN(8);
        __stack_start__ = .;
        
        /* Reservar espacio para el stack */
        . = . + _stack_size;
        
        . = ALIGN(8);
        __stack_end__ = .;
        
    } > RAM

    /*
     * Símbolo especial: inicio del stack (top)
     * El stack de ARM crece hacia abajo, por lo que apunta al final
     */
    _estack = ORIGIN(RAM) + LENGTH(RAM);

    /*
     * Símbolos adicionales útiles para depuración
     */
    _heap_size = __heap_end__ - __heap_start__;
    _ram_end = ORIGIN(RAM) + LENGTH(RAM);
    
    /*
     * Secciones de depuración (no se cargan en el dispositivo)
     */
    .ARM.attributes 0 : { *(.ARM.attributes) }
    .comment 0 : { *(.comment) }
    .debug_info 0 : { *(.debug_info) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_line 0 : { *(.debug_line) }
    .debug_frame 0 : { *(.debug_frame) }
    .debug_str 0 : { *(.debug_str) }
    .debug_loc 0 : { *(.debug_loc) }
    .debug_ranges 0 : { *(.debug_ranges) }
}
