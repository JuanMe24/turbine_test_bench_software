#include <Arduino.h>

// ---------------- PINES (NUEVA DISTRIBUCIÓN) ----------------
#define HX_SCK 4
// Orden exacto: Celda 1, Celda 2, Celda 3, Celda 6
const int DOUT_PINS[4] = {25, 26, 27, 34}; 

// ---------------- DATOS DE CALIBRACIÓN ----------------
// Corresponden a los valores exactos de tu gráfica para las celdas 1, 2, 3 y 6
const float SLOPES[4] = {
    1.1634e-05, // Celda 1
    1.0954e-05, // Celda 2
    1.1655e-05, // Celda 3
    1.2263e-05  // Celda 6
};

const float OFFSETS[4] = {
    0.6654,     // Celda 1
    1.0817,     // Celda 2
    -2.1184,    // Celda 3
    -1.2472     // Celda 6
};

// ---------------- VARIABLES DE CONTROL ----------------
enum Estado { ESPERANDO, TARANDO, MIDIENDO };
Estado estado_actual = ESPERANDO;

unsigned long tiempo_inicio_tara = 0;
unsigned long tiempo_inicio_medicion = 0;

// Acumuladores para el cálculo del Cero
float suma_tara[4] = {0};
long cantidad_lecturas_tara = 0;
float tara_final[4] = {0}; 

// ---------------- LECTURA PARALELA (4 CELDAS) ----------------
bool readHX711_All(long *raw_data) {
    // Esperar a que los 4 módulos estén listos (DOUT en LOW)
    for (int i = 0; i < 4; i++) {
        if (digitalRead(DOUT_PINS[i]) == HIGH) return false; 
    }

    // Inicializar el arreglo a 0
    for(int i=0; i<4; i++) raw_data[i] = 0;

    // Leer los 24 bits simultáneamente
    for (int i = 0; i < 24; i++) {
        digitalWrite(HX_SCK, HIGH); 
        delayMicroseconds(1);
        
        for (int c = 0; c < 4; c++) {
            raw_data[c] <<= 1;
            if (digitalRead(DOUT_PINS[c])) {
                raw_data[c]++;
            }
        }
        
        digitalWrite(HX_SCK, LOW); 
        delayMicroseconds(1);
    }

    // Pulso 25 (Ganancia 128)
    digitalWrite(HX_SCK, HIGH); delayMicroseconds(1);
    digitalWrite(HX_SCK, LOW);

    // Convertir de 24 bits a 32 bits con signo
    for (int c = 0; c < 4; c++) {
        if (raw_data[c] & 0x800000) { 
            raw_data[c] |= 0xFF000000; 
        }
    }
    return true; 
}

// ---------------- SETUP ----------------
void setup() {
    Serial.begin(115200);
    pinMode(HX_SCK, OUTPUT);
    digitalWrite(HX_SCK, LOW);

    for (int i = 0; i < 4; i++) {
        pinMode(DOUT_PINS[i], INPUT);
    }

    delay(1000);
    Serial.println("\n==================================================");
    Serial.println("   PRUEBA PCB: CELDAS 1, 2, 3 y 6 (Nuevos Pines)  ");
    Serial.println("==================================================");
    Serial.println("Comandos:");
    Serial.println(" -> 'I' para INICIAR (Tara automática de 30s).");
    Serial.println(" -> 'F' para FINALIZAR.");
    Serial.println("==================================================");
}

// ---------------- LOOP ----------------
void loop() {
    // 1. Escuchar el Monitor Serie
    if (Serial.available() > 0) {
        char comando = Serial.read();
        if (comando == '\n' || comando == '\r') return;

        if ((comando == 'I' || comando == 'i') && estado_actual == ESPERANDO) {
            estado_actual = TARANDO;
            tiempo_inicio_tara = millis();
            cantidad_lecturas_tara = 0;
            for(int i=0; i<4; i++) suma_tara[i] = 0; 
            
            Serial.println("\n# CALIBRANDO CERO (30s)... NO TOQUE EL SISTEMA.");
        }
        else if ((comando == 'F' || comando == 'f') && estado_actual == MIDIENDO) {
            estado_actual = ESPERANDO;
            Serial.println("--- FIN DE MEDICIÓN ---");
        }
    }

    long lecturas_raw[4];

    // 2. Control de Estados
    if (estado_actual == TARANDO) {
        if (readHX711_All(lecturas_raw)) {
            for(int i=0; i<4; i++) {
                // Peso Absoluto = (Raw * Slope) + Offset
                float peso_abs = (lecturas_raw[i] * SLOPES[i]) + OFFSETS[i];
                suma_tara[i] += peso_abs;
            }
            cantidad_lecturas_tara++;
        }

        // Finalizar tara a los 30 segundos (30000 ms)
        if (millis() - tiempo_inicio_tara >= 30000) {
            Serial.println("# TARA COMPLETADA. Iniciando CSV...");
            for(int i=0; i<4; i++) {
                tara_final[i] = suma_tara[i] / cantidad_lecturas_tara;
            }
            
            estado_actual = MIDIENDO;
            tiempo_inicio_medicion = millis();
            
            Serial.println("Tiempo_ms,Celda1_Kg,Celda2_Kg,Celda3_Kg,Celda6_Kg");
        }
    }
    else if (estado_actual == MIDIENDO) {
        if (readHX711_All(lecturas_raw)) {
            unsigned long tiempo_actual = millis() - tiempo_inicio_medicion;
            Serial.print(tiempo_actual);
            
            for(int i=0; i<4; i++) {
                // Aplicar calibración y restar el cero calculado
                float peso_abs = (lecturas_raw[i] * SLOPES[i]) + OFFSETS[i];
                float peso_neto = peso_abs - tara_final[i];
                
                Serial.print(",");
                Serial.print(peso_neto, 3); // 3 decimales
            }
            Serial.println();
        }
    }
}