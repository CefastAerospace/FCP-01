import socket
import time
import os
import json
from datetime import datetime, timezone
from dataclasses import dataclass, asdict

LOG_FILE = '/mnt/data/mission_log.json'
os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)

@dataclass
class AircraftTelemetry:
    icao: str
    timestamp: str = None
    lat: float = None
    lon: float = None
    alt: float = 0.0
    vel: float = 0.0
    heading: float = 0.0
    msg_type: str = ""
    val_flags: bool = False
    seq_id: int = 0

aircraft_cache = {}
global_sequence = 0

MISSION_DURATION_SECONDS = 600
mission_start_time = time.time()
reported_10min = False

HOST = '127.0.0.1'
PORT = 30003

print("Iniciando missão ADS-B (10 minutos)...")

while True:
    s = None
    stream = None

    try:
        print(f"Conectando ao dump1090 em {HOST}:{PORT}...")
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))

        stream = s.makefile('r', encoding='utf-8', errors='ignore')
        print("Conectado com sucesso. Adquirindo telemetria...")

        while True:
            elapsed_time = time.time() - mission_start_time
            if elapsed_time >= MISSION_DURATION_SECONDS and not reported_10min:
                print("\n" + "="*50)
                print(f"AVISO: A janela padrão de 10 minutos de missão foi atingida!")
                print(f"Total de aeronaves únicas rastreadas: {len(aircraft_cache)}")
                print("O sistema continuará rodando em modo de monitoramento contínuo...")
                print("="*50 + "\n")
                reported_10min = True
                break

            line = stream.readline()
            if not line:
                raise ConnectionError("Stream fechado pelo servidor.")

            line = line.strip()

            if line:
                parts = line.split(',')

                if len(parts) > 15 and parts[0] == 'MSG':
                    msg_type = parts[1]
                    icao = parts[4]

                    if icao not in aircraft_cache:
                        aircraft_cache[icao] = AircraftTelemetry(icao=icao)

                    plane = aircraft_cache[icao]
                    updated = False
                    current_timestamp = datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')

                    plane.msg_type = msg_type

                    if msg_type == '3':
                        if parts[11]:
                            plane.alt = float(parts[11])
                            updated = True
                        if parts[14] and parts[15]:
                            plane.lat = float(parts[14])
                            plane.lon = float(parts[15])
                            updated = True

                    elif msg_type == '4':
                        if parts[12]:
                            plane.vel = float(parts[12])
                            updated = True
                        if len(parts) > 13 and parts[13]:
                            plane.heading = float(parts[13])
                            updated = True

                    if updated:
                        global_sequence += 1
                        plane.seq_id = global_sequence
                        plane.timestamp = current_timestamp

                        if plane.alt > 0 and plane.lat is not None and plane.lon is not None:
                            plane.val_flags = True

                        if plane.val_flags:
                            json_record = json.dumps(asdict(plane), separators=(',',':'))

                            with open(LOG_FILE, 'a') as f:
                                f.write(json_record + '\n')

                            print(f"[{int(elapsed_time)}s] [{current_timestamp}] Log -> Aeronave {icao} | Alt: {plane.alt}ft | Vel: {plane.vel}kt | Total Rastreadas: {len(aircraft_cache)}")

    except Exception as e:
        print(f"Erro de conexão ou socket: {e}. Tentando reconectar em 3 segundos...")
        if stream:
            try: stream.close()
            except: pass
        if s:
            try: s.close()
            except: pass
        time.sleep(3)