import os
import pandas as pd
import tkinter as tk
from tkinter import filedialog
from VIDEO import Video_processing
from MODELS import ModelProcessor
from INDEX import Index
from FRAMEEXTRACTION import GoProH265FastExtractor
from PROCESSOR import AmmaloramentiProcessor
root = tk.Tk()
root.withdraw()

path = tk.filedialog.askdirectory(title = "Seleziona cartella di lavoro") 
path_cartella_video = tk.filedialog.askdirectory(title = "Seleziona cartella video")
cartella_video = os.path.basename(os.path.normpath(path_cartella_video)) # percorso della cartella che contiene il video da elaborare
frames = os.path.join(path_cartella_video, "frames_estratti")

os.makedirs(frames, exist_ok=True)

def main_function(cartella):        #Nome della cartella con tutti i video dentro

    model_class =ModelProcessor(cartella, 'yolo')
    index_class = Index(frames)
    csv_folder = os.path.join(cartella, 'csv')
    os.makedirs(csv_folder, exist_ok=True)

    audio_gps_path = tk.filedialog.askopenfilename(title= "Selezionare file di output dell'analisi audio")
    
    video_class = Video_processing()
    
    video_class.set_dir(cartella) 

    speed = []
    durata = [0]

    file_mp4 = [f for f in os.listdir(cartella) if f.lower().endswith(".mp4")]

    for file in file_mp4:
        # Nome senza estensione
        nome_senza_ext = os.path.splitext(file)[0]
        # Path sottocartella
        video_class.estrai_gpx(nome_senza_ext)
        video_class.extract_csv(nome_senza_ext)
        patt = os.path.join(cartella,'csv', nome_senza_ext + '-gps.csv')
        pat2 = os.path.join(cartella, file)
        vel, fps = video_class.ricalcolo_velocità(patt , pat2)
        speed.extend(vel)
        durata.append(len(vel))

    video_class.merge_csv()

    extraction_points = video_class.calculate_frame_extraction_points(speed)

    extractor = GoProH265FastExtractor(
        video_folder=cartella,
        video_limits=durata,  # dev’essere una lista come [0, n1, n2, ...]
        extraction_points=extraction_points,
        output_folder=frames,
        x_0=560,
        height=0,
        crop_size=1080
    )
    extractor.run()

    df = pd.read_csv(audio_gps_path)

    # Legge la prima riga della colonna desiderata (es. colonna 'Nome')
    timestamp_start = df['Start'].iloc[0] 
    video_class.organizza_frames(timestamp_start)

    model_class.run_all_folders_filtered()

    index_class.process_all_folders_parallel()
    processor = AmmaloramentiProcessor(cartella,audio_gps_path) 
    processor.run()

    #create map
    processor.build_map()

if __name__ == "__main__":
    import multiprocessing
    multiprocessing.freeze_support()  
    main_function(path_cartella_video)
