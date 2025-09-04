import socket
import threading
import time
import numpy as np
from scipy.interpolate import CubicSpline
from datetime import timedelta
import queue 
import os

class esp32:
    def __init__(self):
        # === CONNESSIONE AL DISPOSITIVO ESP32 ===
        self.HOST = "192.168.4.1"
        self.PORT = 1234
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.HOST, self.PORT))
        self.Q = queue.Queue()
        self.interp_attivo = True
        self.counter_name = 0
        self.path = ''
        self.secondi_da_analizzare = 3
        self.sock.settimeout(5)

    def scrivi_al_socket(self, stringa):
        self.sock.send(stringa.encode())

    def set_pathname(self, name):
        self.path = name
    
    def start_queuing(self):
        thread = threading.Thread(target=self.interpolation_and_writing, daemon=True)
        thread.start()
        return thread

    def stop_queue(self):
        self.interp_attivo = False

    def interpolation_and_writing(self):
        buffer_blocks = []
        while self.interp_attivo or not self.Q.empty():
            try:
                elemento = self.Q.get(timeout=1)
                if not elemento:
                    continue
                arr = np.array(elemento)
                dati = self.interpolate_to_8192(arr)
                interpolated_data = dati.astype(int)
                buffer_blocks.append(interpolated_data)

                if len(buffer_blocks) == self.secondi_da_analizzare:
                    combined = np.concatenate(buffer_blocks)
                    output_file = os.path.join(self.path, str(self.counter_name) + ".txt")
                    with open(output_file, 'w') as f:
                        f.writelines(f"{val}\n" for val in combined)
                    self.counter_name += 1
                    buffer_blocks = []
                    print(f"✅ Dati interpolati salvati in '{output_file}'.")
            except queue.Empty:
                continue

    def interpolate_to_8192(self, data_block):
        recorded_count = len(data_block)
        if recorded_count < 2:
            return np.full(8192, data_block[0] if data_block else 0)
        x_old = np.linspace(0, 1, recorded_count)
        x_new = np.linspace(0, 1, 8192)
        cs = CubicSpline(x_old, data_block, extrapolate=True)
        return cs(x_new)
    
    
    # === SCARICAMENTO FILE DALL’ESP32 ===
    def download_and_process_after_recording(self):
        try:
            print("📤 Invio comando 'r' per scaricare i dati...")
            self.sock.sendall(b"r")
            time.sleep(0.5)

            buffer = bytearray()
            temp = []

            while True:
                try:
                    chunk = self.sock.recv(8192)
                    if not chunk:
                        break
                    buffer.extend(chunk)

                    while b'\n' in buffer:
                        newline_index = buffer.index(b'\n')
                        line = buffer[:newline_index].decode(errors='ignore').strip()
                        buffer = buffer[newline_index + 1:]

                        if not line:
                            if temp:
                                self.Q.put(temp)
                                temp = []
                        elif line == ".":
                            if temp:
                                self.Q.put(temp)
                            output_file = os.path.join(self.path, "final_data.txt")
                            with open(output_file, 'w') as f:
                                f.writelines(f"{val}\n" for val in temp)
                            print(f"✅ Scrittura completata: {output_file}")
                            return
                        else:
                            if line[0] in '-0123456789':
                                try:
                                    val = int(line)
                                    temp.append(val)
                                except:
                                    pass
                except socket.timeout:
                    print("🕔 Timeout raggiunto.")
                    break

            print(f"📥 Ricevuti tutti i dati dalla ESP32.")

            print("🧹 Inviando comando 'x' per chiudere la connessione...")
            self.sock.sendall(b"x")

        except Exception as e:
            print("❌ Errore durante la ricezione del file:", e)
            return