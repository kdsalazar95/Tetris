/*
 * Startup code para ARM Cortex-M3
 * 
 * Este archivo contiene:
 * - Tabla de vectores de interrupción
 * - Reset Handler (inicialización del sistema)
 * - Default handlers para interrupciones
 */

    .syntax unified
    .cpu cortex-m3
    .fpu softvfp
    .thumb

/*
 * Definición de símbolos del linker script
 */
.global _estack
.global _sidata
.global _sdata
.global _edata
.global _sbss
.global _ebss

/*
 * Declaración de funciones externas
 */
.extern main
.extern SystemInit

/*
 * ============================================================================
 * TABLA DE VECTORES
 * ============================================================================
 * 
 * La tabla de vectores define los manejadores de interrupciones y excepciones.
 * Debe estar ubicada al inicio de la FLASH (dirección 0x00000000).
 * 
 * Estructura:
 * - Posición 0: Valor inicial del Stack Pointer (SP)
 * - Posición 1: Reset Handler (punto de entrada)
 * - Posiciones 2-15: Excepciones del sistema Cortex-M3
 * - Posiciones 16+: Interrupciones específicas del dispositivo
 */

    .section .vector_table,"a",%progbits
    .type vector_table, %object

vector_table:
    /* Excepciones del Cortex-M3 Core */
    .word   _estack                     /* 0: Valor inicial del Stack Pointer */
    .word   Reset_Handler               /* 1: Reset Handler */
    .word   NMI_Handler                 /* 2: Non Maskable Interrupt */
    .word   HardFault_Handler           /* 3: Hard Fault */
    .word   MemManage_Handler           /* 4: Memory Management */
    .word   BusFault_Handler            /* 5: Bus Fault */
    .word   UsageFault_Handler          /* 6: Usage Fault */
    .word   0                           /* 7: Reservado */
    .word   0                           /* 8: Reservado */
    .word   0                           /* 9: Reservado */
    .word   0                           /* 10: Reservado */
    .word   SVC_Handler                 /* 11: Supervisor Call */
    .word   DebugMon_Handler            /* 12: Debug Monitor */
    .word   0                           /* 13: Reservado */
    .word   PendSV_Handler              /* 14: Pendable Service Call */
    .word   SysTick_Handler             /* 15: System Tick Timer */
    
    /* Interrupciones externas específicas del LM3S6965 */
    .word   GPIOA_Handler               /* 16: GPIO Port A */
    .word   GPIOB_Handler               /* 17: GPIO Port B */
    .word   GPIOC_Handler               /* 18: GPIO Port C */
    .word   GPIOD_Handler               /* 19: GPIO Port D */
    .word   GPIOE_Handler               /* 20: GPIO Port E */
    .word   UART0_Handler               /* 21: UART0 */
    .word   UART1_Handler               /* 22: UART1 */
    .word   SSI0_Handler                /* 23: SSI0 */
    .word   I2C0_Handler                /* 24: I2C0 */
    .word   PWMFault_Handler            /* 25: PWM Fault */
    .word   PWM0_Handler                /* 26: PWM Generator 0 */
    .word   PWM1_Handler                /* 27: PWM Generator 1 */
    .word   PWM2_Handler                /* 28: PWM Generator 2 */
    .word   QEI0_Handler                /* 29: QEI0 */
    .word   ADC0_Handler                /* 30: ADC Sequence 0 */
    .word   ADC1_Handler                /* 31: ADC Sequence 1 */
    .word   ADC2_Handler                /* 32: ADC Sequence 2 */
    .word   ADC3_Handler                /* 33: ADC Sequence 3 */
    .word   Watchdog_Handler            /* 34: Watchdog Timer */
    .word   Timer0A_Handler             /* 35: Timer 0A */
    .word   Timer0B_Handler             /* 36: Timer 0B */
    .word   Timer1A_Handler             /* 37: Timer 1A */
    .word   Timer1B_Handler             /* 38: Timer 1B */
    .word   Timer2A_Handler             /* 39: Timer 2A */
    .word   Timer2B_Handler             /* 40: Timer 2B */
    .word   Comp0_Handler               /* 41: Analog Comparator 0 */
    .word   Comp1_Handler               /* 42: Analog Comparator 1 */
    .word   Comp2_Handler               /* 43: Analog Comparator 2 */
    .word   SysCtrl_Handler             /* 44: System Control */
    .word   FlashCtrl_Handler           /* 45: Flash Control */
    .word   GPIOF_Handler               /* 46: GPIO Port F */
    .word   GPIOG_Handler               /* 47: GPIO Port G */
    .word   GPIOH_Handler               /* 48: GPIO Port H */
    .word   UART2_Handler               /* 49: UART2 */
    .word   SSI1_Handler                /* 50: SSI1 */
    .word   Timer3A_Handler             /* 51: Timer 3A */
    .word   Timer3B_Handler             /* 52: Timer 3B */
    .word   I2C1_Handler                /* 53: I2C1 */
    .word   QEI1_Handler                /* 54: QEI1 */
    .word   CAN0_Handler                /* 55: CAN0 */
    .word   CAN1_Handler                /* 56: CAN1 */
    .word   CAN2_Handler                /* 57: CAN2 */
    .word   Ethernet_Handler            /* 58: Ethernet Controller */
    .word   Hibernate_Handler           /* 59: Hibernation Module */

    .size vector_table, .-vector_table

