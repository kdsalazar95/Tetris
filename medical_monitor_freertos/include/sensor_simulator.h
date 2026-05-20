/*
 * Sensor Simulator Header
 * 
 * Simula sensores médicos realistas para el monitor médico.
 * Genera valores con variación fisiológica natural y eventos anormales ocasionales.
 */

#ifndef SENSOR_SIMULATOR_H
#define SENSOR_SIMULATOR_H

#include <stdint.h>

/*
 * ============================================================================
 * RANGOS NORMALES DE SIGNOS VITALES
 * ============================================================================
 */

/* Heart Rate (Frecuencia Cardíaca) */
#define HR_NORMAL_MIN               60.0f       /* bpm mínimo normal */
#define HR_NORMAL_MAX               100.0f      /* bpm máximo normal */
#define HR_ABNORMAL_MIN             120.0f      /* bpm mínimo anormal (taquicardia) */
#define HR_ABNORMAL_MAX             140.0f      /* bpm máximo anormal */

/* SpO2 (Saturación de Oxígeno) */
#define SPO2_NORMAL_MIN             95.0f       /* % mínimo normal */
#define SPO2_NORMAL_MAX             100.0f      /* % máximo normal */
#define SPO2_ABNORMAL_MIN           85.0f       /* % mínimo anormal (hipoxia) */
#define SPO2_ABNORMAL_MAX           94.0f       /* % máximo anormal */

/* Temperature (Temperatura Corporal) */
#define TEMP_NORMAL_MIN             36.0f       /* °C mínimo normal */
#define TEMP_NORMAL_MAX             37.5f       /* °C máximo normal */
#define TEMP_ABNORMAL_MIN           38.0f       /* °C mínimo anormal (fiebre) */
#define TEMP_ABNORMAL_MAX           39.0f       /* °C máximo anormal */

/*
 * ============================================================================
 * PROBABILIDADES DE EVENTOS ANORMALES
 * ============================================================================
 */

/* Probabilidad de generar valor anormal (1 en N lecturas) */
#define ANOMALY_PROBABILITY         20          /* 1 en 20 = 5% de probabilidad */

/*
 * ============================================================================
 * FUNCIONES DE SIMULACIÓN
 * ============================================================================
 */

/**
 * @brief Inicializa el generador de números pseudoaleatorios
 * 
 * Debe llamarse una vez al inicio del programa para configurar la semilla
 * del generador. Usa el tick count de FreeRTOS como semilla.
 */
void sensor_simulator_init(void);

/**
 * @brief Simula lectura de sensor de frecuencia cardíaca
 * 
 * Genera valores en el rango normal (60-100 bpm) con variación natural.
 * Ocasionalmente (5% de probabilidad) genera valores anormales (120-140 bpm)
 * para simular taquicardia.
 * 
 * Características:
 * - Variación realista dentro del rango
 * - Transiciones suaves entre valores
 * - Eventos anormales esporádicos
 * 
 * @return Frecuencia cardíaca simulada en bpm
 */
float sensor_read_heart_rate(void);

/**
 * @brief Simula lectura de sensor de saturación de oxígeno
 * 
 * Genera valores en el rango normal (95-100%) con variación natural.
 * Ocasionalmente (5% de probabilidad) genera valores anormales (85-94%)
 * para simular hipoxia.
 * 
 * Características:
 * - Alta precisión (valores típicos 97-99%)
 * - Poca variación (SpO2 es estable en personas sanas)
 * - Eventos de desaturación esporádicos
 * 
 * @return Saturación de oxígeno simulada en porcentaje
 */
float sensor_read_spo2(void);

/**
 * @brief Simula lectura de sensor de temperatura
 * 
 * Genera valores en el rango normal (36.0-37.5°C) con variación natural.
 * Ocasionalmente (5% de probabilidad) genera valores anormales (38.0-39.0°C)
 * para simular fiebre.
 * 
 * Características:
 * - Cambios muy lentos (temperatura corporal es muy estable)
 * - Precisión típica de termómetro (0.1°C)
 * - Eventos de fiebre esporádicos
 * 
 * @return Temperatura corporal simulada en grados Celsius
 */
float sensor_read_temperature(void);

/*
 * ============================================================================
 * UTILIDADES INTERNAS (NO USAR DIRECTAMENTE)
 * ============================================================================
 */

/**
 * @brief Genera número pseudoaleatorio en rango [min, max]
 * 
 * Implementa un generador lineal congruencial simple.
 * No es criptográficamente seguro, pero suficiente para simulación.
 * 
 * @param min Valor mínimo del rango
 * @param max Valor máximo del rango
 * @return Número float aleatorio en [min, max]
 */
float random_float(float min, float max);

/**
 * @brief Genera número entero pseudoaleatorio en rango [0, max-1]
 * 
 * @param max Límite superior (exclusivo)
 * @return Número entero aleatorio en [0, max-1]
 */
uint32_t random_int(uint32_t max);

#endif /* SENSOR_SIMULATOR_H */
