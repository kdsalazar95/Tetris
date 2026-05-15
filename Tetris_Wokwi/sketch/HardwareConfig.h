#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <Arduino.h>

// ============================================
// PINES PARA BOTONES (GPIOs seguros)
// ============================================

// Botones en el lado DERECHO del board (fácil acceso)
#define BTN_LEFT      25   // GPIO25 - Mover izquierda
#define BTN_RIGHT     26   // GPIO26 - Mover derecha
#define BTN_DOWN      27   // GPIO27 - Bajar rápido
#define BTN_ROTATE    14   // GPIO14 - Rotar pieza

// Botón opcional de pausa
#define BTN_PAUSE     12   // GPIO12 - Pausar juego

// Buzzer (opcional - para efectos de sonido)
#define BUZZER_PIN    13   // GPIO13 - Buzzer

// Alias para compatibilidad
#define BUTTON_LEFT    BTN_LEFT
#define BUTTON_RIGHT   BTN_RIGHT
#define BUTTON_DOWN    BTN_DOWN
#define BUTTON_ROTATE  BTN_ROTATE
#define BUTTON_DROP    BTN_DOWN
#define BUTTON_PAUSE   BTN_PAUSE

// ============================================
// CONFIGURACIÓN DE DISPLAY ILI9341
// ============================================

// PINES SPI PARA DISPLAY (usar HSPI por defecto)
// Estos son los pines por defecto de TFT_eSPI para ESP32
#define TFT_MISO      19   // GPIO19 (no usado si solo escribes)
#define TFT_MOSI      23   // GPIO23 - Data Out
#define TFT_SCLK      18   // GPIO18 - Clock
#define TFT_CS        15   // GPIO15 - Chip Select
#define TFT_DC        2    // GPIO2  - Data/Command
#define TFT_RST       4    // GPIO4  - Reset

// ============================================
// PINES QUE DEBES EVITAR
// ============================================

/*
 * NO USAR ESTOS PINES EN ESP32:
 * 
 * GPIO 0:  Boot mode (strapping pin)
 * GPIO 1:  TX0 (Serial Debug)
 * GPIO 3:  RX0 (Serial Debug)
 * GPIO 5:  Strapping pin
 * GPIO 6-11: Flash integrada (SPI Flash)
 * GPIO 34-39: Solo INPUT (ADC, no pullup/pulldown)
 */

// ============================================
// INICIALIZACIÓN
// ============================================

void initButtons() {
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_ROTATE, INPUT_PULLUP);
    pinMode(BTN_PAUSE, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    
    digitalWrite(BUZZER_PIN, LOW); // Buzzer apagado
    
    Serial.println("[HW] Botones inicializados en pines:");
    Serial.printf("  LEFT:   GPIO%d\n", BTN_LEFT);
    Serial.printf("  RIGHT:  GPIO%d\n", BTN_RIGHT);
    Serial.printf("  DOWN:   GPIO%d\n", BTN_DOWN);
    Serial.printf("  ROTATE: GPIO%d\n", BTN_ROTATE);
    Serial.printf("  PAUSE:  GPIO%d\n", BTN_PAUSE);
    Serial.printf("  BUZZER: GPIO%d\n", BUZZER_PIN);
}

// ============================================
// FUNCIONES DE SONIDO (opcional)
// ============================================

void playTone(int frequency, int duration) {
    tone(BUZZER_PIN, frequency, duration);
}

void playRotateSound() {
    playTone(800, 20);
}

void playLockSound() {
    playTone(200, 50);
}

void playLineClearSound() {
    playTone(1500, 200);
}

void playGameOverSound() {
    playTone(100, 1000);
}

#endif