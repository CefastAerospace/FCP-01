import socket
import time
import os
import json

LOG_FILE = '/mnt/data/mission_log.json'
os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)

aircraft_cache = {}

MISSION_DURATION_SECONDS = 600
mission_start_time = time.time()

s = socket.socket()
s.connect(('127.0.0.1', 30003))

print("Iniciando missão ADS-B (10 minutos)...")

while True:
    try:
        elapsed_time = time.time() - mission_start_time
        if elapsed_time >= MISSION_DURATION_SECONDS and not reported_10min:
                    print("\n" + "="*50)
                    print(f"AVISO: A janela padrão de 10 minutos de missão foi atingida!")
                    print(f"Total de aeronaves únicas rastreadas: {len(aircraft_cache)}")
                    print("O sistema continuará rodando em modo de monitoramento contínuo...")
                    print("="*50 + "\n")
                    reported_10min = True

        line = s.recv(1024).decode(errors='ignore').split('\n')[0]

        if line:
            parts = line.split(',')

            if len(parts) > 15 and parts[0] == 'MSG':
                msg_type = parts[1]
                icao = parts[4]

                if icao not in aircraft_cache:
                    aircraft_cache[icao] = {
                        "icao": icao,
                        "alt": 0,
                        "vel": 0,
                        "lat": None,
                        "lon": None
                    }

                updated = False
                current_timestamp = time.time()

                if msg_type == '3':
                    if parts[11]:
                        aircraft_cache[icao]["alt"] = float(parts[11])
                        updated = True
                    if parts[14] and parts[15]:
                        aircraft_cache[icao]["lat"] = float(parts[14])
                        aircraft_cache[icao]["lon"] = float(parts[15])
                        updated = True

                elif msg_type == '4':
                    if parts[12]:
                        aircraft_cache[icao]["vel"] = float(parts[12])
                        updated = True

                if updated:
                    aircraft_cache[icao]["timestamp"] = current_timestamp

                    plane_data = aircraft_cache[icao]
                    json_record = json.dumps(plane_data, separators=(',',':'))

                    with open(LOG_FILE, 'a') as f:
                        f.write(json_record + '\n')

                    print(f"[{int(elapsed_time)}s] Log -> Aeronave {icao} | Alt: {plane_data['alt']}ft | Vel: {plane_data['vel']}kt | Total Rastreadas: {len(aircraft_cache)}")

    except Exception as e:
        print("Error", e)

    time.sleep(1)