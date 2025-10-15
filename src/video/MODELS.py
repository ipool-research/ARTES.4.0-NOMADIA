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
            self.model_path = r"/home/ipool/Desktop/Artes_CUDA/MODELLI/yolo.pt"  # load the model
        else:
            self.model_path = r"/home/ipool/Desktop/Artes_CUDA/MODELLI/rtdetr.pt" #load the model (cambiare i path)

    def run_all_folders_filtered(self, batch_size=8, img_size=512, confidence_thresholds=None, save_images=True):
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



def process_folder_filtered(folder_path, model, img_size=512, batch_size=8, confidence_thresholds=None, save_images=True):
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
            conf=0.3,
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
                                  confidence_thresholds=None, iou_thr_wbf=0.2, save_images=True):
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
                     colors=[(31, 119, 180), (44, 160, 44), (255, 127, 14), (148, 103, 189), (140, 86, 75)],
                     class_names=['Longitudinal', 'Transverse', 'Alligator', 'Pothole', 'Manhole'],
                     image_size=[512, 512],
                     save_images=True):
    """
    Save YOLO label files and (optionally) images with drawn boxes.
    Handles boxes as xyxy or xcycwh, normalized or pixel.
    Uses the real image size for scaling.
    """
    import os
    import cv2

    # Folders
    labels_folder = os.path.join(path_save, 'labels')
    os.makedirs(labels_folder, exist_ok=True)

    if save_images:
        images_folder = os.path.join(path_save, 'images')
        os.makedirs(images_folder, exist_ok=True)
        image_copy = image.copy()

    H, W = image.shape[:2]

    def to_xyxy_pixels(box):
        bx = list(map(float, box))
        # normalized if all in [0, 1.1]
        normalized = all(0.0 <= v <= 1.1 for v in bx)
        if normalized:
            bx = [bx[0]*W, bx[1]*H, bx[2]*W, bx[3]*H]

        x0, y0, x2v, y2v = bx[0], bx[1], bx[2], bx[3]

        # Decide format: if x2<x0 or y2<y0 or looks like sizes, assume center format
        if (x2v < x0) or (y2v < y0) or (x2v <= W*0.5 and y2v <= H*0.5 and x0 <= W and y0 <= H):
            # xcycwh
            xc, yc, bw, bh = bx
            x1 = int(round(xc - bw/2)); y1 = int(round(yc - bh/2))
            x2 = int(round(xc + bw/2)); y2 = int(round(yc + bh/2))
        else:
            # xyxy
            x1 = int(round(x0)); y1 = int(round(y0))
            x2 = int(round(x2v)); y2 = int(round(y2v))

        if x1 > x2: x1, x2 = x2, x1
        if y1 > y2: y1, y2 = y2, y1

        x1 = max(0, min(W-1, x1)); y1 = max(0, min(H-1, y1))
        x2 = max(0, min(W-1, x2)); y2 = max(0, min(H-1, y2))

        if x2 == x1: x2 = min(W-1, x1+1)
        if y2 == y1: y2 = min(H-1, y1+1)
        return x1, y1, x2, y2

    label_lines = []

    for i, box in enumerate(boxes_list):
        cls = int(labels_list[i]) if i < len(labels_list) else 0
        x1, y1, x2, y2 = to_xyxy_pixels(box)

        # YOLO normalized xc,yc,w,h
        xc = ((x1 + x2) / 2) / W
        yc = ((y1 + y2) / 2) / H
        bw = (x2 - x1) / W
        bh = (y2 - y1) / H
        label_lines.append(f"{cls} {xc:.6f} {yc:.6f} {bw:.6f} {bh:.6f}")

        if save_images:
            color = colors[cls % len(colors)] if colors else (0, 255, 0)
            name_txt = class_names[cls] if cls < len(class_names) else f"class_{cls}"

            # Draw rect
            cv2.rectangle(image_copy, (x1, y1), (x2, y2), color, 2)
            # Label bg + text
            (tw, th), baseline = cv2.getTextSize(name_txt, cv2.FONT_HERSHEY_DUPLEX, 0.6, 1)
            y_text = max(th + 2, y1 - 5)
            tl = (x1, y_text - th - 4)
            br = (x1 + tw + 6, y_text + 2)
            cv2.rectangle(image_copy, tl, br, color, -1)
            cv2.putText(image_copy, name_txt, (x1 + 3, y_text), cv2.FONT_HERSHEY_DUPLEX, 0.6, (255,255,255), 1, cv2.LINE_AA)

    # Write labels
    label_filename = os.path.splitext(name)[0] + '.txt'
    with open(os.path.join(labels_folder, label_filename), 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(label_lines) + ('\n' if label_lines else ''))

    # Save image
    if save_images:
        img_name = name if name.lower().endswith(('.jpg', '.jpeg', '.png')) else (os.path.splitext(name)[0] + '.jpg')
        cv2.imwrite(os.path.join(images_folder, img_name), image_copy)


