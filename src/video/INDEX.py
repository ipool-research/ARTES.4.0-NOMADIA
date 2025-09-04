import os
from glob import glob
from multiprocessing import Pool, cpu_count
from tqdm import tqdm
import pandas as pd
import cv2
import numpy as np
class Index():
    def __init__(self, path):
        self.path = path # percorso alla cartella con le cartelle dei vari intervalli
        self.intervalli_dir = os.path.join(os.path.dirname(path), 'intervalli')
        os.makedirs(self.intervalli_dir, exist_ok=True)
        self.mask = 'configuration/maschera_finale.png'
        self.columns = [
            'frame',
            'Longitudinal', 'Transverse', 'Alligator', 'Pothole', 'Manhole',
            'Longitudinal_sx', 'Transverse_sx', 'Alligator_sx', 'Pothole_sx', 'Manhole_sx',
            'Longitudinal_dx', 'Transverse_dx', 'Alligator_dx', 'Pothole_dx', 'Manhole_dx',
            'Longitudinal_both', 'Transverse_both', 'Alligator_both', 'Pothole_both', 'Manhole_both'
        ]


    def process_all_folders_parallel(self, num_workers=None):
        folders = sorted([
            os.path.join(self.path, d)
            for d in os.listdir(self.path)
            if os.path.isdir(os.path.join(self.path, d))
        ])

        if num_workers is None:
            num_workers = min(cpu_count(), len(folders))

        with Pool(processes=num_workers) as pool:
            results = list(tqdm(pool.imap_unordered(self.process_labels_folder, folders), total=len(folders)))

        return results  
    
    def calcola_percentuali(self, labels_path):
        colori = [255, 127, 85]  # Maschera 1, 2, 3

        # === Carica immagine maschera in scala di grigi ===
        mask_gray = cv2.imread(self.mask, cv2.IMREAD_GRAYSCALE)
        if mask_gray is None:
            raise FileNotFoundError(f"Maschera non trovata: {self.mask}")
        h, w = mask_gray.shape

        # === Carica file di label YOLO ===
        with open(labels_path, 'r') as f:
            lines = [l.strip() for l in f if l.strip()]

        data = []

        for line in lines:
            # YOLO: class x_center y_center width height (valori normalizzati)
            class_id, xc, yc, bw, bh = map(float, line.split())

            # Coordinate in pixel
            x1 = int(xc - bw / 2)
            y1 = int(yc - bh / 2)
            x2 = int(xc + bw / 2)
            y2 = int(yc + bh / 2)

            # Clipping all'immagine
            x1, y1 = max(0, x1), max(0, y1)
            x2, y2 = min(w, x2), min(h, y2)

            crop = mask_gray[y1:y2, x1:x2]
            area_box = (x2 - x1) * (y2 - y1)

            # Calcola overlap percentuale per ciascuna maschera
            percentuali = []
            for c in colori:
                mask_bin = (crop == c).astype(np.uint8)
                overlap = int(np.sum(mask_bin))
                overlap_pct = (overlap / area_box * 100) if area_box > 0 else 0.0
                percentuali.append(overlap_pct)

            data.append([int(class_id)] + percentuali)

        # === Crea DataFrame ===
        df = pd.DataFrame(data, columns=[
            "classe",
            "percentuale_maschera1",
            "percentuale_maschera2",
            "percentuale_maschera3"
        ])
        return df
    
    def convert_df_to_row(self, df, labels):
        frame_name = os.path.splitext(os.path.basename(labels.rstrip('/')))[0]
        # Escludi quelli con maschera3 > 50%
        df = df[df['percentuale_maschera3'] <= 50]

        result = {'frame': frame_name}
        classi = ['Longitudinal', 'Transverse', 'Alligator', 'Pothole', 'Manhole']
        
        # Inizializza tutti i campi a 0
        for c in classi:
            result[c] = 0
            result[f'{c}_sx'] = 0
            result[f'{c}_dx'] = 0
            result[f'{c}_both'] = 0

        for _, row in df.iterrows():
            cl = int(row['classe'])
            c_name = classi[cl]
            m1 = row['percentuale_maschera1']
            m2 = row['percentuale_maschera2']

            # Conta come 'tutti <1%'
            if m1 < 1 and m2 < 1:
                result[c_name] += 1
                continue

            # Salta i 'both' se non soddisfano i criteri
            if m1 > 0 and m2 > 0:
                if cl in [0, 3]:  # Longitudinal o Pothole
                    if m1 >= 40 and m2 >= 40:
                        result[f'{c_name}_both'] += 1
                elif cl in [1, 2]:  # Transverse o Alligator
                    result[f'{c_name}_both'] += 1
                elif cl == 4:  # Manhole
                    result[f'{c_name}_both'] += 1
                continue

            # Solo sx
            if m1 > 0 and m2 == 0:
                if cl in [0, 3] and m1 >= 40:
                    result[f'{c_name}_sx'] += 1
                elif cl in [1, 2]:
                    result[f'{c_name}_sx'] += 1
                elif cl == 4:
                    result[f'{c_name}_sx'] += 1

            # Solo dx
            elif m2 > 0 and m1 == 0:
                if cl in [0, 3] and m2 >= 40:
                    result[f'{c_name}_dx'] += 1
                elif cl in [1, 2]:
                    result[f'{c_name}_dx'] += 1
                elif cl == 4:
                    result[f'{c_name}_dx'] += 1

        return pd.DataFrame([result])



    def process_labels_folder(self, path):
        labels_path = os.path.join(self.path, path, 'labels')
        folder_name = os.path.basename(path)
        print(self.intervalli_dir)
        file_csv = os.path.join(self.intervalli_dir, folder_name + '.csv')
        if not os.path.exists(labels_path):
            #print(f"[SKIP] {path} non contiene 'labels'")
            return

        label_files = sorted(glob(os.path.join(labels_path, '*.txt')))

        tabella = pd.DataFrame(columns = self.columns)
        for label in label_files:
            df = self.calcola_percentuali(label)
            new_row = pd.DataFrame(self.convert_df_to_row(df, label), columns=self.columns)
            tabella = pd.concat([tabella, new_row], ignore_index=True)

        print(f"Salvo CSV in: {file_csv}")
        tabella.to_csv(file_csv)


