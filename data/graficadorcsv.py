import serial
import csv
import datetime
import os
import time

# --- CONFIGURACIÓN DEL EXPERIMENTO ---
SERIAL_PORT = 'COM3'  # Verifica que siga siendo el COM6
BAUD_RATE = 115200
CARPETA_DATOS = r'C:\Users\juand\Documentos\2026-1\PAI\Cells_prueba\data'

# Crear la carpeta si el sistema no la encuentra
if not os.path.exists(CARPETA_DATOS):
    os.makedirs(CARPETA_DATOS)

# Generar un nombre único para evitar sobrescribir datos valiosos
nombre_archivo = f"exp_paper_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
ruta_completa = os.path.join(CARPETA_DATOS, nombre_archivo)

try:
    # 1. Abrir conexión
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Conectado a {SERIAL_PORT}. Reiniciando ESP32...")
    
    # IMPORTANTE: Al abrir el puerto, el ESP32 hace un reset físico. 
    # Esperamos 2.5 segundos para que termine de arrancar.
    time.sleep(2.5) 
    
    # 2. Iniciar experimento
    print("Enviando comando de tara ('I')...")
    ser.write(b'I\n')
    
    print(f"\n>>> Grabando datos en: {ruta_completa}")
    print(">>> Presiona Ctrl+C en esta consola para detener el ensayo de forma segura.\n")
    
    # 3. Captura de datos
    with open(ruta_completa, "w", newline='') as f:
        writer = csv.writer(f)
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                # Mostrar en pantalla lo que está pasando
                print(line)
                
                # Si la línea tiene comas, asumimos que es la tira de datos: Tiempo,C1,C2...
                if ',' in line:
                    data = line.split(',')
                    writer.writerow(data)
                    f.flush() # FORZAR GUARDADO: Si se va la luz, no pierdes lo anterior.

except KeyboardInterrupt:
    print("\nAdquisición detenida por el usuario (Ctrl+C).")
    try:
        ser.write(b'F\n') # Intentar decirle al ESP32 que pare
    except:
        pass
except serial.SerialException as e:
    print(f"\nERROR DE PUERTO: {e}")
    print("¡Asegúrate de haber cerrado el Monitor Serie en VS Code!")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
    print("\nPuerto serial liberado. Archivo CSV cerrado y listo para Excel/Matlab.")