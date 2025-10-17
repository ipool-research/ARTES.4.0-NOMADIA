import pandas as pd
import os
import re
import folium
from branca.element import Element as FoliumElement
from concurrent.futures import ProcessPoolExecutor, as_completed
import traceback

class AmmaloramentiProcessor:
    def __init__(self, cartella, gps):
        self.cartella_input = os.path.join(cartella, 'intervalli')
        self.gps_file = gps
        self.output_file = os.path.join(cartella, 'output_finale.csv')
        self.max_workers = 4
        self.output_html = os.path.join(cartella, 'mappa_intervalli_bordi.html')

    @staticmethod
    def get_color(indice: int) -> str:
        """
        Restituisce il colore corretto in base all'indice.
        Questa mappatura corrisponde a quella definita nella legenda HTML.
        """
        return {
            0: 'green',   # Buono
            1: 'yellow',  # Medio
            2: 'orange',  # Mediocre
            3: 'red'      # Pessimo
        }.get(int(indice), 'gray')

    def calcola_somma_colonne(self, df):
        cols = [c for c in df.columns if c not in ("frame", "Unnamed: 0")]
        num = df[cols].apply(pd.to_numeric, errors="coerce").fillna(0)
        return pd.DataFrame([num.sum()])

    def calcola_totali_accorpati(self, sum_df):
        poth = {
            "tipo": "Alligator/Pothole",
            "SX/BOTH": int(sum_df[["Alligator_both","Pothole_both","Alligator_sx","Pothole_sx"]].sum(axis=1).iloc[0]),
            "DX": int(sum_df[["Alligator_dx","Pothole_dx"]].sum(axis=1).iloc[0]),
            "FUORI": int(sum_df[["Alligator","Pothole"]].sum(axis=1).iloc[0])
        }
        man = {
            "tipo": "Manhole",
            "SX/BOTH": int(sum_df[["Manhole_both","Manhole_sx"]].sum(axis=1).iloc[0]),
            "DX": int(sum_df[["Manhole_dx"]].sum(axis=1).iloc[0]),
            "FUORI": int(sum_df[["Manhole"]].sum(axis=1).iloc[0])
        }
        crack = {
            "tipo": "Longitudinal/Transverse",
            "SX/BOTH": int(sum_df[["Longitudinal_both","Transverse_both","Longitudinal_sx","Transverse_sx"]].sum(axis=1).iloc[0]),
            "DX": int(sum_df[["Longitudinal_dx","Transverse_dx"]].sum(axis=1).iloc[0]),
            "FUORI": int(sum_df[["Longitudinal","Transverse"]].sum(axis=1).iloc[0])
        }
        return poth, man, crack

    def process_intervallo(self, file_path, id_intervallo, classe_acustica):
        try:
            df = pd.read_csv(file_path)
            sum_df = self.calcola_somma_colonne(df)
            poth, man, crack = self.calcola_totali_accorpati(sum_df)

            cls_eff = classe_acustica
            if cls_eff != -1 and man["SX/BOTH"] > 0:
                cls_eff -= 1

            if cls_eff == -1 or cls_eff < 0:
                indice = calcola_indice(df)
            else:
                indice = calcola_indice_per_riga(
                    cls_eff,
                    poth["SX/BOTH"], poth["DX"], poth["FUORI"],
                    crack["SX/BOTH"], crack["DX"], crack["FUORI"]
                )

            result = {'id_intervallo': id_intervallo}
            row = sum_df.iloc[0]
            for col in row.index:
                result[col] = int(row[col])
            result['indice_finale'] = int(indice)
            return result
        except Exception as e:
            return {'id_intervallo': id_intervallo, 'error': traceback.format_exc()}

    def run(self):
        ammaloramenti_cols = [
            'Longitudinal', 'Transverse', 'Alligator', 'Pothole', 'Manhole',
            'Longitudinal_sx', 'Transverse_sx', 'Alligator_sx', 'Pothole_sx', 'Manhole_sx',
            'Longitudinal_dx', 'Transverse_dx', 'Alligator_dx', 'Pothole_dx', 'Manhole_dx',
            'Longitudinal_both', 'Transverse_both', 'Alligator_both', 'Pothole_both', 'Manhole_both',
            'indice_finale'
        ]

        gps_df = pd.read_csv(self.gps_file)
        pattern = re.compile(r'INTERVALLO_(\d+)\.csv$', re.IGNORECASE)
        
        tasks = []
        for fname in os.listdir(self.cartella_input):
            m = pattern.match(fname)
            if m:
                x = int(m.group(1))
                if x < len(gps_df):
                    cls = gps_df.iloc[x]['classe_acustica']
                    tasks.append((os.path.join(self.cartella_input, fname), x, cls))

        future_to_params = {}
        results_dict = {}

        with ProcessPoolExecutor(max_workers=self.max_workers) as exe:
            for t in tasks:
                future = exe.submit(self.process_intervallo, *t)
                future_to_params[future] = t

            for fut in as_completed(future_to_params):
                t = future_to_params[fut]
                try:
                    res = fut.result()
                    results_dict[t[1]] = res
                except Exception as e:
                    results_dict[t[1]] = {'id_intervallo': t[1], 'error': traceback.format_exc()}

        results = []
        for i in range(len(gps_df)):
            gps_data = gps_df.iloc[i].to_dict()
            row = {'id_intervallo': i, **gps_data}

            res = results_dict.get(i)
            if res and 'error' not in res:
                for k, v in res.items():
                    if k != 'id_intervallo':
                        row[k] = v
            else:
                for col in ammaloramenti_cols:
                    row[col] = 0
                if res and 'error' in res:
                    row['error'] = res['error']

            results.append(row)

        df = pd.DataFrame(results)

        if 'error' in df.columns:
            err_df = df[df['error'].notna()]
            if not err_df.empty:
                print("⚠️ Errori riscontrati nei seguenti id_intervallo:")
                print(err_df[['id_intervallo', 'error']])
            df = df.drop(columns=['error'])

        col_order = [
            'id_intervallo', 'Start', 'End', 'Latitude_inizio', 'Latitude_fine',
            'Longitude_inizio', 'Longitude_fine', 'Velocità',
            'LF[315,1000]_mean', 'LF[315,1000]_max', 'classe_acustica'
        ]
        
        existing_cols = [col for col in df.columns]
        col_order_final = col_order + [col for col in ammaloramenti_cols if col in existing_cols and col not in col_order]
        df = df[col_order_final]
        df = df.sort_values('id_intervallo').reset_index(drop=True)
        df.to_csv(self.output_file, index=False)
        print("✅ Output salvato in", self.output_file)

    def build_map(self):
        """
        Crea una mappa da un file CSV, disegna linee con contorni,
        aggiunge una legenda e salva il risultato in un file HTML.
        """
        try:
            df = pd.read_csv(self.output_file)
        except FileNotFoundError:
            print(f"Errore: file {self.output_file} non trovato. Esegui prima il metodo .run()")
            return

        req = ['Latitude_inizio','Longitude_inizio','Latitude_fine','Longitude_fine','indice_finale','id_intervallo']
        if not all(c in df.columns for c in req):
            raise ValueError(f"Il CSV deve contenere le colonne: {req}")

        if df.empty:
            print("CSV vuoto, creo una mappa centrata sull'Italia.")
            m = folium.Map(location=[42.5, 12.5], zoom_start=5)
        else:
            centro = [df['Latitude_inizio'].mean(), df['Longitude_inizio'].mean()]
            m = folium.Map(location=centro, zoom_start=17)
            
            all_points = []
            for _, row in df.iterrows():
                start = [row['Latitude_inizio'], row['Longitude_inizio']]
                end = [row['Latitude_fine'], row['Longitude_fine']]
                all_points.extend([start, end])
                
                col = self.get_color(row['indice_finale'])

                folium.PolyLine(locations=[start, end], color='black', weight=10, opacity=1).add_to(m)
                folium.PolyLine(
                    locations=[start, end],
                    color=col,
                    weight=6,
                    opacity=0.8,
                    tooltip=f"Intervallo {row['id_intervallo']}: indice {int(row['indice_finale'])}"
                ).add_to(m)
            
            if all_points:
                m.fit_bounds(all_points)

        # Definiamo l'HTML per la nostra legenda
        legend_html = '''
        <div style="
            position: fixed; 
            bottom: 50px; left: 50px; width: 200px; height: 150px; 
            background-color: white; z-index:9999; font-size:14px;
            border:2px solid grey; padding: 10px; border-radius: 5px;">
        <b>Stato pavimentazione:</b><br>
        <i style="background:green; width:10px; height:10px; display:inline-block; margin-right:5px;"></i> Buono (0)<br>
        <i style="background:yellow; width:10px; height:10px; display:inline-block; margin-right:5px;"></i> Medio (1)<br>
        <i style="background:orange; width:10px; height:10px; display:inline-block; margin-right:5px;"></i> Mediocre (2)<br>
        <i style="background:red; width:10px; height:10px; display:inline-block; margin-right:5px;"></i> Pessimo (3)<br>
        </div>
        '''
        
        # --- CORREZIONE APPLICATA QUI ---
        # Aggiungiamo la legenda come un elemento HTML direttamente all'oggetto mappa
        m.get_root().html.add_child(FoliumElement(legend_html))

        # Usiamo il metodo .save() che è progettato per scrivere un file HTML pulito
        m.save(self.output_html)
        
        print("Mappa con legenda salvata in:", self.output_html)
        return self.output_html


