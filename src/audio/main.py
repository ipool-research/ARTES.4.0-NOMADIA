import ESP32
import time
import datetime
from datetime import timezone
import threading
import pytz
import os
import gopro
import gps

stop_event = threading.Event()
path = '/home/ipool/Desktop/Artes' #sostituire con il percorso della cartella corrispondente

def wait_for_stop():
    input("Premi ENTER per fermare la registrazione...\n")
    try:
        Gopro.stop_gopro_video()
        print("✅ Stop ricevuto. Fermata la GoPro")
    except:
        print("GoPro disconnessa, continuo la procedura di stop..")

    stop_event.set()


if __name__ == "__main__":
    Gopro = gopro.GoPro()
    #inizializza gps idem
    Gps = gps.GPS(path)
    print("Aspetto un dato valido dal GPS...")
    Gps.start()
    pint = ESP32.esp32() 
    Gps.start_periodic_saving(interval_seconds=30)

    success = Gopro.start_gopro_video()
    #success = True
    if success:
        pint.scrivi_al_socket("s")
        time_name = datetime.datetime.now(timezone.utc).astimezone(pytz.timezone("Europe/Rome"))
        #start_gps
        stop_thread = threading.Thread(target = wait_for_stop)
        stop_thread.start()
        date_str = time_name.strftime("%d-%m-%Y")
        time_str = time_name.strftime("%H_%M_%S")
        esp_filename = f"PINT_REC_{date_str}_{time_str}"
        output_folder = os.path.join(path, 'dati_pint', esp_filename)
        os.makedirs(output_folder, exist_ok=True)
        pint.set_pathname(output_folder)
        Gps.start_processing_thread()

        while not stop_event.is_set():
            time.sleep(0.001)
        
        pint.scrivi_al_socket("n")  
        Gps.stop()
        th = pint.start_queuing()
        
        pint.download_and_process_after_recording()     
        print("finito")

        pint.stop_queue()
        th.join()

    else:
        print("❌ Errore nell'avvio della GoPro. Registrazione annullata.")
    
    print("Fine misura!")
