    #include <Arduino.h>

    // ---------------- PINES ----------------
    #define HX_DOUT 32
    #define HX_SCK  33 // Recuerda usar el pin 4 (u otro de salida), NO el 34.

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

    // ---------------- SETUP ----------------
    void setup() {
        Serial.begin(115200);
        pinMode(HX_DOUT, INPUT);
        pinMode(HX_SCK, OUTPUT);
        digitalWrite(HX_SCK, LOW);

        // Esperar un momento a que abra la terminal
        delay(1000); 

        Serial.println("\n==================================================");
        Serial.println("      RECOLECTOR DE DATOS MULTIPUNTO (CSV)        ");
        Serial.println("==================================================");
        Serial.println("INSTRUCCIONES:");
        Serial.println("1. Coloque un peso en la celda (empiece con 0 kg).");
        Serial.println("2. Escriba el valor del peso en la barra de arriba y presione Enter.");
        Serial.println("   (Ejemplo: 0 o 10 o 30.5)");
        Serial.println("3. El sistema tomará muestras por 5 segundos.");
        Serial.println("==================================================");
        
        // Encabezado del CSV para que Excel lo reconozca
        Serial.println("\n--- COPIAR DESDE LA SIGUIENTE LÍNEA ---");
        Serial.println("Peso_Kg,Valor_RAW");
    }

    // ---------------- LOOP ----------------
    void loop() {
        // Verificar si ingresaste un número por el teclado
        if (Serial.available() > 0) {
            // Leer el número decimal que ingresaste
            float peso_ingresado = Serial.parseFloat();
            
            // Limpiar cualquier residuo en el buffer (como saltos de línea)
            while(Serial.available() > 0) { Serial.read(); }

            Serial.print("# Tomando muestras para ");
            Serial.print(peso_ingresado, 2);
            Serial.println(" kg... No mueva el sensor.");

            long suma = 0;
            int cantidad_lecturas = 0;
            unsigned long tiempo_inicio = millis();

            // Descartar la primera lectura por estabilidad
            readHX711(); 

            // Capturar datos durante exactamente 5 segundos
            while (millis() - tiempo_inicio < 5000) {
                suma += readHX711();
                cantidad_lecturas++;
            }

            if (cantidad_lecturas > 0) {
                long promedio_raw = suma / cantidad_lecturas;
                
                // ESTA ES LA LÍNEA FORMATO CSV: Peso,ValorRAW
                Serial.print(peso_ingresado, 2);
                Serial.print(",");
                Serial.println(promedio_raw);
            }
            
            Serial.println("# Listo. Coloque el siguiente peso e ingrese el valor.");
        }
    }