#include <Arduino.h>

// ---------------- PINES (6 CELDAS) ----------------
#define HX_SCK 4
// Mapeo físico en tu baquela para las 6 celdas: C1, C2, C3, C6, C4, C5
const int DOUT_PINS[6] = {25, 26, 27, 34, 32, 33}; 

// ---------------- CALIBRACIÓN ENVIADA ----------------
const float SLOPES[6] = {
    1.1634e-05, // Celda 1
    1.0954e-05, // Celda 2
    1.1655e-05, // Celda 3
    1.2263e-05, // Celda 6
    1.1655e-05, // Celda 4
    1.1138e-05  // Celda 5
};  

const float OFFSETS[6] = {
    0.6654,     // Celda 1
    1.0817,     // Celda 2
    -2.1184,    // Celda 3
    -1.2472,    // Celda 6
    0.0,        // Celda 4 (Ajustable tras calibrar)
    0.0         // Celda 5 (Ajustable tras calibrar)
};

// ---------------- VARIABLES DE CONTROL ----------------
enum Estado { ESPERANDO, TARANDO, MIDIENDO };
Estado estado_actual = ESPERANDO;

float suma_tara[6] = {0};
float tara_final[6] = {0}; 
long cantidad_lecturas_tara = 0;
unsigned long tiempo_inicio_tara = 0;
unsigned long tiempo_inicio_medicion = 0;

// ---------------- LECTURA PARALELA (6 CELDAS) ----------------
bool readHX711_All(long *raw_data) {
    // Verificar si los 6 módulos están listos (DOUT en LOW)
    for (int i = 0; i < 6; i++) {
        if (digitalRead(DOUT_PINS[i]) == HIGH) return false; 
    }

    for(int i=0; i<6; i++) raw_data[i] = 0;

    // Leer 24 bits en paralelo para las 6 celdas
    for (int i = 0; i < 24; i++) {
        digitalWrite(HX_SCK, HIGH); 
        delayMicroseconds(1);
        for (int c = 0; c < 6; c++) {
            raw_data[c] <<= 1;
            if (digitalRead(DOUT_PINS[c])) raw_data[c]++;
        }
        digitalWrite(HX_SCK, LOW); 
        delayMicroseconds(1);
    }

    // Pulso 25
    digitalWrite(HX_SCK, HIGH); delayMicroseconds(1);
    digitalWrite(HX_SCK, LOW);

    // Extensión de signo para enteros de 24 a 32 bits
    for (int c = 0; c < 6; c++) {
        if (raw_data[c] & 0x800000) raw_data[c] |= 0xFF000000; 
    }
    return true; 
}

void setup() {
    Serial.begin(115200);
    pinMode(HX_SCK, OUTPUT);
    digitalWrite(HX_SCK, LOW);
    for (int i = 0; i < 6; i++) pinMode(DOUT_PINS[i], INPUT);
    
    Serial.println("\n=========================================");
    Serial.println("  MONITOR SERIAL INDEPENDIENTE: 6 CELDAS  ");
    Serial.println("=========================================");
    Serial.println("Instrucciones:");
    Serial.println("2. Envía la letra 'I' para iniciar.");
    Serial.println("=========================================");
}

void loop() {
    if (Serial.available() > 0) {
        char comando = Serial.read();
        if (comando == '\n' || comando == '\r') return; 

        if ((comando == 'I' || comando == 'i') && estado_actual == ESPERANDO) {
            estado_actual = TARANDO;
            tiempo_inicio_tara = millis();
            cantidad_lecturas_tara = 0;
            for(int i=0; i<6; i++) suma_tara[i] = 0; 
            Serial.println("\n# CALCULANDO EL CERO (30s)... NO MUEVAS EL MARCO.");
        }
        else if ((comando == 'F' || comando == 'f') && estado_actual == MIDIENDO) {
            estado_actual = ESPERANDO;
            Serial.println("\n--- MEDICIÓN DETENIDA POR EL USUARIO ---");
        }
    }

    long raw[6];
    
    // TARA (30 segundos)
    if (estado_actual == TARANDO) {
        if (readHX711_All(raw)) {
            for(int i=0; i<6; i++) {
                suma_tara[i] += (raw[i] * SLOPES[i]) + OFFSETS[i];
            }
            cantidad_lecturas_tara++;
        }
        if (millis() - tiempo_inicio_tara >= 30000) {
            for(int i=0; i<6; i++) {
                tara_final[i] = suma_tara[i] / cantidad_lecturas_tara;
            }
            estado_actual = MIDIENDO;
            tiempo_inicio_medicion = millis();
            Serial.println("\nTiempo(ms),Celda1(Kg),Celda2(Kg),Celda3(Kg),Celda6(Kg),Celda4(Kg),Celda5(Kg)");
        }
    } 
    // MEDICIÓN EN TIEMPO REAL
    else if (estado_actual == MIDIENDO) {
        if (readHX711_All(raw)) {
            Serial.print(millis() - tiempo_inicio_medicion);
            
            for(int i=0; i<6; i++) {
                float peso_neto = ((raw[i] * SLOPES[i]) + OFFSETS[i]) - tara_final[i];
                Serial.print(",");
                Serial.print(peso_neto, 3); 
            }
            Serial.println();
        }
    }
}