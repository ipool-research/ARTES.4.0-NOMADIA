"""
Script per classificazione del segnale audio

Input:
- parametri del fit per il segnale tra -1 e 1 da 20 km/h a 55 km/h
- segnale tcn 
- intervalli temporali 

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
import time
import threading


class Classificazione_Audio:

	def __init__(self, gps_path, tcn_path):

                self.tcn_path = tcn_path
                #Carica i parametri del fit a*log(v) + b
                with open('Fit_model_speed_LF_315_1000_Hz.pkl','rb') as f:
                        Fit_modello_speed_Lf = pkl.load(f)

                #Parametri del modello di fit popt_fit[0] = a, popt_fit[1] = b, velocità in km/h
                self.popt_fit = Fit_modello_speed_Lf['popt']

                #Carica il filtro alle basse frequenza [315,1000] Hz per segnali campionati ad una frequenza di 8192 Hz
                with open('LF_tcn.pkl','rb') as f:
                        filtro_lf = pkl.load(f)

                self.sos_lf = filtro_lf['LF[315,1000]']

                #Dati intervalli del percorso gps, ciascuna riga corrisponde ai punti di inizio e fine per un segmento con lunghezza 
                #pari a 20 mt, velocità è data in m/s 
                #Carica i file gps
                files_gps_csv = [f for f in os.listdir(gps_path) if f.endswith('csv')]

                files_gps_csv.sort(key=ind_gps)
        
                        #Estrae e concatena informazioni dal dato gps
                for n in range(len(files_gps_csv)):

                        gps = pd.read_csv(os.path.join(gps_path,files_gps_csv[n]))
                        # Concatenate GPS data if there are multiple files
                        if n == 0:
                                self.gps_total = gps.copy()
                        else:
                                self.gps_total = pd.concat([self.gps_total, gps], ignore_index=True)

                #tempi di inizio
                self.Start = self.gps_total['Start']
                self.End = self.gps_total['End']
                self.Velocita = self.gps_total['Velocità']

                # aggiungi una nuova colonna 'classe_acustica' inizializzata a NaN
                self.gps_total['LF[315,1000]_mean'] = np.nan
                self.gps_total['LF[315,1000]_max'] = np.nan
                self.gps_total['classe_acustica'] = pd.Series([pd.NA] * len(self.gps_total), dtype="Int64")

                #Applica la conversione in secondi a tutti i tempi del gps ed esclude il primo intervallo
                self.Start_seconds = self.Start.apply(hh_mm_ss_to_ss)
                self.End_seconds = self.End.apply(hh_mm_ss_to_ss)

                #salva il primo tempo per gli elementi successivi
                self.Tempo_iniziale = self.Start_seconds[0]

                #Toglie il tempo iniziale agli elemtni convertiti in secondi
                self.Start_seconds = self.Start_seconds - self.Tempo_iniziale
                self.End_seconds = self.End_seconds - self.Tempo_iniziale

                #Carica il file tcn e lo converte in un array tra -1 e 1
                self.bitres = 24 #risoluzione in bit del tcn
                self.sr_tcn = 8192 #frequenza di campionamento del tcn
                self.threshold = 0.001373166848044763 #solo per (NOMADIA) 
                self.nfft = 2048

                #Dati TCN corrispondenti in formato txt, ogni file ha una lunghezza di 3 s
                        # Dati TCN
                self.tcn_total = np.array([], dtype='float')
                self.tcn_file_set = set()
                self.tcn_lock = threading.Lock()

                # Avvia il monitor TCN in background
                threading.Thread(target=self.monitor_tcn_folder, daemon=True).start()

        def monitor_tcn_folder(self, polling_interval=1.0):
                while True:
                        all_txt = [f for f in os.listdir(self.tcn_path) if f.endswith('.txt') and 'final_data' not in f]
                        new_files = [f for f in all_txt if f not in self.tcn_file_set]

                        if new_files:
                                new_files.sort(key=lambda f: int(f.split('.txt')[0]))
                                for f in new_files:
                                        try:
                                                with open(os.path.join(self.tcn_path, f), 'r') as file:
                                                        lines = [int(line.strip()) for line in file.readlines()]
                                                        values = np.array(lines, dtype='float')
                                                        tcn_tmp = values / (2**(self.bitres - 1) - 1)

                                                with self.tcn_lock:
                                                        self.tcn_total = np.concatenate((self.tcn_total, tcn_tmp))

                                                self.tcn_file_set.add(f)
                                                print(f"[INFO] File aggiunto: {f}, nuovi campioni: {len(tcn_tmp)}")
                                        except Exception as e:
                                                print(f"[ERRORE] Impossibile leggere {f}: {e}")
                        time.sleep(polling_interval)

#Spacchetta il file in elementi intervallati da 20 mt
        def analyze(self):
                n_analizzati = 0
                tot = len(self.Start_seconds)

                # Prepara le colonne, se non esistono
                for col in ['classe_acustica', 'LF[315,1000]_max', 'LF[315,1000]_mean']:
                        if col not in self.gps_total.columns:
                                self.gps_total[col] = np.nan

                while n_analizzati < tot:
                        with self.tcn_lock:
                                tcn_data = self.tcn_total.copy()

                        for n in range(tot):
                                if not np.isnan(self.gps_total.loc[n, 'classe_acustica']):
                                        continue  # già analizzato

                                start_idx = int(self.Start_seconds[n] * self.sr_tcn)
                                end_idx = int(self.End_seconds[n] * self.sr_tcn)

                                if end_idx >= len(tcn_data):
                                        continue  # ancora non ci sono abbastanza dati

                                tcn_evaluation = tcn_data[start_idx:end_idx]

                                if len(tcn_evaluation) < self.nfft:
                                        tcn_filt = sosfilt(self.sos_lf, tcn_evaluation)
                                        spl = 20 * np.log10(np.sqrt(np.mean(tcn_filt**2)))
                                        classe = assegna_classe_acustica(self.popt_fit, 3.6 * self.Velocita[n], spl)

                                        self.gps_total.loc[n, 'classe_acustica'] = int(classe)
                                        self.gps_total.loc[n, 'LF[315,1000]_max'] = spl
                                        self.gps_total.loc[n, 'LF[315,1000]_mean'] = spl
                                else:
                                        spl_list = []
                                        for i in range(0, len(tcn_evaluation) - self.nfft + 1, self.nfft // 2):
                                                tcn_tmp = tcn_evaluation[i:i+self.nfft]
                                                tcn_filt = sosfilt(self.sos_lf, tcn_tmp)
                                                spl_list.append(20 * np.log10(np.sqrt(np.mean(tcn_filt**2))))

                                        spl_mean = np.mean(spl_list)
                                        spl_max = np.max(spl_list)
                                        classe = assegna_classe_acustica(self.popt_fit, 3.6 * self.Velocita[n], spl_max)

                                        self.gps_total.loc[n, 'classe_acustica'] = int(classe)
                                        self.gps_total.loc[n, 'LF[315,1000]_max'] = spl_max
                                        self.gps_total.loc[n, 'LF[315,1000]_mean'] = spl_mean

                                n_analizzati += 1
                                print(f"[ANALISI] Riga {n+1}/{tot} completata.")

                        if n_analizzati < tot:
                                time.sleep(1)
                        else:
                                break
                output_finale = os.path.join('home','ipool','Desktop', 'condivisa', 'GPS_classe_acustica.csv')
                self.gps_total.to_csv(output_finale, index=False)    
                

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


#Ordina i file gps
def ind_gps(file):
        """
        estrae l'indice del file gps per ordinare la sequenza
        """
        return int(file.split('.csv')[0].split('_')[-1])

#Ordina i file TCN
def ind_tcn(file):
        """
        estrae l'indice del file txt del TCN per ordinarli
        """
        return int(file.split('.txt')[0])

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





















