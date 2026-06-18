import serial

ser = serial.Serial(
    "COM6",
    115200,
    timeout=2
)

query = bytes.fromhex(
    "01 03 00 00 00 01 84 0A"
)

ser.write(query)

print(ser.read(100))

ser.close()