#include <Arduino.h>

// ---------------- PINES ----------------
#define HX_SCK 4
// Se agregan los pines 25 y 26 para las celdas 7 y 8
const int DOUT_PINS[8] = {32, 33, 34, 35, 36, 39, 25, 26};

// ---------------- DATOS DE CALIBRACIÓN ----------------
// Celdas 1 a 6: Tus datos reales.
// Celdas 7 y 8: Valores provisionales (cámbialos cuando calibres las de 20kg).
const float SLOPES[8] = {
    1.1634e-05, 1.0954e-05, 1.1655e-05, 1.0539e-05, 1.1138e-05, 1.2263e-05, 
    1.0000e-05, 1.0000e-05  // <-- Celda 7 y 8 provisionales
};

const float OFFSETS[8] = {
    0.6654, 1.0817, -2.1184, -0.24656, -2.1766, -1.2472, 
    0.0,    0.0             // <-- Celda 7 y 8 provisionales
};

// ---------------- VARIABLES DE CONTROL ----------------
enum Estado { ESPERANDO, TARANDO, MIDIENDO };
Estado estado_actual = ESPERANDO;

unsigned long tiempo_inicio_tara = 0;
unsigned long tiempo_inicio_medicion = 0;

// Acumuladores para los 30 segundos de tara (ahora para 8 celdas)
float suma_tara[8] = {0};
long cantidad_lecturas_tara = 0;
float tara_final[8] = {0}; 

// ---------------- LECTURA PARALELA BAJO NIVEL (8 CELDAS) ----------------
bool readHX711_All(long *raw_data) {
    // Esperar a que las 8 celdas estén listas (DOUT en LOW)
    for (int i = 0; i < 8; i++) {
        if (digitalRead(DOUT_PINS[i]) == HIGH) return false; 
    }

    // Inicializar el arreglo a 0
    for(int i=0; i<8; i++) raw_data[i] = 0;

    // Leer los 24 bits de las 8 celdas al mismo tiempo
    for (int i = 0; i < 24; i++) {
        digitalWrite(HX_SCK, HIGH); 
        delayMicroseconds(1);
        
        for (int c = 0; c < 8; c++) {
            raw_data[c] <<= 1;
            if (digitalRead(DOUT_PINS[c])) {
                raw_data[c]++;
            }
        }
        
        digitalWrite(HX_SCK, LOW); 
        delayMicroseconds(1);
    }

    // Pulso 25 para fijar la ganancia en 128
    digitalWrite(HX_SCK, HIGH); delayMicroseconds(1);
    digitalWrite(HX_SCK, LOW);

    // Extensión de signo de 24 a 32 bits con signo para las 8 celdas
    for (int c = 0; c < 8; c++) {
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

    for (int i = 0; i < 8; i++) {
        pinMode(DOUT_PINS[i], INPUT);
    }

    delay(1000);
    Serial.println("\n==================================================");
    Serial.println("   SISTEMA MAESTRO: 8 CELDAS (LECTURA PARALELA)   ");
    Serial.println("==================================================");
    Serial.println("Comandos:");
    Serial.println(" -> Envíe 'I' para INICIAR (Hará tara por 30s).");
    Serial.println(" -> Envíe 'F' para FINALIZAR medición.");
    Serial.println("==================================================");
}

// ---------------- LOOP ----------------
void loop() {
    // 1. Gestión de Comandos por Serial
    if (Serial.available() > 0) {
        char comando = Serial.read();
        if (comando == '\n' || comando == '\r') return;

        if ((comando == 'I' || comando == 'i') && estado_actual == ESPERANDO) {
            estado_actual = TARANDO;
            tiempo_inicio_tara = millis();
            cantidad_lecturas_tara = 0;
            for(int i=0; i<8; i++) suma_tara[i] = 0; 
            
            Serial.println("\n# INICIANDO TARA DE 30 SEGUNDOS... NO TOQUE EL BANCO DE PRUEBAS.");
        }
        else if ((comando == 'F' || comando == 'f') && estado_actual == MIDIENDO) {
            estado_actual = ESPERANDO;
            Serial.println("--- FIN DEL CSV ---");
            Serial.println("# Medición detenida. Envíe 'I' para un nuevo ensayo.");
        }
    }

    // Arreglo para guardar las 8 lecturas crudas
    long lecturas_raw[8];

    // 2. Máquina de Estados
    if (estado_actual == TARANDO) {
        if (readHX711_All(lecturas_raw)) {
            // Calcular el peso absoluto base y acumularlo para el promedio
            for(int i=0; i<8; i++) {
                float peso_abs = (lecturas_raw[i] * SLOPES[i]) + OFFSETS[i];
                suma_tara[i] += peso_abs;
            }
            cantidad_lecturas_tara++;
        }

        // Verificar si se cumplieron los 30 segundos
        if (millis() - tiempo_inicio_tara >= 30000) {
            Serial.println("# TARA COMPLETADA EXITOSAMENTE.");
            for(int i=0; i<8; i++) {
                tara_final[i] = suma_tara[i] / cantidad_lecturas_tara;
            }
            
            estado_actual = MIDIENDO;
            tiempo_inicio_medicion = millis();
            
            Serial.println("\n--- INICIO DEL CSV ---");
            Serial.println("Tiempo_ms,C1_Kg,C2_Kg,C3_Kg,C4_Kg,C5_Kg,C6_Kg,C7_Kg,C8_Kg");
        }
    }
    else if (estado_actual == MIDIENDO) {
        if (readHX711_All(lecturas_raw)) {
            unsigned long tiempo_actual = millis() - tiempo_inicio_medicion;
            Serial.print(tiempo_actual);
            
            for(int i=0; i<8; i++) {
                // Peso Neto = (RAW * Slope + Offset) - Tara_Inicial
                float peso_abs = (lecturas_raw[i] * SLOPES[i]) + OFFSETS[i];
                float peso_neto = peso_abs - tara_final[i];
                
                Serial.print(",");
                Serial.print(peso_neto, 3); 
            }
            Serial.println();
        }
    }
}