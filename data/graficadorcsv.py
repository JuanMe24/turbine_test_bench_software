import serial
import csv
import datetime
import os
import time

# --- CONFIGURACIÓN ---
SERIAL_PORT = 'COM6'  
BAUD_RATE = 115200
CARPETA_DATOS = r'C:\Users\juand\Documentos\2026-1\PAI\Cells_prueba\data' 

# Crear la carpeta si no existe
if not os.path.exists(CARPETA_DATOS):
    os.makedirs(CARPETA_DATOS)

# Nombre de archivo único con fecha y hora
nombre_archivo = f"ensayo_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
ruta_completa = os.path.join(CARPETA_DATOS, nombre_archivo)

# Conectar al puerto
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

# IMPORTANTE: Al abrir el puerto serial, el ESP32 se reinicia físicamente.
# Hay que darle 2 segundos para que termine de arrancar antes de hablarle.
time.sleep(2) 

print(f"Grabando datos en: {ruta_completa}")
print("Enviando comando 'I' para iniciar la tara automática...")

# --- AQUÍ ESTÁ LA MAGIA: PYTHON ENVÍA LA 'I' POR TI ---
ser.write(b'I\n')

print("Presiona Ctrl+C en esta consola para detener el guardado.")

try:
    with open(ruta_completa, "w", newline='') as f:
        writer = csv.writer(f)
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                # Imprimir en consola lo que dice el ESP32
                print(f"ESP32: {line}")
                
                # Si la línea tiene comas, es una línea de datos: la guardamos
                if ',' in line:
                    data = line.split(',')
                    writer.writerow(data)
                    f.flush() # Fuerza la escritura al disco
                    
except KeyboardInterrupt:
    print("\nGuardado detenido.")
    # Le decimos al ESP32 que se detenga enviando la 'F'
    ser.write(b'F\n')
finally:
    ser.close()
    print("Archivo cerrado correctamente.")