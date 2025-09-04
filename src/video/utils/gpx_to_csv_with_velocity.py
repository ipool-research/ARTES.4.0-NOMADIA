import gpxpy
import pandas as pd
import os
import numpy as np
from geopy.distance import geodesic  # Per calcolare la distanza tra coordinate GPS

def calculate_speed(lat1, lon1, alt1, lat2, lon2, alt2, time_diff):
    """
    Calcola la velocità 2D e 3D tra due punti GPS consecutivi.
    """
    if pd.isna(lat1) or pd.isna(lon1) or pd.isna(lat2) or pd.isna(lon2) or time_diff <= 0:
        return None, None  # Se manca un valore o il tempo è 0, velocità non calcolabile

    # Distanza 2D (latitudine e longitudine)
    distance_2d = geodesic((lat1, lon1), (lat2, lon2)).meters

    # Distanza verticale (altitudine)
    distance_vertical = alt2 - alt1 if not pd.isna(alt1) and not pd.isna(alt2) else 0

    # Distanza 3D (Pitagora)
    distance_3d = np.sqrt(distance_2d**2 + distance_vertical**2)

    # Velocità in m/s
    speed_2d = distance_2d / time_diff
    speed_3d = distance_3d / time_diff

    return round(speed_2d, 3), round(speed_3d, 3)

def gpx_to_csv(input_file, output_file=None):
    # Leggi il file GPX
    with open(input_file, 'r') as gpx_file:
        gpx = gpxpy.parse(gpx_file)

    # Estrai i punti GPS con timestamp in microsecondi
    data = []
    for track in gpx.tracks:
        for segment in track.segments:
            for point in segment.points:
                data.append({
                    'Milliseconds': None,  # Verrà calcolato dopo
                    'Latitude': point.latitude,
                    'Longitude': point.longitude,
                    'Altitude': point.elevation,
                    'TS': int(point.time.timestamp() * 1e6) if point.time else None,  # Timestamp in microsecondi
                    'GpsAccuracy': 108,  # Valore fisso
                    'GpsFix': 3  # Valore fisso per "fix valid"
                })

    # Converti in DataFrame Pandas
    df = pd.DataFrame(data)

    # Calcola la differenza di tempo tra i punti successivi
    df['time_diff'] = df['TS'].diff() / 1e6  # Converti in secondi

    # Calcola i Milliseconds (tempo relativo al primo punto)
    df['Milliseconds'] = (df['TS'] - df['TS'].min()) / 1000  # Converti in millisecondi
    df['Milliseconds'] = df['Milliseconds'].fillna(0).astype(int)  # Il primo punto sarà 0

    # Aggiungi colonne con i valori della riga precedente
    df['Latitude_prev'] = df['Latitude'].shift(1)
    df['Longitude_prev'] = df['Longitude'].shift(1)
    df['Altitude_prev'] = df['Altitude'].shift(1)

    # Calcola la velocità tra punti GPS consecutivi
    df[['Speed', 'Speed3D']] = df.apply(lambda row: calculate_speed(
        row['Latitude_prev'], row['Longitude_prev'], row['Altitude_prev'],
        row['Latitude'], row['Longitude'], row['Altitude'],
        row['time_diff']
    ), axis=1, result_type='expand')

    # Se il nome di output non è specificato, usa il nome del file input
    if output_file is None:
        output_file = os.path.splitext(input_file)[0] + "-gps.csv"

    # Salva in CSV con le colonne ordinate correttamente
    
    df[['Milliseconds', 'Latitude', 'Longitude', 'Altitude', 'Speed', 'Speed3D', 'TS', 'GpsAccuracy', 'GpsFix']].to_csv(output_file, index=False)
    print(f"File CSV salvato: {output_file}")
    return df['TS'][0]
    
