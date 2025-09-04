"""
Script per classificazione del segnale audio

Input:
- scelta del tipo di fit da utilizzare a seconda della tipologia di strada:
        lasciare decommentata una sola riga fra 37, 38, 39
- segnale tcn (cartella tcn)
- intervalli temporali (cartella gps)

Ouput:
- file csv con classe acustica definita dal tcn

Procedimento:
- Nota la velocità di misura si estragono i livelli nella banda di frequenza da 315 a 1000 Hz
- Sull'intervallo del tcn si calcola il livello di pressione interno corrispondente a segmenti con lunghezza di 2048 elementi (2048/8192 = tempo di percorrenza)
- Si estrae il valore massimo dell'intervallo
- Si fa la sottrazione tra massimo ed il livello predetto dal modello di fit (a*log(v) + b)
- Classificazione come riportato di seguito:
        -> Se la differenza è compresa tra [-inf, 2] -> 0
        -> Se la differenza è compresa tra [2,4] -> 1
        -> Se la differenza è compresa tra [4,inf] -> 2

Note: 
- La soglia a 2 è stata determinata da analisi su pavimentazioni a basso impatto acustico (eta di vita pari a 3 anni minimo) 
- La soglia a 4 da analisi visiva di soli elementi del dataset Urbane ML (sopra tale soglia la pavimentazioni presentavo tipicamente molti ammaloramenti) 

VERSIONE 1: modello calibrato sui dati misurati in zona Urbana di Ospedaletto 
"""

import os
import pickle as pkl
import numpy as np 
import matplotlib.pyplot as plt
from scipy.signal import sosfilt
import pandas as pd
cwd = os.getcwd()
#fit = 'Fit_Autostrada.pkl'
#fit = Fit_Extraurbana.pkl'
fit = 'Fit_Urbana.pkl'
#Carica i parametri del fit a*log(v) + b
with open(f'FIT\{fit}','rb') as f:
        Fit_modello_speed_Lf = pkl.load(f)

#Parametri del modello di fit popt_fit[0] = a, popt_fit[1] = b, velocità in km/h
popt_fit = Fit_modello_speed_Lf['popt']

#Carica il filtro alle basse frequenza [315,1000] Hz per segnali campionati ad una frequenza di 8192 Hz
with open(r'FIT/LF_tcn.pkl','rb') as f:
        filtro_lf = pkl.load(f)

sos_lf = filtro_lf['LF[315,1000]']

#Dati intervalli del percorso gps, ciascuna riga corrisponde ai punti di inizio e fine per un segmento con lunghezza 
#pari a 20 mt, velocità è data in m/s 
fold = os.path.dirname(cwd)
path_gps_csv_files = os.path.join(fold, 'data', 'gps')

#Dati TCN corrispondenti in formato txt, ogni file ha una lunghezza di 3 s
path_dati_TCN = os.path.join(fold, 'data', 'tcn')

#Carica i file gps
files_gps_csv = [f for f in os.listdir(path_gps_csv_files) if f.endswith('csv')]

#Ordina i file gps
def ind_gps(file):
        """
        estrae l'indice del file gps per ordinare la sequenza
        """
        return int(file.split('.csv')[0].split('_')[-1])

files_gps_csv.sort(key=ind_gps)

files_tcn_txt = [f for f in os.listdir(path_dati_TCN) if f.endswith('.txt') and 'final_data' not in f]

#Ordina i file TCN
def ind_tcn(file):
        """
        estrae l'indice del file txt del TCN per ordinarli
        """
        return int(file.split('.txt')[0])

files_tcn_txt.sort(key=ind_tcn)

#Estrae e concatena informazioni dal dato gps
for n in range(len(files_gps_csv)):

        gps = pd.read_csv(os.path.join(path_gps_csv_files,files_gps_csv[n]))
        # Concatenate GPS data if there are multiple files
        if n == 0:
                gps_total = gps.copy()
        else:
                gps_total = pd.concat([gps_total, gps], ignore_index=True)

#tempi di inizio
Start = gps_total['Start']
End = gps_total['End']
Velocita = gps_total['Velocità']

# aggiungi una nuova colonna 'classe_acustica' inizializzata a NaN
gps_total['LF[315,1000]_mean'] = np.nan
gps_total['LF[315,1000]_max'] = np.nan
gps_total['classe_acustica'] = pd.Series([pd.NA] * len(gps_total), dtype="Int64")


