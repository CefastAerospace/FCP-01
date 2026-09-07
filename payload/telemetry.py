import socket
import time
import os
import json

LOG_FILE = '/mnt/data/mission_log.json'

os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)

aicraft_cache = {}

s = socket.socket()
s.connect(('127.0.0.1', 30003))

print("Procurando por telemetria de aviões próximos, e criando log...")

while True:
    try:
        line = s.recv(1024).decode(errors='ignore').split('\n')[0]
        if line:
            parts = line.split(',')

            if len(parts) > 12 and parts[0] == 'MSG':
                msg_type = parts[1]
                icao = parts[4]

                if icao not in aicraft_cache:
                    aicraft_cache[icao] = {
                        "icao": icao,
                        "alt": 0,
                        "vel": 0,
                        "lat": None,
                        "lon": None
                    }
                
                if msg_type == '3':
                    if parts[11]:
                        aicraft_cache[icao]["alt"] = float(parts[11])
                    if parts[14] and parts[15]:
                        aicraft_cache[icao]["lat"] = float(parts[14])
                        aicraft_cache[icao]["lon"] = float(parts[15])

                elif msg_type == '4':
                    if parts[12]:
                        aicraft_cache[icao]["vel"] = float(parts[12])

                plane_data = aicraft_cache[icao]
                json_record = json.dumps(plane_data)

                with open(LOG_FILE, 'a') as f:
                    f.write(json_record + '\n')
                print(f"Log -> Aeronave {icao} | Alt: {plane_data['alt']}ft | Vel: {plane_data['vel']}kt")

    except Exception as e:
        print("Error", e)
    time.sleep(1)