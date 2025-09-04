import os
import cv2
import threading
import queue
import time
import shutil
from concurrent.futures import ThreadPoolExecutor
import datetime
from multiprocessing import Process, Queue as MPQueue, cpu_count
from tqdm import tqdm

class GoProH265FastExtractor:
    def __init__(self, video_folder, video_limits, extraction_points, output_folder, x_0=360, height=0, crop_size=1080):
        self.video_folder = video_folder
        self.video_limits = video_limits
        self.extraction_points = set(extraction_points)
        self.output_folder = output_folder
        self.temp_output_folder = "/dev/shm/gopro_tmp"
        self.x_0 = x_0
        self.height = height
        self.crop_size = crop_size
        self.resize_size = (512, 512)
        self.BATCH_SIZE = 32
        self.total_frames = len(self.extraction_points)
        self.progress_lock = threading.Lock()
        self.frames_saved = 0

        os.makedirs(self.temp_output_folder, exist_ok=True)
        os.makedirs(self.output_folder, exist_ok=True)
        self.videos = sorted([f for f in os.listdir(self.video_folder) if f.lower().endswith('.mp4')])

    def run(self):
        print("\U0001F680 Avvio estrazione frame parallela (Jetson Orin Nano)...")
        print("\U0001F552 Orario di partenza:", datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
        t_start = time.perf_counter()

        self.pbar = tqdm(total=self.total_frames, desc="Estrazione frame", unit="frame")

        num_processes =  cpu_count()
        processes = []
        mp_queues = []

        # Divide i video in gruppi per ogni processo
        video_splits = [[] for _ in range(num_processes)]
        for i, video in enumerate(self.videos):
            video_splits[i % num_processes].append(video)

        for i in range(num_processes):
            q = MPQueue()
            mp_queues.append(q)
            p = Process(target=self.extract_from_list, args=(video_splits[i], q))
            p.start()
            processes.append(p)

        for p in processes:
            p.join()

        # Copia dei file da RAM-disk a SSD
        print("\U0001F4C5 Copia dei file da RAM a SSD...")
        for file in os.listdir(self.temp_output_folder):
            src = os.path.join(self.temp_output_folder, file)
            dst = os.path.join(self.output_folder, file)
            shutil.move(src, dst)

        self.pbar.close()
        t_end = time.perf_counter()
        print(f"\u23F1️ Tempo totale: {t_end - t_start:.2f} s")
        print(f"\U0001F4BE Frame salvati in: {self.output_folder}")

    def extract_from_list(self, video_list, result_queue):
        executor = ThreadPoolExecutor(max_workers=4)
        batch = []

        for video_name in video_list:
            video_path = os.path.join(self.video_folder, video_name)
            gst_pipeline = build_gstreamer_pipeline(video_path)
            cap = cv2.VideoCapture(gst_pipeline, cv2.CAP_GSTREAMER)

            if not cap.isOpened():
                print(f"❌ Impossibile aprire {video_name}")
                continue

            start_idx = self.video_limits[0]  # Inizio assoluto
            frame_idx = 0
            while True:
                ret, frame = cap.read()
                if not ret:
                    break

                frame_idx_assoluto = start_idx + frame_idx
                if frame_idx_assoluto in self.extraction_points:
                    processed = self.process_frame(frame)
                    if processed is not None:
                        batch.append((processed, video_name, frame_idx_assoluto))

                        with self.progress_lock:
                            self.frames_saved += 1
                            self.pbar.update(1)

                        if len(batch) >= self.BATCH_SIZE:
                            executor.submit(self.save_batch, list(batch))
                            batch.clear()

                frame_idx += 1

            cap.release()

        if batch:
            executor.submit(self.save_batch, batch)

        executor.shutdown()

    def process_frame(self, frame):
        try:
            if self.height == 0:
                y_start = (frame.shape[0] - self.crop_size) // 2
            else:
                y_start = self.height

            x_start = self.x_0 if self.x_0 > 0 else (frame.shape[1] - self.crop_size) // 2
            cropped_img = frame[y_start:y_start + self.crop_size, x_start:x_start + self.crop_size]
            resized_img = cv2.resize(cropped_img, self.resize_size, interpolation=cv2.INTER_AREA)
            return resized_img
        except Exception as e:
            print(f"⚠️ Errore durante il processing del frame: {e}")
            return None

    def save_batch(self, items):
        for img, video_name, frame_idx in items:
            name = video_name.split('.MP4')[0]
            output_path = os.path.join(self.temp_output_folder, f'{name}_frame_{frame_idx}.jpg')
            cv2.imwrite(output_path, img, [cv2.IMWRITE_JPEG_QUALITY, 85])

def build_gstreamer_pipeline(filepath):
    return (
        f"filesrc location={filepath} ! "
        "qtdemux ! "
        "h265parse ! "
        "nvv4l2decoder ! "
        "nvvidconv ! "
        "video/x-raw, format=BGRx ! "
        "videoconvert ! "
        "video/x-raw, format=BGR ! "
        "appsink drop=true max-buffers=1"
    )