/*
 * ============================================================================
 * RESET HANDLER
 * ============================================================================
 * 
 * El Reset Handler es el punto de entrada del programa después de un reset.
 * 
 * Secuencia de inicialización:
 * 1. Copiar datos inicializados de FLASH a RAM (sección .data)
 * 2. Inicializar datos no inicializados a cero (sección .bss)
 * 3. Llamar a constructores C++ (__libc_init_array)
 * 4. Llamar a SystemInit() para configuración del hardware
 * 5. Llamar a main() para iniciar el programa
 */

    .section .text.Reset_Handler
    .type Reset_Handler, %function
    .global Reset_Handler

Reset_Handler:
    /*
     * 1. COPIAR SECCIÓN .data DE FLASH A RAM
     * 
     * La sección .data contiene variables globales inicializadas.
     * En FLASH están los valores iniciales, que deben copiarse a RAM.
     */
    
    ldr     r0, =_sdata         /* R0 = dirección de inicio de .data en RAM */
    ldr     r1, =_edata         /* R1 = dirección de fin de .data en RAM */
    ldr     r2, =_sidata        /* R2 = dirección de .data en FLASH (fuente) */
    movs    r3, #0              /* R3 = 0 (contador) */
    b       copy_data_check

copy_data_loop:
    ldr     r4, [r2, r3]        /* Leer word de FLASH */
    str     r4, [r0, r3]        /* Escribir word en RAM */
    adds    r3, r3, #4          /* Incrementar offset */

copy_data_check:
    adds    r4, r0, r3          /* R4 = dirección actual en RAM */
    cmp     r4, r1              /* ¿Llegamos al final? */
    bcc     copy_data_loop      /* Si no, continuar copiando */

    /*
     * 2. INICIALIZAR SECCIÓN .bss A CERO
     * 
     * La sección .bss contiene variables globales no inicializadas.
     * Según el estándar C, deben inicializarse a cero.
     */
    
    ldr     r0, =_sbss          /* R0 = inicio de .bss */
    ldr     r1, =_ebss          /* R1 = fin de .bss */
    movs    r2, #0              /* R2 = 0 (valor a escribir) */
    b       zero_bss_check

zero_bss_loop:
    str     r2, [r0]            /* Escribir 0 en dirección actual */
    adds    r0, r0, #4          /* Siguiente word */

zero_bss_check:
    cmp     r0, r1              /* ¿Llegamos al final? */
    bcc     zero_bss_loop       /* Si no, continuar */

    /*
     * 3. LLAMAR A __libc_init_array
     * 
     * Esta función ejecuta los constructores de C++.
     * También inicializa la biblioteca C estándar.
     */
    bl      __libc_init_array

    /*
     * 4. LLAMAR A SystemInit
     * 
     * SystemInit() configura el hardware del sistema:
     * - Configuración de relojes
     * - Inicialización de periféricos básicos
     * - Configuración de la MPU (si se usa)
     */
    bl      SystemInit

    /*
     * 5. LLAMAR A main
     * 
     * Transferir control al programa principal.
     * main() nunca debería retornar, pero si lo hace, entramos en bucle infinito.
     */
    bl      main

    /*
     * Si main() retorna (no debería), entrar en bucle infinito
     */
infinite_loop:
    b       infinite_loop

    .size Reset_Handler, .-Reset_Handler

/*
 * ============================================================================
 * DEFAULT HANDLER
 * ============================================================================
 * 
 * Handler por defecto para interrupciones no implementadas.
 * Simplemente entra en un bucle infinito.
 * 
 * En un sistema de producción, esto podría:
 * - Registrar el error
 * - Realizar un reset del sistema
 * - Activar un LED de error
 */

    .section .text.Default_Handler,"ax",%progbits
    .type Default_Handler, %function

Default_Handler:
    b       Default_Handler
    .size Default_Handler, .-Default_Handler

