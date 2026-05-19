#include <Arduino.h>

// ---------------- PINS ----------------
#define HX_DOUT 32
#define HX_SCK   4

// ---------------- CONFIGURACIÓN DEL ASISTENTE ----------------
const float PESO_CONOCIDO = 2.0;          // <<--- PON AQUÍ EL PESO EXACTO DE TU OBJETO EN KG
const long UMBRAL_RUIDO = 800;            // Tolerancia de ruido para estabilidad
const unsigned long TIEMPO_STABLE = 3000;  // Tiempo de espera (3 segundos)

// ---------------- ESTADOS DEL ASISTENTE ----------------
enum EstadosCalibracion {
    ESTADO_TARA,
    ESTADO_ESPERAR_PESO,
    ESTADO_CALCULAR_FACTOR,
    ESTADO_MONITOR_FINAL
};

EstadosCalibracion estado_actual = ESTADO_TARA;

// ---------------- VARIABLES ----------------
long tare_offset = 0;
long ultimo_valor_raw = 0;
unsigned long cronometro = 0;
float factor_calculado = 1.0;

// ---------------- LECTURA BAJO NIVEL ----------------
long readHX711() {
    while (digitalRead(HX_DOUT) == HIGH) { delayMicroseconds(10); }
    long value = 0;
    for (int i = 0; i < 24; i++) {
        digitalWrite(HX_SCK, HIGH); delayMicroseconds(1);
        value <<= 1;
        if (digitalRead(HX_DOUT)) value++;
        digitalWrite(HX_SCK, LOW); delayMicroseconds(1);
    }
    digitalWrite(HX_SCK, HIGH); delayMicroseconds(1);
    digitalWrite(HX_SCK, LOW);
    if (value & 0x800000) { value |= 0xFF000000; }
    return value;
}

void setup() {
    Serial.begin(115200);
    pinMode(HX_DOUT, INPUT);
    pinMode(HX_SCK, OUTPUT);
    digitalWrite(HX_SCK, LOW);

    Serial.println("\n==================================================");
    Serial.println("     ASISTENTE DE CALIBRACIÓN AUTOMÁTICA          ");
    Serial.println("==================================================");
    Serial.println("PASO 1: Deje la celda completamente LIBRE y VACÍA.");
    Serial.println("Calculando el cero del sistema (Tare)...");
    
    ultimo_valor_raw = readHX711();
}

void loop() {
    long valor_actual_raw = readHX711();
    long diferencia = abs(valor_actual_raw - ultimo_valor_raw);
    bool esta_quieto = (diferencia <= UMBRAL_RUIDO);

    switch (estado_actual) {
        
        case ESTADO_TARA:
            if (esta_quieto) {
                if (cronometro == 0) cronometro = millis();
                else if (millis() - cronometro >= TIEMPO_STABLE) {
                    tare_offset = valor_actual_raw;
                    Serial.println("-> [OK] Cero definido con éxito.");
                    Serial.println("\n==================================================");
                    Serial.print("PASO 2: Coloque el peso conocido de (");
                    Serial.print(PESO_CONOCIDO);
                    Serial.println(" kg) sobre la celda.");
                    Serial.println("Esperando a que detecte el peso y se estabilice...");
                    Serial.println("==================================================");
                    
                    cronometro = 0;
                    estado_actual = ESTADO_ESPERAR_PESO;
                }
            } else {
                cronometro = 0; // Se movió, reiniciar tiempo
            }
            break;

        case ESTADO_ESPERAR_PESO:
            // Detecta si el valor RAW cambió significativamente respecto al cero (pusiste el peso)
            long cambio_respecto_cero = abs(valor_actual_raw - tare_offset);
            
            if (cambio_respecto_cero > (UMBRAL_RUIDO * 5) && esta_quieto) {
                if (cronometro == 0) cronometro = millis();
                else if (millis() - cronometro >= TIEMPO_STABLE) {
                    estado_actual = ESTADO_CALCULAR_FACTOR;
                }
            } else {
                cronometro = 0;
            }
            break;

        case ESTADO_CALCULAR_FACTOR: {
            long neto_con_peso = valor_actual_raw - tare_offset;
            
            // Fórmula: Factor = Delta RAW / Peso Real
            factor_calculado = (float)neto_con_peso / PESO_CONOCIDO;
            
            Serial.println("\n>>> ¡CALIBRACIÓN COMPLETADA! <<<");
            Serial.println("--------------------------------------------------");
            Serial.print("Para esta celda, copie este parámetro:\n\n");
            Serial.print("  const float FACTOR_CALIBRACION = ");
            Serial.print(factor_calculado, 2);
            Serial.println(".0;\n");
            Serial.println("--------------------------------------------------");
            Serial.println("Iniciando modo monitor en tiempo real (unidades en KG):");
            
            estado_actual = ESTADO_MONITOR_FINAL;
            break;
        }

        case ESTADO_MONITOR_FINAL: {
            long valor_neto = valor_actual_raw - tare_offset;
            float peso_kg = (float)valor_neto / factor_calculado;
            
            Serial.print("Fuerza Real: ");
            Serial.print(peso_kg, 3);
            Serial.println(" kg");
            delay(100); // Para lectura cómoda en pantalla
            break;
        }
    }

    ultimo_valor_raw = valor_actual_raw;
}