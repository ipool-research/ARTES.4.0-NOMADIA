#!/bin/bash
# Script per avviare l'applicativo Artes Audio

# Spostati nella cartella di progetto
cd ~/Desktop/Artes_audio || { echo "Cartella non trovata!"; exit 1; }

# Avvia lo script Python
python Classificazione_tcn.py


cd ../Artes_video || { echo "Cartella non trovata!"; exit 1; }
source artes_video/bin/activate
python main.py