/*
 * ============================================================================
 * WEAK ALIASES
 * ============================================================================
 * 
 * Definir todos los handlers como weak aliases al Default_Handler.
 * Esto permite que el usuario sobrescriba cualquier handler sin modificar este archivo.
 * 
 * .weak: Si el símbolo no se define en otro lugar, usar el alias
 * .thumb_set: Definir como función Thumb (requerido para Cortex-M)
 */

    /* Excepciones del Cortex-M3 */
    .weak   NMI_Handler
    .thumb_set NMI_Handler, Default_Handler

    .weak   HardFault_Handler
    .thumb_set HardFault_Handler, Default_Handler

    .weak   MemManage_Handler
    .thumb_set MemManage_Handler, Default_Handler

    .weak   BusFault_Handler
    .thumb_set BusFault_Handler, Default_Handler

    .weak   UsageFault_Handler
    .thumb_set UsageFault_Handler, Default_Handler

    .weak   SVC_Handler
    .thumb_set SVC_Handler, Default_Handler

    .weak   DebugMon_Handler
    .thumb_set DebugMon_Handler, Default_Handler

    .weak   PendSV_Handler
    .thumb_set PendSV_Handler, Default_Handler

    .weak   SysTick_Handler
    .thumb_set SysTick_Handler, Default_Handler

    /* Interrupciones del LM3S6965 */
    .weak   GPIOA_Handler
    .thumb_set GPIOA_Handler, Default_Handler

    .weak   GPIOB_Handler
    .thumb_set GPIOB_Handler, Default_Handler

    .weak   GPIOC_Handler
    .thumb_set GPIOC_Handler, Default_Handler

    .weak   GPIOD_Handler
    .thumb_set GPIOD_Handler, Default_Handler

    .weak   GPIOE_Handler
    .thumb_set GPIOE_Handler, Default_Handler

    .weak   UART0_Handler
    .thumb_set UART0_Handler, Default_Handler

    .weak   UART1_Handler
    .thumb_set UART1_Handler, Default_Handler

    .weak   SSI0_Handler
    .thumb_set SSI0_Handler, Default_Handler

    .weak   I2C0_Handler
    .thumb_set I2C0_Handler, Default_Handler

    .weak   PWMFault_Handler
    .thumb_set PWMFault_Handler, Default_Handler

    .weak   PWM0_Handler
    .thumb_set PWM0_Handler, Default_Handler

    .weak   PWM1_Handler
    .thumb_set PWM1_Handler, Default_Handler

    .weak   PWM2_Handler
    .thumb_set PWM2_Handler, Default_Handler

    .weak   QEI0_Handler
    .thumb_set QEI0_Handler, Default_Handler

    .weak   ADC0_Handler
    .thumb_set ADC0_Handler, Default_Handler

    .weak   ADC1_Handler
    .thumb_set ADC1_Handler, Default_Handler

    .weak   ADC2_Handler
    .thumb_set ADC2_Handler, Default_Handler

    .weak   ADC3_Handler
    .thumb_set ADC3_Handler, Default_Handler

    .weak   Watchdog_Handler
    .thumb_set Watchdog_Handler, Default_Handler

    .weak   Timer0A_Handler
    .thumb_set Timer0A_Handler, Default_Handler

    .weak   Timer0B_Handler
    .thumb_set Timer0B_Handler, Default_Handler

    .weak   Timer1A_Handler
    .thumb_set Timer1A_Handler, Default_Handler

    .weak   Timer1B_Handler
    .thumb_set Timer1B_Handler, Default_Handler

    .weak   Timer2A_Handler
    .thumb_set Timer2A_Handler, Default_Handler

    .weak   Timer2B_Handler
    .thumb_set Timer2B_Handler, Default_Handler

    .weak   Comp0_Handler
    .thumb_set Comp0_Handler, Default_Handler

    .weak   Comp1_Handler
    .thumb_set Comp1_Handler, Default_Handler

    .weak   Comp2_Handler
    .thumb_set Comp2_Handler, Default_Handler

    .weak   SysCtrl_Handler
    .thumb_set SysCtrl_Handler, Default_Handler

    .weak   FlashCtrl_Handler
    .thumb_set FlashCtrl_Handler, Default_Handler

    .weak   GPIOF_Handler
    .thumb_set GPIOF_Handler, Default_Handler

    .weak   GPIOG_Handler
    .thumb_set GPIOG_Handler, Default_Handler

    .weak   GPIOH_Handler
    .thumb_set GPIOH_Handler, Default_Handler

    .weak   UART2_Handler
    .thumb_set UART2_Handler, Default_Handler

    .weak   SSI1_Handler
    .thumb_set SSI1_Handler, Default_Handler

    .weak   Timer3A_Handler
    .thumb_set Timer3A_Handler, Default_Handler

    .weak   Timer3B_Handler
    .thumb_set Timer3B_Handler, Default_Handler

    .weak   I2C1_Handler
    .thumb_set I2C1_Handler, Default_Handler

    .weak   QEI1_Handler
    .thumb_set QEI1_Handler, Default_Handler

    .weak   CAN0_Handler
    .thumb_set CAN0_Handler, Default_Handler

    .weak   CAN1_Handler
    .thumb_set CAN1_Handler, Default_Handler

    .weak   CAN2_Handler
    .thumb_set CAN2_Handler, Default_Handler

    .weak   Ethernet_Handler
    .thumb_set Ethernet_Handler, Default_Handler

    .weak   Hibernate_Handler
    .thumb_set Hibernate_Handler, Default_Handler
