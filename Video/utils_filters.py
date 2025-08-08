import numpy as np

def filter_boxes_by_class_thresholds(boxes, scores, classes, thresholds):
  
    filtered_boxes = []
    filtered_scores = []
    filtered_classes = []

    for box, score, cls in zip(boxes, scores, classes):
        cls = int(cls)
              
        if score >= thresholds.get(str(cls), 0):  # Default threshold of 0 if class not in thresholds
            filtered_boxes.append(box)
            filtered_scores.append(score)
            filtered_classes.append(cls)
    
    return filtered_boxes, filtered_scores, filtered_classes

def normalize_boxes(boxes, image_width, image_height):
    normalized_boxes = []
    for box in boxes:
        x_min = box[0] / image_width
        y_min = box[1] / image_height
        x_max = box[2] / image_width
        y_max = box[3] / image_height
        normalized_boxes.append([x_min, y_min, x_max, y_max])
    return normalized_boxes

def results_process(results):
    print(results)
    print(type(results))
    # Combina i risultati in una lista
    all_boxes = []
    all_scores = []
    all_classes = []

    boxes = results[0].boxes.xyxy.cpu().numpy()  # bounding boxes
    scores = results[0].boxes.conf.cpu().numpy()  # confidence scores
    classes = results[0].boxes.cls.cpu().numpy()  # class labels

    all_boxes.append(boxes)
    all_scores.append(scores)
    all_classes.append(classes)

    if not all_scores:
        results_format = {"boxes":all_boxes, "scores": all_scores, "classes": all_classes}
        return results_format
    else:
        # Concatenazione di tutte le predizioni
        all_boxes = np.vstack(all_boxes)
        all_scores = np.concatenate(all_scores)
        all_classes = np.concatenate(all_classes)

        results_format = {"boxes":all_boxes, "scores": all_scores, "classes": all_classes}

    return results_format

def results_for_wbf (results, image_size=(512, 512), classes_treshs={"0": 0, "1": 0, "2": 0, "3": 0, "4": 0}):
    results_format = results_process(results)
    all_boxes = normalize_boxes(results_format['boxes'], image_size[0], image_size[1])
    all_scores = results_format['scores']
    all_classes = results_format['classes']
    filtered_boxes, filtered_scores, filtered_classes = filter_boxes_by_class_thresholds(all_boxes, all_scores, all_classes, classes_treshs)                
    if filtered_boxes:
        all_boxes     =  [filtered_boxes  ,[]]
        all_scores    =  [filtered_scores ,[]]
        all_classes   =  [filtered_classes,[]]

    return all_boxes, all_scores, all_classes

def results_for_filtered(results, image_size, classes_treshs):
    boxes = results['boxes']
    scores = results['scores']
    classes = results['classes']

    # qui il tuo codice per filtrare i box, ad esempio
    filtered_boxes = []
    filtered_scores = []
    filtered_classes = []

    for box, score, cls in zip(boxes, scores, classes):
        if classes_treshs is None or score >= classes_treshs.get(int(cls), 0.1):
            filtered_boxes.append(box)
            filtered_scores.append(score)
            filtered_classes.append(cls)

    return filtered_boxes, filtered_scores, filtered_classes
