#!/bin/bash
# Script per avviare l'applicativo Artes Audio

# Spostati nella cartella dell'ambiente virtuale
cd ../Artes_Audio || { echo "Cartella non trovata!"; exit 1; }

# Attiva l'ambiente virtuale
source artes_audio/bin/activate || { echo "Impossibile attivare l'ambiente virtuale!"; exit 1; }

# Spostati nella cartella di lavoro
cd ../audio

# Avvia lo script Python principale
python main.py

# Disattiva l'ambiente virtuale
deactivate
