/*
 * Sensor Simulator Implementation
 * 
 * Implementa simuladores realistas de sensores médicos.
 */

#include "sensor_simulator.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

/*
 * ============================================================================
 * GENERADOR DE NÚMEROS PSEUDOALEATORIOS
 * ============================================================================
 * 
 * Implementa un Linear Congruential Generator (LCG) simple.
 * Fórmula: next = (a * seed + c) mod m
 * 
 * Parámetros (Numerical Recipes):
 * - a = 1664525
 * - c = 1013904223
 * - m = 2^32 (implícito en uint32_t)
 */

static uint32_t rand_seed = 12345;      /* Semilla inicial */

/**
 * @brief Actualiza y retorna el siguiente número pseudoaleatorio
 */
static uint32_t rand_next(void) {
    rand_seed = (1664525UL * rand_seed + 1013904223UL);
    return rand_seed;
}

/**
 * @brief Inicializa el generador con una semilla basada en el tiempo
 */
void sensor_simulator_init(void) {
    /* Usar tick count de FreeRTOS como semilla */
    rand_seed = (uint32_t)xTaskGetTickCount();
    
    /* Mezclar un poco más la semilla */
    rand_next();
    rand_next();
}

/**
 * @brief Genera entero aleatorio en [0, max-1]
 */
uint32_t random_int(uint32_t max) {
    if (max == 0) return 0;
    return rand_next() % max;
}

/**
 * @brief Genera float aleatorio en [min, max]
 */
float random_float(float min, float max) {
    /* Normalizar a [0, 1] */
    float normalized = (float)rand_next() / (float)UINT32_MAX;
    
    /* Escalar a [min, max] */
    return min + normalized * (max - min);
}

/*
 * ============================================================================
 * SIMULADORES DE SENSORES MÉDICOS
 * ============================================================================
 */

/**
 * @brief Simula sensor de frecuencia cardíaca (HR)
 * 
 * Comportamiento:
 * - 95% del tiempo: valores normales (60-100 bpm)
 * - 5% del tiempo: valores anormales/taquicardia (120-140 bpm)
 * 
 * Características de la simulación:
 * - Variación fisiológica natural
 * - Transiciones realistas entre valores
 * - Eventos anormales esporádicos para testear alarmas
 */
float sensor_read_heart_rate(void) {
    float hr;
    
    /* Determinar si generar valor anormal (5% probabilidad) */
    if (random_int(ANOMALY_PROBABILITY) == 0) {
        /* Valor anormal - taquicardia */
        hr = random_float(HR_ABNORMAL_MIN, HR_ABNORMAL_MAX);
    } else {
        /* Valor normal */
        hr = random_float(HR_NORMAL_MIN, HR_NORMAL_MAX);
    }
    
    /* Redondear a 1 decimal (precisión típica de monitor cardíaco) */
    hr = ((int)(hr * 10.0f)) / 10.0f;
    
    return hr;
}

/**
 * @brief Simula sensor de saturación de oxígeno (SpO2)
 * 
 * Comportamiento:
 * - 95% del tiempo: valores normales (95-100%)
 * - 5% del tiempo: valores anormales/hipoxia (85-94%)
 * 
 * Características de la simulación:
 * - SpO2 es muy estable en condiciones normales
 * - Valores típicos: 97-99%
 * - Desaturaciones ocasionales para testear alarmas
 */
float sensor_read_spo2(void) {
    float spo2;
    
    /* Determinar si generar valor anormal (5% probabilidad) */
    if (random_int(ANOMALY_PROBABILITY) == 0) {
        /* Valor anormal - hipoxia */
        spo2 = random_float(SPO2_ABNORMAL_MIN, SPO2_ABNORMAL_MAX);
    } else {
        /* Valor normal - sesgar hacia el extremo alto (más realista) */
        spo2 = random_float(97.0f, SPO2_NORMAL_MAX);
    }
    
    /* Redondear a 1 decimal */
    spo2 = ((int)(spo2 * 10.0f)) / 10.0f;
    
    return spo2;
}

/**
 * @brief Simula sensor de temperatura corporal
 * 
 * Comportamiento:
 * - 95% del tiempo: valores normales (36.0-37.5°C)
 * - 5% del tiempo: valores anormales/fiebre (38.0-39.0°C)
 * 
 * Características de la simulación:
 * - Temperatura corporal es muy estable
 * - Cambios lentos y graduales
 * - Precisión típica: 0.1°C
 * - Episodios de fiebre ocasionales
 */
float sensor_read_temperature(void) {
    float temp;
    
    /* Determinar si generar valor anormal (5% probabilidad) */
    if (random_int(ANOMALY_PROBABILITY) == 0) {
        /* Valor anormal - fiebre */
        temp = random_float(TEMP_ABNORMAL_MIN, TEMP_ABNORMAL_MAX);
    } else {
        /* Valor normal - temperatura típica es 36.5-37.0°C */
        temp = random_float(TEMP_NORMAL_MIN, TEMP_NORMAL_MAX);
    }
    
    /* Redondear a 1 decimal (precisión de termómetro digital) */
    temp = ((int)(temp * 10.0f)) / 10.0f;
    
    return temp;
}
