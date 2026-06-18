#include <Arduino.h>

// ---------------- PINES (8 CELDAS) ----------------
#define HX_SCK 4
// Mapeo: 1, 2, 3, 6 (Originales) | 4, 5, 7, 8 (Nuevas)
// Pines de tu baquela: D25, D26, D27, D34, D32, D33, D35, VP(36)
const int DOUT_PINS[8] = {25, 26, 27, 34, 32, 33, 35, 36}; 

// ---------------- CALIBRACIÓN ----------------
const float SLOPES[8] = {
    1.1634e-05, // Celda 1
    1.0954e-05, // Celda 2
    1.1655e-05, // Celda 3
    1.2263e-05, // Celda 6
    1.1655e-05,    // Celda 4 (¡Por calibrar!)
    1.1138e-05,    // Celda 5 (¡Por calibrar!)
    1.0e-05,    // Celda 7 (¡Por calibrar!)
    1.0e-05     // Celda 8 (¡Por calibrar!)
};

const float OFFSETS[8] = {
    0.6654,     // Celda 1
    1.0817,     // Celda 2
    -2.1184,    // Celda 3
    -1.2472,    // Celda 6
    0.0,        // Celda 4 (¡Por calibrar!)
    0.0,        // Celda 5 (¡Por calibrar!)
    0.0,        // Celda 7 (¡Por calibrar!)
    0.0         // Celda 8 (¡Por calibrar!)
};

// ---------------- VARIABLES ----------------
enum Estado { ESPERANDO, TARANDO, MIDIENDO };
Estado estado_actual = ESPERANDO;

float suma_tara[8] = {0};
float tara_final[8] = {0}; 
long cantidad_lecturas_tara = 0;
unsigned long tiempo_inicio_tara = 0;
unsigned long tiempo_inicio_medicion = 0;

// ---------------- LECTURA PARALELA (8 CELDAS) ----------------
bool readHX711_All(long *raw_data) {
    for (int i = 0; i < 8; i++) {
        if (digitalRead(DOUT_PINS[i]) == HIGH) return false; 
    }

    for(int i=0; i<8; i++) raw_data[i] = 0;

    for (int i = 0; i < 24; i++) {
        digitalWrite(HX_SCK, HIGH); 
        delayMicroseconds(1);
        for (int c = 0; c < 8; c++) {
            raw_data[c] <<= 1;
            if (digitalRead(DOUT_PINS[c])) raw_data[c]++;
        }
        digitalWrite(HX_SCK, LOW); 
        delayMicroseconds(1);
    }

    // Pulso 25
    digitalWrite(HX_SCK, HIGH); delayMicroseconds(1);
    digitalWrite(HX_SCK, LOW);

    for (int c = 0; c < 8; c++) {
        if (raw_data[c] & 0x800000) raw_data[c] |= 0xFF000000; 
    }
    return true; 
}

void setup() {
    Serial.begin(115200);
    pinMode(HX_SCK, OUTPUT);
    digitalWrite(HX_SCK, LOW);
    for (int i = 0; i < 8; i++) pinMode(DOUT_PINS[i], INPUT);
    
    Serial.println("=========================================");
    Serial.println("SISTEMA DE 8 CELDAS LISTO.");
    Serial.println("Envíe 'I' para Tara Automática (30s).");
    Serial.println("=========================================");
}

void loop() {
    if (Serial.available() > 0) {
        char comando = Serial.read();
        if ((comando == 'I' || comando == 'i') && estado_actual == ESPERANDO) {
            estado_actual = TARANDO;
            tiempo_inicio_tara = millis();
            cantidad_lecturas_tara = 0;
            for(int i=0; i<8; i++) suma_tara[i] = 0; 
            Serial.println("# CALIBRANDO CERO (30s)... NO TOQUE EL SISTEMA.");
        }
        else if ((comando == 'F' || comando == 'f') && estado_actual == MIDIENDO) {
            estado_actual = ESPERANDO;
            Serial.println("--- FIN DE MEDICIÓN ---");
        }
    }

    long raw[8];
    if (estado_actual == TARANDO) {
        if (readHX711_All(raw)) {
            for(int i=0; i<8; i++) suma_tara[i] += (raw[i] * SLOPES[i]) + OFFSETS[i];
            cantidad_lecturas_tara++;
        }
        if (millis() - tiempo_inicio_tara >= 30000) {
            for(int i=0; i<8; i++) tara_final[i] = suma_tara[i] / cantidad_lecturas_tara;
            estado_actual = MIDIENDO;
            tiempo_inicio_medicion = millis();
            Serial.println("Tiempo,C1,C2,C3,C6,C4,C5,C7,C8");
        }
    } else if (estado_actual == MIDIENDO) {
        if (readHX711_All(raw)) {
            Serial.print(millis() - tiempo_inicio_medicion);
            for(int i=0; i<8; i++) {
                Serial.print(",");
                Serial.print(((raw[i] * SLOPES[i]) + OFFSETS[i]) - tara_final[i], 3);
            }
            Serial.println();
        }
    }
}