def hh_mm_ss_to_ss(time):
        """
        Converte una data del tipo yyy-mm-dd hh:mm:ss in secondi
        non considera il cambio data al momento attuale
        """
        time_split = time.split(' ')[-1]
        time_split = time_split.split(':')
        hh = float(time_split[0])
        mm = float(time_split[1])
        ss = float(time_split[-1])

        time_seconds = hh*60**2 + mm*60 + ss
        
        return time_seconds

#Applica la conversione in secondi a tutti i tempi del gps ed esclude il primo intervallo
Start_seconds = Start.apply(hh_mm_ss_to_ss)
End_seconds = End.apply(hh_mm_ss_to_ss)

#salva il primo tempo per gli elementi successivi
Tempo_iniziale = Start_seconds[0]

#Toglie il tempo iniziale agli elemtni convertiti in secondi
Start_seconds = Start_seconds - Tempo_iniziale
End_seconds = End_seconds - Tempo_iniziale

#Carica il file tcn e lo converte in un array tra -1 e 1
bitres = 24 #risoluzione in bit del tcn
sr_tcn = 8192 #frequenza di campionamento del tcn
threshold = 0.001373166848044763 #solo per (NOMADIA) 
nfft = 2048

tcn_total = []

for q in range(len(files_tcn_txt)):
        fp = os.path.join(path_dati_TCN,files_tcn_txt[q])    

        with open(fp, 'r') as f:
                lines = f.readlines()

        lines = [int(line) for line in lines]
        values = np.array(lines,dtype = 'float')
        tcn_tmp = values/(2**(bitres-1)-1) #conversione del valore letto tra -1 e 1
        tcn_total = np.concatenate((tcn_total,tcn_tmp))

# Funzione per assegnare la classe acustica al segmento sotto analisi
def assegna_classe_acustica(popt_fit,speed,spl_max):
        """
        Assegna la classe acustica al segmento sotto analisi
        speed deve essere in km/h
        """
        if speed < 21:
                classe_acustica =-1
        else:
                dspl_max = spl_max - popt_fit[0]*np.log10(speed) - popt_fit[1]
                classe_acustica = 0
                
                if dspl_max < 2:
                        classe_acustica = 0
                elif dspl_max > 4:
                        classe_acustica = 2
                else:
                        classe_acustica = 1
        
        return classe_acustica
n = 0
#Spacchetta il file in elementi intervallati da 20 mt
for n in range(len(Start_seconds)):

        if End_seconds[n]*sr_tcn < len(tcn_total):
                
                tcn_evaluation = tcn_total[int(Start_seconds[n]*sr_tcn):int(End_seconds[n]*sr_tcn)]
                
                #valutazione acustica dell'emissione
                if len(tcn_evaluation) < nfft:
                        
                        tcn_filt = sosfilt(sos_lf,tcn_evaluation)
                        spl = 20*np.log10(np.sqrt(np.mean(tcn_filt**2)))
                        classe_acustica = assegna_classe_acustica(popt_fit,3.6*Velocita[n],spl)
                        
                        gps_total.loc[n,'classe_acustica'] = int(classe_acustica)
                        gps_total.loc[n,'LF[315,1000]_max'] = spl
                        gps_total.loc[n,'LF[315,1000]_mean'] = spl

                else:
                        spl = []
                        for i in range(0,len(tcn_evaluation)-nfft+1,nfft//2):
                                tcn_tmp = tcn_evaluation[i:i+nfft]
                                tcn_filt = sosfilt(sos_lf,tcn_tmp)
                                spl.append(20*np.log10(np.sqrt(np.mean(tcn_filt**2))))
                        
                        spl_mean = np.mean(spl)
                        spl_max = np.max(spl)
                        classe_acustica = assegna_classe_acustica(popt_fit,3.6*Velocita[n],spl_max)

                        gps_total.loc[n,'classe_acustica'] = int(classe_acustica)
                        gps_total.loc[n,'LF[315,1000]_max'] = spl_max
                        gps_total.loc[n,'LF[315,1000]_mean'] = spl_mean
gps_total.to_csv('GPS_classe_acustica.csv')        
                




























