import pandas as pd
import numpy as np
import os
import shutil
import sys
import subprocess
import re
import csv
import cv2
import math
from scipy.interpolate import interp1d
from datetime import datetime
from utils.speed_functions import get_fps_video,resample_speed
from utils.displacement_functions import calculate_frame_extraction_points, extract_specific_frames3_512_decentr
from gopro2gpx.gopro2gpx import main
from utils.gpx_to_csv_with_velocity import gpx_to_csv


class Video_processing():
    def __init__(self):
        self.visual_coverage = 5
        self.dir = []
        self.name = ''
        self.output_folder = ''
        self.tempo_inizio = datetime.now()
        self.fps = 120
        self.x_0 = 360
        self.height = 0
        self.crop_size = 1080
        self.counternan =0
        self.output_file_csv = ''
    
    def set_dir(self, folder_path):
        self.dir.append(folder_path)
        self.name = os.path.basename(folder_path)

    def estrai_gpx(self, name):
        output_path_senza_estensione = os.path.join(self.dir[0], name)
        input_file = os.path.join(self.dir[0], f"{name}.MP4")
        comando = [
            "python", "-m", "gopro2gpx.gopro2gpx",
            input_file,
            output_path_senza_estensione,
            "--gpx"
        ]
        try:
            subprocess.run(comando, check=True)
            print("GPX estratto con successo.")
        except subprocess.CalledProcessError as e:
            print(f"Errore durante l'esecuzione del comando: {e}")

    def extract_csv(self, name):
        input_file = os.path.join(self.dir[0], name + '.gpx')
        output_file = os.path.join(self.dir[0], 'csv', name + '-gps.csv')
        # Verifica che il file esista 
        if not os.path.exists(input_file):
            print(f"Errore: il file '{input_file}' non esiste.")
            return
        # Chiama la funzione di conversione
        self.tempo_inizio = gpx_to_csv(input_file, output_file )

    def merge_csv(self):
        cartella = os.path.join(self.dir[0], 'csv')
        self.output_file_csv = os.path.join(self.dir[0], self.name +'-gps.csv')
        file_csv = [f for f in os.listdir(cartella) if f.lower().endswith('.csv')]
        file_csv.sort()  # Ordine alfabetico, opzionale

        if not file_csv:
            raise FileNotFoundError("Nessun file CSV trovato nella cartella.")

        # Leggi il primo file con header
        primo_path = os.path.join(cartella, file_csv[0])
        df = pd.read_csv(primo_path)

        # Leggi gli altri file senza header
        for nome_file in file_csv[1:]:
            path = os.path.join(cartella, nome_file)
            df_temp = pd.read_csv(path, header=0)  # Supponiamo tutti abbiano header, che verrà ignorato
            df = pd.concat([df, df_temp], ignore_index=True)

        # Salva il risultato
        df.to_csv(self.output_file_csv, index=False)

    def ricalcolo_velocità(self, gps_file, video_path):
        # Read the CSV file
        df = pd.read_csv(gps_file)
        # Convert TS to datetime (assuming TS is in microseconds)
        df['datetime'] = pd.to_datetime(df['TS'], unit='us')
        # Convert Speed column to a numpy array
        speed = df['Speed'].to_numpy()
        # Calculate the time differences between consecutive TS values (in seconds)
        df['time_diff'] = df['TS'].diff() / 1e6  # Convert from microseconds to seconds
        # Calculate the sampling frequency
        fs = 1 / df['time_diff'].mean()  # Average sampling frequency
        #print(f"Estimated sampling frequency: {fs} Hz")
        self.fps = get_fps_ffprobe(video_path)
        # Create a time axis based on the original timestamps (in seconds)
        time_original = (df['TS'] - df['TS'].min()) / 1e6  # Convert TS to seconds relative to the first timestamp
        # Define new time axis in order to match the frequency sample from the camera
        time_new = np.arange(0, time_original.max(), 1/self.fps)
        # Interpolate Speed to match the new 60 Hz time axis
        speed_resampled = resample_speed(time_original,speed,time_new)
        return speed_resampled, self.fps


    def start_time(self):
        csv_file = os.path.join(self.dir[0], self.output_file_csv) 
        print(csv_file)
        df = pd.read_csv(csv_file, usecols=['TS'])
        for raw_value in df['TS']:
            ts_str = str(raw_value).strip()
            for fmt in ("%Y-%m-%d %H:%M:%S.%f", "%Y-%m-%d %H:%M:%S"):
                try:
                    self.tempo_inizio = datetime.strptime(ts_str, fmt)
                    print("✅ Trovato timestamp valido (formato datetime):", self.tempo_inizio)
                    return
                except ValueError:
                    continue

            # 2. Se è tutto numerico, prova come timestamp in microsecondi
            if ts_str.isdigit() and len(ts_str) >= 13:
                try:
                    ts_micro = int(ts_str)
                    self.tempo_inizio = datetime.fromtimestamp(ts_micro / 1_000_000)
                    print("✅ Trovato timestamp valido (microsecondi):", self.tempo_inizio)
                    return
                except Exception as e:
                    print("⚠️ Errore parsing microsecondi:", e)
                    continue

        raise ValueError("❌ Nessun valore timestamp valido trovato nella colonna 'TS'")


    def organizza_frames(self, timestamp_ini):
        # Cartella di output
        self.output_folder = os.path.join(self.dir[0], 'frames_estratti')
        os.makedirs(self.output_folder, exist_ok=True)

        # Imposta self.tempo_inizio leggendo dal CSV
        self.start_time()

        # Calcola quanti secondi tra tempo_inizio e timestamp_ini
        timestamp_start = parse_datetime(timestamp_ini)

        delta_secondi = abs((timestamp_start - self.tempo_inizio).total_seconds())

        frame_start_index = int(delta_secondi * self.fps)
        print(frame_start_index)

        # Lista di file immagine con nome tipo GXxxxx_frame_xxx.png
        frame_files = sorted([
            f for f in os.listdir(self.output_folder)
            if f.lower().endswith('.png') or f.lower().endswith('.jpg') and re.search(r'frame_(\d+)', f)
        ], key=get_frame_number)

        frame_indices = [get_frame_number(f) for f in frame_files]

        # Trova il primo frame disponibile >= frame_start_index
        frame_start_available = next((i for i in frame_indices if i >= frame_start_index), None)
        if frame_start_available is None:
            raise ValueError("❌ Nessun frame disponibile dopo il timestamp indicato.")

        # Separa file da tenere e da spostare in backup
        frame_files_to_keep = [f for f in frame_files if get_frame_number(f) >= frame_start_available]
        frame_files_to_backup = [f for f in frame_files if get_frame_number(f) < frame_start_available]

        # Crea cartella backup e sposta i vecchi frame
        backup_dir = os.path.join(self.output_folder, 'backup_frames')
        os.makedirs(backup_dir, exist_ok=True)
        for f in frame_files_to_backup:
            shutil.move(os.path.join(self.output_folder, f), os.path.join(backup_dir, f))

        # Ordina i frame da tenere
        frame_files_to_keep = sorted(frame_files_to_keep, key=get_frame_number)

        # Raggruppa in cartelle da 4
        for i in range(0, len(frame_files_to_keep), 4):
            gruppo = frame_files_to_keep[i:i+4]
            intervallo_dir = os.path.join(self.output_folder, f'INTERVALLO_{i//4 + 1}')
            os.makedirs(intervallo_dir, exist_ok=True)
            for f in gruppo:
                shutil.move(os.path.join(self.output_folder, f), os.path.join(intervallo_dir, f))

        print(f"✅ Organizzati {len(frame_files_to_keep)} frame in {len(frame_files_to_keep) // 4 + 1} cartelle.")

    def calculate_frame_extraction_points(self, speed_data):
        # Initialize cumulative displacement and time variables
        cumulative_displacement = 0
        extraction_points = [0]  # To store frame indices where a frame should be extracted
        frame_time = 0
        frame_interval = 1 / self.fps  # Time between frames in seconds
        flag = 0
        # Loop through speed data to calculate displacement
        for i, speed in enumerate(speed_data):
            if speed < 1.5:
                continue
            else:
                if flag == 0:
                    extraction_points.append(i)
                    flag = 1 
                displacement = speed * frame_interval  # Displacement for the current frame
                if not math.isnan(displacement): 
                # Accumulate the displacement
                    cumulative_displacement += displacement
                else:
                    self.counternan +=1
                # If displacement reaches the desired threshold (visual_coverage - overlap)
                if cumulative_displacement >= (self.visual_coverage):
                    extraction_points.append(i)  # Record the frame index
                    cumulative_displacement = 0  # Reset the displacement for the next segment

                # Increment time
                frame_time += frame_interval
        print("counternan : ", self.counternan)
        return extraction_points

