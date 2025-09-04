import os
import pandas as pd
from VIDEO import Video_processing
from MODELS import ModelProcessor
from INDEX import Index
from FrameExtracton import GoProH265FastExtractor
from PROCESSOR import AmmaloramentiProcessor

cartella_video = "video" # percorso della cartella che contiene il video da elaborare
path = r"/home/ipool/Desktop/main_scheda" 
frames = r"/home/ipool/Desktop/main_scheda/"+cartella_video+"/frames_estratti"

os.makedirs(frames, exist_ok=True)

def main_function(cartella):        #Nome della cartella con tutti i video dentro
    cartella_input = os.path.join(path, cartella)

    model_class =ModelProcessor(cartella_input, 'yolo')
    index_class = Index(frames)
    csv_folder = os.path.join(cartella_input, 'csv')
    os.makedirs(csv_folder, exist_ok=True)
    audio_gps_path = r"/home/ipool/Desktop/output_audio/GPS_classe_acustica.csv"
    video_class = Video_processing(path)
    

    video_class.set_dir(cartella) 

    speed = []
    durata = [0]

    file_mp4 = [f for f in os.listdir(cartella_input) if f.lower().endswith(".mp4")]

    for file in file_mp4:
        # Nome senza estensione
        nome_senza_ext = os.path.splitext(file)[0]
        # Path sottocartella
        video_class.estrai_gpx(nome_senza_ext)
        video_class.extract_csv(nome_senza_ext)
        patt = os.path.join(cartella_input,'csv', nome_senza_ext + '-gps.csv')
        pat2 = os.path.join(cartella_input, file)
        vel, fps = video_class.ricalcolo_velocità(patt , pat2)
        speed.extend(vel)
        durata.append(len(vel))

    video_class.merge_csv()

    extraction_points = video_class.calculate_frame_extraction_points(speed)

    extractor = GoProH265FastExtractor(
        video_folder=cartella_input,
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
    processor = AmmaloramentiProcessor(cartella_input,audio_gps_path) 
    processor.run()

    #create map
    processor.build_map()

if __name__ == "__main__":
    import multiprocessing
    multiprocessing.freeze_support()  
    main_function(cartella_video)