# --- FINE DELLA CLASSE ---

def calcola_indice(df):
    cols_pot = ["Alligator_both","Pothole_both","Alligator_sx","Pothole_sx",
                "Alligator_dx","Pothole_dx","Alligator","Pothole"]
    cols_crk = ["Longitudinal_both","Transverse_both","Longitudinal_sx","Transverse_sx",
                "Longitudinal_dx","Transverse_dx","Longitudinal","Transverse"]

    pot_df = df[cols_pot].apply(pd.to_numeric, errors="coerce").fillna(0)
    crk_df = df[cols_crk].apply(pd.to_numeric, errors="coerce").fillna(0)

    P_A = int(pot_df.to_numpy().sum())
    L_T = int(crk_df.to_numpy().sum())

    if P_A > 6:
        return 3
    elif P_A > 3:
        return 3 if L_T > 6 else 2
    elif P_A > 0:
        return 2 if L_T > 6 else 1
    else:
        if L_T > 6:
            return 2
        elif L_T > 3:
            return 1
        else:
            return 0
        
def calcola_indice_new(df):
    cols_pot = ["Alligator_both","Pothole_both","Alligator_sx","Pothole_sx",
                "Alligator_dx","Pothole_dx","Alligator","Pothole"]
    cols_crk = ["Longitudinal_both","Transverse_both","Longitudinal_sx","Transverse_sx",
                "Longitudinal_dx","Transverse_dx","Longitudinal","Transverse"]

    pot_df = df[cols_pot].apply(pd.to_numeric, errors="coerce").fillna(0)
    crk_df = df[cols_crk].apply(pd.to_numeric, errors="coerce").fillna(0)

    P_A_ormaie = int(pot_df[["Alligator_both","Pothole_both","Alligator_sx","Pothole_sx","Alligator_dx","Pothole_dx"]].to_numpy().sum())
    
    if P_A_ormaie >= 4:
        P_A = 3
    elif P_A_ormaie >= 1:
        P_A = 2
    else:
        P_A_fuori = int(pot_df[["Alligator","Pothole"]].to_numpy().sum())
        if P_A_fuori >= 4:
            P_A = 2
        elif P_A_fuori >= 1:
            P_A = 1
        else:
            P_A = 0


    L_T_ormaie = int(crk_df[["Longitudinal_both","Transverse_both","Longitudinal_sx","Transverse_sx","Longitudinal_dx","Transverse_dx"]].to_numpy().sum())

    if L_T_ormaie >= 6:
        L_T = 3
    elif L_T_ormaie >= 1:
        L_T = 2
    else:
        L_T_fuori = int(crk_df[["Longitudinal","Transverse"]].to_numpy().sum())
        if L_T_fuori >= 6:
            L_T = 2
        elif L_T_fuori >= 1:
            L_T = 1
        else:
            L_T = 0


    somma_pesata = 1*P_A + 0.8*L_T

    if somma_pesata < 1:
        return 0
    elif somma_pesata < 3.5:
        return 1
    elif somma_pesata < 4.5:
        return 2
    else:
        return 3       

    
