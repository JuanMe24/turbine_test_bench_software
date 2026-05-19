#include <Arduino.h>

// ---------------- PINS ----------------
#define HX_DOUT 32
#define HX_SCK  33

// ---------------- FUNCTION ----------------
long readHX711()
{
    // Wait until data ready
    while (digitalRead(HX_DOUT) == HIGH)
    {
        delayMicroseconds(10);
    }

    long value = 0;

    // Read 24 bits
    for (int i = 0; i < 24; i++)
    {
        digitalWrite(HX_SCK, HIGH);
        delayMicroseconds(1);

        value <<= 1;

        if (digitalRead(HX_DOUT))
            value++;

        digitalWrite(HX_SCK, LOW);
        delayMicroseconds(1);
    }

    // Extra pulse:
    // 1 pulse => next conversion Channel A Gain 128
    digitalWrite(HX_SCK, HIGH);
    delayMicroseconds(1);
    digitalWrite(HX_SCK, LOW);

    // Sign extension
    if (value & 0x800000)
    {
        value |= 0xFF000000;
    }

    return value;
}

// ---------------- SETUP ----------------
void setup()
{
    Serial.begin(115200);

    pinMode(HX_DOUT, INPUT);
    pinMode(HX_SCK, OUTPUT);

    digitalWrite(HX_SCK, LOW);

    Serial.println("HX711 Test");
}

// ---------------- LOOP ----------------
void loop()
{
    long raw = readHX711();

    Serial.print("RAW: ");
    Serial.println(raw);

    delay(100);
}