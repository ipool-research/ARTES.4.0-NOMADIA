#!/bin/bash
# Script per avviare l'applicativo Artes Audio

# Spostati nella cartella di progetto
cd ~/Desktop/Artes_audio || { echo "Cartella non trovata!"; exit 1; }

# Attiva l'ambiente virtuale
source artes_env/bin/activate || { echo "Impossibile attivare l'ambiente virtuale!"; exit 1; }

# Avvia lo script Python principale
python main.py

# Disattiva l'ambiente virtuale
deactivate
