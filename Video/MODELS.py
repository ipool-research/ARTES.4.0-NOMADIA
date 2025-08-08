from ultralytics import YOLO
from ultralytics import RTDETR
import os
import time
from glob import glob
import pandas as pd
import numpy as np
import torch
from ultralytics.engine.results import Boxes
from pathlib import Path
import cv2
from utils_filters import *
from ensemble_boxes import weighted_boxes_fusion
from multiprocessing import Pool, cpu_count
from tqdm import tqdm


class ModelProcessor:
    def __init__(self, process_folder, model_name='yolo'):
        self.folder_path = os.path.join(process_folder, 'frames_estratti')
        self.model_name = model_name
        if model_name == 'yolo':
            self.model_path = r"F:\ARTES\VERSIONE_DB\Training\dataset_db\train_yolo_no-empty\weights\best.pt"  # load the model
        else:
            self.model_path = r"/home/ipool/Desktop/Artes/MODELLI/yolo.pt" #load the model (cambiare i path)

    def run_all_folders_filtered(self, batch_size=8, img_size=512, confidence_thresholds=None, save_images=False):
        folders = sorted([
            os.path.join(self.folder_path, d)
            for d in os.listdir(self.folder_path)
            if os.path.isdir(os.path.join(self.folder_path, d))
        ])

        args = [
            (folder, self.model_name, self.model_path, img_size, batch_size, confidence_thresholds, save_images)
            for folder in folders
        ]

        # Riduci il numero di processi per Jetson
        n_proc = min(2, len(args))  # massimo 2 per sicurezza
        with Pool(processes=n_proc) as pool:
            list(tqdm(pool.imap_unordered(process_folder_wrapper, args), total=len(args)))


def process_folder_wrapper(args):
    folder, model_name, model_path, img_size, batch_size, confidence_thresholds, save_images = args

    # Carica il modello dentro il processo (non da main)
    if model_name == 'yolo':
        model = YOLO(model_path)
        process_folder_filtered(folder, model, img_size=img_size, batch_size=batch_size,
                                confidence_thresholds=confidence_thresholds, save_images=save_images)
    elif model_name == 'rtdetr':
        model = RTDETR(model_path)
        process_folder_rtdetr_batched(folder, model, mode='wbf', img_size=img_size, batch_size=batch_size,
                                      confidence_thresholds=confidence_thresholds, save_images=save_images)



def process_folder_filtered(folder_path, model, img_size=512, batch_size=8, confidence_thresholds=None, save_images=False):
    image_paths = sorted(glob(os.path.join(folder_path, '*.jpg')) +
                         glob(os.path.join(folder_path, '*.png')))

    if not image_paths:
        print(f"[SKIP] Nessuna immagine trovata in: {folder_path}")
        return

    for i in range(0, len(image_paths), batch_size):
        batch_paths = image_paths[i:i + batch_size]
        results = model.predict(
            batch_paths,
            imgsz=img_size,
            conf=0.1,
            iou=0.5,
            device='cuda' if torch.cuda.is_available() else 'cpu',
            save=False,
            verbose=False
        )

        for result, path_img in zip(results, batch_paths):
            image = cv2.imread(path_img)
            h, w = image.shape[:2]
            image_name = os.path.basename(path_img)
            folder = os.path.dirname(path_img)

            # Box in formato xyxy, normalizzati
            boxes_xyxy = result.boxes.xyxy.cpu().numpy()
            scores = result.boxes.conf.cpu().numpy()
            classes = result.boxes.cls.cpu().numpy()

            # Converti in formato compatibile con la tua funzione
            results_format = {
                'boxes': boxes_xyxy,
                'scores': scores,
                'classes': classes
            }

            # Filtra i risultati
            filtered_boxes, filtered_scores, filtered_classes = results_for_filtered(
                results_format,
                image_size=(w, h),
                classes_treshs=confidence_thresholds
            )

            if filtered_boxes:
                # De-normalizza le box da [0,1] -> salva YOLO
                boxes_yolo = []
                for box in filtered_boxes:
                    boxes_yolo.append([
                        box[0], box[1], box[2], box[3]  # già normalizzati
                    ])

                save_results_wbf(
                    image=image,
                    name=image_name,
                    path_save=folder,
                    boxes_list=boxes_yolo,
                    scores_list=filtered_scores,
                    labels_list=filtered_classes,
                    image_size=[w, h],
                    save_images=save_images
                )