def calcola_indice_per_riga(classe_acustica, pot_sx_both, pot_dx, pot_fuori,
                                crk_sx_both, crk_dx, crk_fuori):
        
        # Copiato logic esatto
        if classe_acustica == 0:
            if pot_sx_both > 0:
                pot_indice = 2
            elif pot_dx > 0 or pot_fuori > 0:
                pot_indice = 1
            else:
                pot_indice = 0

            crk_somma = crk_dx + crk_fuori
            if crk_sx_both > 0 or crk_somma > 6:
                crk_indice = 1 
            else:
                crk_indice = 0

        elif classe_acustica == 1:
            if pot_sx_both > 0:
                pot_indice = 3
            elif pot_dx > 0:
                pot_indice = 2
            elif pot_fuori > 0:
                pot_indice = 1
            else:
                pot_indice = 0

            crk_somma =crk_sx_both + crk_dx + crk_fuori
            if crk_sx_both > 6:
                crk_indice = 3
            elif crk_sx_both > 0 and crk_somma > 6:
                crk_indice = 3
            elif crk_sx_both > 0:
                crk_indice = 2
            elif crk_somma > 6:
                crk_indice = 2
            elif crk_dx > 0:
                crk_indice = 1
            else:
                crk_indice = 0
            

        elif classe_acustica == 2:
            if pot_sx_both > 0:
                pot_indice = 3
            elif pot_dx > 0:
                pot_indice = 2
            elif pot_fuori > 0:
                pot_indice = 1
            else:
                pot_indice = 0    

            crk_somma = crk_dx + crk_fuori
            if crk_sx_both > 0:
                crk_indice = 3
            elif crk_dx > 6:
                crk_indice = 3
            elif crk_somma > 6:
                crk_indice = 3
            elif crk_dx > 0:
                crk_indice = 2
            elif crk_fuori > 0:
                crk_indice = 1
            else:
                crk_indice = 0
        else:
            pot_indice = crk_indice = 0

        x = round(pot_indice + crk_indice * 0.8, 2)

        if x == 0 and classe_acustica > 0:
            return 1
        elif 0 < x < 1: 
            return 0
        elif 1 <= x < 2.5: 
            return 1
        elif 2.5 <= x < 4.5: 
            return 2
        elif 4.5 <= x <= 6: 
            return 3
        
        return 0
