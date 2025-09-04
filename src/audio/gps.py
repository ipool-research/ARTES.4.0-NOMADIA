import csv
import time
from datetime import datetime, timezone, timedelta
import threading
import queue
import os
import serial
import pandas as pd
import pytz

from geopy.distance import geodesic
from scipy.interpolate import interp1d
import numpy as np

# Configura la porta seriale
SERIAL_PORT = "/dev/ttyUSB0"
BAUD_RATE = 4800

class GPS:
    def __init__(self, path, serial_port=SERIAL_PORT, baud_rate=BAUD_RATE):
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        time_name = datetime.now(timezone.utc).astimezone(pytz.timezone("Europe/Rome"))
        date_str = time_name.strftime("%d-%m-%Y")
        time_str = time_name.strftime("%H_%M_%S")
        gps_folder=f"GPS_{date_str}_{time_str}"
        self.csv_folder = os.path.join(path, 'gps', 'csv', gps_folder)
        self.intervals_folder = os.path.join(path, 'gps', 'intervalli', gps_folder)
        nmea_path = os.path.join(path, 'gps', 'nmea', gps_folder)
        os.makedirs(self.csv_folder, exist_ok=True)
        os.makedirs(self.intervals_folder, exist_ok=True)
        os.makedirs(nmea_path, exist_ok=True)
        self.data_queue = queue.Queue()
        self.stop_event = threading.Event()
        self.collector_thread = None
        self.saver_thread = None
        self.ser = None
        self.nmea_filename = os.path.join(nmea_path, datetime.utcnow().strftime("gps_data_%Y%m%d_%H%M%S.nmea"))
        self.interval_counter = 1

    def get_folder(self):
        return self.intervals_folder
    # controlla se il GPS ha segnale, se si avvia il thread per la raccolta dati
    def start(self):
        try:
            self.ser = serial.Serial(self.serial_port, baudrate=self.baud_rate, timeout=1)
            self.ser.flushInput()
            self.ser.flushOutput()
            dati_presenti = False
            #aspetto che ci siano dei dati validi da parte del GPS prima di avviare il tutto
            while not dati_presenti:
                line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                print(line)
                if line.startswith("$GPRMC"):
                    fields = line.split(",")
                    if len(fields) > 2 and fields[2] == "A":
                            dati_presenti=True
            print("Dati ricevuti correttamente!")
            self.collector_thread = threading.Thread(
                target=gps_collector,
                args=(self.ser, self.data_queue, self.nmea_filename, self.stop_event),
                daemon=True
            )
            self.collector_thread.start()
            print("GPS avviato.")
        except Exception as e:
            print(f"Errore durante l'avvio: {e}")

    def start_periodic_saving(self, interval_seconds=20):
        def periodic_saver():
            while not self.stop_event.is_set():
                self._save_current_data()
                time.sleep(interval_seconds)
            # Salvataggio finale
            self._save_current_data()

        self.saver_thread = threading.Thread(target=periodic_saver, daemon=True)
        self.saver_thread.start()
        print(f"Salvataggio automatico ogni {interval_seconds} secondi avviato.")

    def _save_current_data(self):
        data_batch = []
        while not self.data_queue.empty():
            try:
                data = self.data_queue.get_nowait()
                data_batch.append([data["time"], data["latitude"], data["longitude"]])
            except queue.Empty:
                break

        if data_batch:
            timestamp = datetime.utcnow().strftime("%Y%m%d_%H%M%S")
            filename = os.path.join(self.csv_folder, f"gps_data_{timestamp}.csv")
            write_csv(filename, data_batch)
            print(f"[SALVATO] {filename}")
        else:
            print("[INFO] Nessun nuovo dato da salvare.")

    def stop(self):
        self.stop_event.set()
        if self.collector_thread:
            self.collector_thread.join()
        if self.saver_thread:
            self.saver_thread.join()
        if self.ser and self.ser.is_open:
            self.ser.close()
        print(f"Dati NMEA salvati in: {self.nmea_filename}")
        print("GPS fermato.")

    def start_processing_thread(self, fs_gps=1, fixed_distance=20):
        def processing_loop():
            print("[GPS] Thread di processing avviato.")
            buffer = []
            residual_data = []  # dati residui da concatenare ai nuovi

            while not self.stop_event.is_set():
                try:
                    # Carica i nuovi CSV nella cartella
                    files = [os.path.join(self.csv_folder, f) for f in os.listdir(self.csv_folder) if f.endswith('.csv')]
                    files.sort(key=os.path.getctime)

                    if not files:
                        time.sleep(2)
                        continue

                    for file in files:
                        data = load_gps_from_csv2(file)
                        buffer.extend(data.values.tolist())
                        os.remove(file)

                    # Aggiungi eventuali dati residui al buffer
                    buffer = residual_data + buffer

                    # Esegui l'interpolazione solo se ci sono almeno 2 punti
                    while len(buffer) >= 2:
                        df_buffer = pd.DataFrame(buffer, columns=['Time', 'Latitude', 'Longitude'])

                        result = interpolate_data_length(df_buffer, fs_gps, [], [], [], fixed_distance)
                        lat_new, lon_new, speed_new, time_new, last_lon, last_lat, last_time, check = result

                        if check and len(lat_new) > 1:
                            segment_length = len(lat_new) - 1

                            # Salva intervallo da 20m (non includere il punto finale, che diventa lo start del prossimo)
                            self._write_interval_csv([
                                time_new[:segment_length],
                                lat_new[:segment_length],
                                lon_new[:segment_length],
                                speed_new[:segment_length]
                            ], self.intervals_folder)

                            # Prepara i dati residui: punto finale diventa l'inizio del prossimo intervallo
                            residual_data = [[time_new[-1], lat_new[-1], lon_new[-1]]]

                            # Mantieni nel buffer solo i punti successivi a quello finale
                            last_time_value = pd.to_datetime(time_new[-1])
                            buffer = [[t, lat, lon] for t, lat, lon in buffer if pd.to_datetime(t) > last_time_value]
                        else:
                            break  # esci se non si riesce a fare un nuovo intervallo

                    time.sleep(2)

                except Exception as e:
                    print(f"[GPS] Errore nel processing: {e}")
                    time.sleep(5)

            print("[GPS] Thread di processing terminato.")

        thread = threading.Thread(target=processing_loop, daemon=True)
        thread.start()
        self.processing_thread = thread

    def _write_interval_csv(self, data, path):
        while True:
            file_name = os.path.join(path, f"intervallo_{self.interval_counter}.csv")
            if not os.path.exists(file_name):
                break
            self.interval_counter += 1

        rows = [['Start', 'End', 'Latitude_inizio', 'Latitude_fine', 'Longitude_inizio', 'Longitude_fine', 'Velocità']]
        for i in range(len(data[0]) - 1):
            if data[3][i] <= 0:
                velocita = data[3][i+1]
            else:
                velocita = (data[3][i] + data[3][i+1]) / 2
            rows.append([
                data[0][i], data[0][i+1],
                data[1][i], data[1][i+1],
                data[2][i], data[2][i+1],
                velocita
            ])

        with open(file_name, mode='w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerows(rows)

        print(f"[GPS] CSV intervallo creato: {file_name}")


def load_gps_from_csv2(file):
    """
    load the gps data from a csv file
    """
    data = pd.read_csv(os.path.join(file))
    if not isinstance(data, pd.DataFrame):
        raise ValueError(f"Il file {file} non è stato letto correttamente come DataFrame.")
    data['Longitude'] =  data['Longitude']  
    data['Latitude'] =  data['Latitude']
    data['Time'] = pd.to_datetime(data['Time'])
    #print("dati dal csv pre:", data)

    nan_rows = data[data['Latitude'].isna() | data['Longitude'].isna()]
    data = data.dropna(subset=['Latitude','Longitude'])
    #print("dati dal csv post:", data)   

    return data


def interpolate_data_length(data, fs_gps, last_lon = [], last_lat = [], last_time = [], fixed_distance= 20):
    """
    interpolate the data to a fixed length
    """
    if len(last_time) > 0:
        last_time_in_ns = [t.value for t in last_time]  # `t.value` restituisce il timestamp in nanosecondi

        lon_data = np.concatenate([last_lon, data['Longitude'].values])
        lat_data = np.concatenate([last_lat, data['Latitude'].values])

        # Calcolo dei delta temporali in nanosecondi
        time_deltas_ns = (data['Time'].astype('int64') - data['Time'].iloc[0].value)
        time_data = np.concatenate([last_time_in_ns, last_time_in_ns[-1] + time_deltas_ns])
    else:
        lon_data = data['Longitude'].values
        lat_data = data['Latitude'].values
        time_data = data['Time'].astype('int64')  # Già in nanosecondi


    last_lon = []
    last_lat = []
    last_time = []

    cumulative_distance = [0]
    speeds = [0]

    for k in range(1, len(lon_data)):
        point1 = (lat_data[k-1], lon_data[k-1])
        point2 = (lat_data[k], lon_data[k])
        distance = geodesic(point1, point2).meters
        cumulative_distance.append(cumulative_distance[-1] + distance)
        speeds.append(distance*fs_gps)

    if len(cumulative_distance) != len(time_data):
        raise ValueError("Lunghezza cumulative_distance e time_data non corrispondono.")
    
    if cumulative_distance[-1]<=0.9:
        check_distance = False
        lat_new, lon_new, speed_new, time_new = [], [], [], []
    else:
        check_distance = True
        interp_lat = interp1d(cumulative_distance, lat_data, kind='linear')
        interp_lon = interp1d(cumulative_distance, lon_data, kind='linear')
        interp_speed = interp1d(cumulative_distance, speeds, kind='linear')
        interp_time = interp1d(cumulative_distance, time_data, kind='linear')
        print("interp_time", interp_time)

        new_distances = np.arange(0, cumulative_distance[-1], fixed_distance)
        if new_distances[-1] < cumulative_distance[-1]:
            new_distances = np.append(new_distances, cumulative_distance[-1])

        # Interpolate latitude and longitude values at the new distances
        lat_new = interp_lat(new_distances)
        lon_new = interp_lon(new_distances)
        speed_new = interp_speed(new_distances)
        time_new = [pd.Timestamp(t, unit='ns') for t in interp_time(new_distances)]
        print("time_new", time_new)

        if(cumulative_distance[-1]% fixed_distance >= 2):
            last_valid_index = np.where(new_distances % fixed_distance == 0)[0][-1]
            if last_valid_index > 0:
                last_lat = lat_data[last_valid_index-1 :]
                last_lon = lon_data[last_valid_index-1 :]
                last_time = [pd.Timestamp(t, unit='ns') for t in time_data[last_valid_index-1:]]
            else:
                last_lat = []
                last_lon = []
                last_time = []

    return [lat_new, lon_new, speed_new, time_new, last_lon, last_lat, last_time, check_distance]

def nmea_to_decimal(coord_str, direction):
    degrees = int(coord_str[:len(coord_str) - 7])
    minutes = float(coord_str[len(coord_str) - 7:])
    decimal = degrees + (minutes / 60.0)

    if direction in ['S', 'W']:
        decimal = -decimal
    return decimal

# Funzione per analizzare una stringa NMEA e restituire un dizionario con le informazioni necessarie
def parse_nmea(sentence):
    try:
        if sentence.startswith("$GPGGA"):
            fields = sentence.split(',')
            if len(fields) > 5 and fields[2] and fields[4]:
                latitude = nmea_to_decimal(fields[2], fields[3])
                longitude = nmea_to_decimal(fields[4], fields[5])
                return {"latitude": latitude, "longitude": longitude}

        elif sentence.startswith("$GPRMC"):
            fields = sentence.split(',')
            if len(fields) > 9 and fields[1] and fields[9]:
                utc_time = fields[1].split('.')[0]  # Rimuove i millisecondi
                date = fields[9]
                try:
                    dt = datetime.strptime(date + utc_time, "%d%m%y%H%M%S")
                    dt_plus_2h = dt + timedelta(hours=2)
                    time_utc = dt_plus_2h.strftime("%Y-%m-%d %H:%M:%S")
                    return {"time": time_utc}

                except Exception as e:
                    print(f"Errore nel parsing della data GPRMC: {e}")
    except Exception as e:
        print(f"Errore durante l'analisi: {e}")
    return None

# Funzione per scrivere i dati in un file CSV
def write_csv(filename, data):
    with open(filename, mode="w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow(["Time", "Latitude", "Longitude"])
        writer.writerows(data)

# Thread per la raccolta continua dei dati GPS
def gps_collector(serial_connection, data_queue, nmea_filename, stop_event):
    temp_data = {}
    nmea_buffer = []
    buffer_size = 10  # Numero di righe NMEA prima del salvataggio

    while not stop_event.is_set():
        try:
            line = serial_connection.readline().decode('utf-8', errors='ignore').strip()
            nmea_buffer.append(line.encode("utf-8"))

            if len(nmea_buffer) >= buffer_size:
                with open(nmea_filename, "ab") as nmea_file:
                    nmea_file.write(b"\n".join(nmea_buffer) + b"\n")
                nmea_buffer.clear()

            parsed_data = parse_nmea(line)
            if parsed_data:
                temp_data.update(parsed_data)
                if "time" in temp_data and "latitude" in temp_data and "longitude" in temp_data:
                    data_queue.put(temp_data.copy())
                    temp_data.clear()
        except Exception as e:
            print(f"Errore nella raccolta dati: {e}")

    if nmea_buffer:
        with open(nmea_filename, "ab") as nmea_file:
            nmea_file.write(b"\n".join(nmea_buffer) + b"\n")

    

