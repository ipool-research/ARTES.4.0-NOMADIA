import pandas as pd
import folium
from branca.element import Template, MacroElement

def get_color(indice):
    return {
        0: 'green',
        1: 'yellow',
        2: 'orange',
        3: 'red'
    }.get(indice, 'gray')

def crea_mappa_con_bordi(path_csv, output_html='mappa_intervalli_bordi.html'):
    df = pd.read_csv(path_csv)

    req = ['Latitude_inizio','Longitude_inizio','Latitude_fine','Longitude_fine','indice_finale','id_intervallo']
    if not all(c in df.columns for c in req):
        raise ValueError(f"Il CSV deve contenere le colonne: {req}")

    centro = [df['Latitude_inizio'].iloc[0], df['Longitude_inizio'].iloc[0]]
    m = folium.Map(location=centro, zoom_start=17)

    for _, row in df.iterrows():
        start = [row['Latitude_inizio'], row['Longitude_inizio']]
        end = [row['Latitude_fine'], row['Longitude_fine']]
        col = get_color(row['indice_finale'])

        # linea di contorno nera più spessa
        folium.PolyLine(locations=[start, end],
                        color='black',
                        weight=10,
                        opacity=1).add_to(m)

        # linea colorata più sottile sopra
        folium.PolyLine(locations=[start, end],
                        color=col,
                        weight=6,
                        opacity=0.8,
                        tooltip=f"Intervallo {row['id_intervallo']}: indice {int(row['indice_finale'])}"
                       ).add_to(m)

    # Aggiungi legenda
    legenda_html = """
    {% macro html(this, kwargs) %}
    <div style="
        position: fixed; 
        bottom: 50px; left: 50px; 
        width: 220px; 
        z-index:9999; 
        font-size:14px;
        background-color: white;
        border: 2px solid grey;
        border-radius: 5px;
        padding: 10px;
        box-shadow: 2px 2px 6px rgba(0,0,0,0.3);
    ">
        <b>Legenda</b><br>
        <div style="display: flex; align-items: center; margin-bottom: 5px;">
            <span style="background:green; width:15px; height:15px; display:inline-block; margin-right:8px;"></span>
            tratta in buone condizioni
        </div>
        <div style="display: flex; align-items: center; margin-bottom: 5px;">
            <span style="background:yellow; width:15px; height:15px; display:inline-block; margin-right:8px;"></span>
            tratta in medie condizioni
        </div>
        <div style="display: flex; align-items: center; margin-bottom: 5px;">
            <span style="background:orange; width:15px; height:15px; display:inline-block; margin-right:8px;"></span>
            tratta in mediocri condizioni
        </div>
        <div style="display: flex; align-items: center;">
            <span style="background:red; width:15px; height:15px; display:inline-block; margin-right:8px;"></span>
            tratta in pessime condizioni
        </div>
    </div>
    {% endmacro %}
    """
    legenda = MacroElement()
    legenda._template = Template(legenda_html)
    m.get_root().add_child(legenda)

    m.save(output_html)
    print("Mappa salvata in:", output_html)
    return output_html

# Esempio:
crea_mappa_con_bordi("/home/ipool/Desktop/main_scheda/misure/output.csv", "/home/ipool/Desktop/main_scheda/misure/mappa_intervalli_bordi.html")