def process_folder_rtdetr_batched(folder_path, model, mode='filtered', img_size=512, batch_size=8,
                                  confidence_thresholds=None, iou_thr_wbf=0.2, save_images=False):
    image_paths = sorted(glob(os.path.join(folder_path, '*.jpg')) +
                         glob(os.path.join(folder_path, '*.png')) +
                         glob(os.path.join(folder_path, '*.jpeg')))

    if not image_paths:
        print(f"[SKIP] Nessuna immagine trovata in: {folder_path}")
        return

    print(f"[START] {folder_path} - {len(image_paths)} immagini")

    for i in range(0, len(image_paths), batch_size):
        batch_paths = image_paths[i:i + batch_size]

        results = model.predict(
            source=batch_paths,
            imgsz=img_size,
            conf=0.3,
            iou=0.5,
            device='cuda' if torch.cuda.is_available() else 'cpu',
            save=False,
            verbose=False
        )

        for result, image_path in zip(results, batch_paths):
            image = cv2.imread(image_path)
            h, w = image.shape[:2]
            image_name = os.path.basename(image_path)
            folder = os.path.dirname(image_path)

            if mode == 'wbf':
                # Estrai box, score e classi nel formato richiesto da WBF
                boxes, scores, classes = results_for_wbf(result)

                if boxes:
                    # Applica WBF (risultati già normalizzati [0-1])
                    final_boxes, final_scores, final_classes = weighted_boxes_fusion(
                        boxes, scores, classes,
                        iou_thr=iou_thr_wbf,
                        weights=[1] * len(boxes),
                        skip_box_thr=0.0001,
                        conf_type='box_and_model_avg',
                        allows_overflow=False
                    )

                    save_results_wbf(
                        image=image,
                        name=image_name,
                        path_save=folder,
                        boxes_list=final_boxes,
                        scores_list=final_scores,
                        labels_list=final_classes,
                        image_size=[w, h],
                        save_images=save_images
                    )

            elif mode == 'filtered':
                # Estrai box, score e classi come dizionario (come nel tuo approccio YOLO)
                boxes_xyxy = result.boxes.xyxy.cpu().numpy()
                scores = result.boxes.conf.cpu().numpy()
                classes = result.boxes.cls.cpu().numpy()

                results_format = {
                    'boxes': boxes_xyxy,
                    'scores': scores,
                    'classes': classes
                }

                filtered_boxes, filtered_scores, filtered_classes = results_for_filtered(
                    results_format,
                    image_size=(w, h),
                    classes_treshs=confidence_thresholds
                )

                if filtered_boxes:
                    save_results_wbf(
                        image=image,
                        name=image_name,
                        path_save=folder,
                        boxes_list=filtered_boxes,
                        scores_list=filtered_scores,
                        labels_list=filtered_classes,
                        image_size=[w, h],
                        save_images=save_images
                    )


def save_results_wbf(image, name, path_save, boxes_list, scores_list, labels_list,
                     colors=[(b, g, r) for (r, g, b) in [[0, 130, 200], [60, 180, 75], [230, 25, 75], [245, 130, 48], [145, 30, 180]]],
                     class_names=['Longitudinal', 'Transverse', 'Alligator', 'Pothole', 'Manhole'],
                     image_size=[512, 512],
                     save_images=False):
    
    labels_folder = os.path.join(path_save, 'labels')
    os.makedirs(labels_folder, exist_ok=True)

    if save_images:
        images_folder = os.path.join(path_save, 'images')
        os.makedirs(images_folder, exist_ok=True)
        image_copy = image.copy()

    label_lines = []

    for box, cls in zip(boxes_list, labels_list):
        x1 = int(box[0] * image_size[0])
        y1 = int(box[1] * image_size[1])
        x2 = int(box[2] * image_size[0])
        y2 = int(box[3] * image_size[1])

        # Coordinate YOLO normalizzate
        box_width = box[2] - box[0]
        box_height = box[3] - box[1]
        x_center = box[0] + box_width / 2
        y_center = box[1] + box_height / 2

        label_line = f"{int(cls)} {x_center:.6f} {y_center:.6f} {box_width:.6f} {box_height:.6f}"
        label_lines.append(label_line)

        if save_images:
            color = colors[int(cls)]
            label = f'{class_names[int(cls)]}'
            cv2.rectangle(image_copy, (x1, y1), (x2, y2), color, 1)
            (tw, th), baseline = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
            cv2.rectangle(image_copy, (x1, y1 - th - 5), (x1 + tw + 5, y1), color, -1)
            cv2.putText(image_copy, label, (x1 + 2, y1 - 5), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)

    label_filename = os.path.splitext(name)[0] + '.txt'
    with open(os.path.join(labels_folder, label_filename), 'w') as f:
        f.write('\n'.join(label_lines))

    if save_images:
        cv2.imwrite(os.path.join(images_folder, name), image_copy)