def get_frame_number(filename):
    """Estrae il numero del frame da un nome tipo GXxxxxx_frame_12345.png"""
    match = re.search(r'frame_(\d+)', filename)
    return int(match.group(1)) if match else -1

def parse_datetime(s):
    """Prova a convertire una stringa in datetime"""
    try:
        return datetime.strptime(s, "%Y-%m-%d %H:%M:%S.%f")
    except ValueError:
        try:
            return datetime.strptime(s, "%Y-%m-%d %H:%M:%S")
        except ValueError:
            if s.isdigit() and len(s) >= 13:
                return datetime.fromtimestamp(int(s) / 1_000_000)
            raise
    
def get_fps_ffprobe(video_path):
    try:
        result = subprocess.run(
            [
                "ffprobe", "-v", "error",
                "-select_streams", "v:0",
                "-show_entries", "stream=r_frame_rate",
                "-of", "default=noprint_wrappers=1:nokey=1",
                video_path
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True
        )
        if result.returncode != 0:
            raise RuntimeError(result.stderr)
        rate = result.stdout.strip()
        if '/' in rate:
            num, den = map(int, rate.split('/'))
            return num / den
        else:
            return float(rate)
    except Exception as e:
        print(f"Errore nel recupero FPS con ffprobe: {e}")
        return None

def resample_speed(t1,s1,t2):
    speed_interp = interp1d(t1, s1, kind='linear', fill_value="extrapolate")
    s2 = speed_interp(t2)    
    return s2


def parse_datetime(time_str):
    try:
        return datetime.strptime(time_str, "%Y-%m-%d %H:%M:%S.%f")
    except ValueError:
        return datetime.strptime(time_str, "%Y-%m-%d %H:%M:%S")


