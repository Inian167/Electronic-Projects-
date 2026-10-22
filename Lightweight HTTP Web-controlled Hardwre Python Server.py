import socket
import serial
import time
import sys

# ========== CONFIGURATION ==========
SERIAL_PORT = 'COM12'     # Your Arduino Nano CH340 COM port
BAUD_RATE = 9600
HTTP_PORT = 8080
# ===================================

# === Connect to Arduino ===
try:
    arduino = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)  # Let Arduino reset
    print(f"[✔] Arduino connected on {SERIAL_PORT}")
except serial.SerialException:
    print(f"[✘] Could not connect to Arduino on {SERIAL_PORT}")
    print("[⚠] Please check the USB cable or try a different COM port.")
    sys.exit(1)

# === Start HTTP server ===
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind(('0.0.0.0', HTTP_PORT))  # Accept requests from all network IPs
server_socket.listen(1)

print(f"[✔] HTTP server running at:")
print(f"     http://localhost:{HTTP_PORT}/")
print(f"     http://192.168.162.26:{HTTP_PORT}/   ← From other devices")

# === Server loop ===
while True:
    client, address = server_socket.accept()
    print(f"\n[📶] Connection from {address}")

    request = client.recv(1024).decode()
    print("[📝] Request received:")
    print(request)

    message = "Use /on or /off to control the LED"

    if "GET /on" in request:
        arduino.write(b'1')
        message = "✅ LED turned ON"
    elif "GET /off" in request:
        arduino.write(b'0')
        message = "❌ LED turned OFF"

    # Send HTML response
    html = f"""\
HTTP/1.1 200 OK\r
Content-Type: text/html\r
\r
<html>
  <head>
    <title>LED Control</title>
    <style>
      body {{ font-family: Arial; text-align: center; padding-top: 40px; background: #f4f4f4; }}
      h1 {{ color: #333; }}
      button {{
        font-size: 20px; padding: 15px 30px; margin: 20px;
        border: none; border-radius: 8px;
      }}
      .on {{ background: green; color: white; }}
      .off {{ background: red; color: white; }}
    </style>
  </head>
  <body>
    <h1>{message}</h1>
    <a href="/on"><button class="on">Turn ON</button></a>
    <a href="/off"><button class="off">Turn OFF</button></a>
    <p>Accessed from {address[0]}</p>
  </body>
</html>
"""

    client.sendall(html.encode())
    client.close()
