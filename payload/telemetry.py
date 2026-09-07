import socket
import time
import os

LOG_FILE = '/mnt/data/mission_log.json'

os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)

s = socket.socket()
s.connect(('127.0.0.1', 30003))

print("Procurando por telemetria de aviões próximos...")

while True:
    try:
        line = s.recv(1024).decode(errors='ignore').split('\n')[0]
        if line:
            parts = line.split(',')
            if len(parts) > 4 and parts[0] == 'MSG':
                msg_type = parts[1]
                icao = parts[4]

                if msg_type in ['3', '4']:
                    with open(LOG_FILE, 'a') as f:
                        f.write(line + '\n')
                    print(f"Adquirido tipo {msg_type} da aeronave {icao}")
    except Exception as e:
        print("Error", e)
    time.sleep(1)