/*******************************************************************************
 * Progetto     : 82037601
 * Nome         : wifi.h
 * Descr        : Definizioni e prototipi per la gestione della connettività Wi-Fi 
 *                (modalità Access Point) e del server TCP per la trasmissione dati
 *******************************************************************************
 ****/
#ifndef MAIN_DRIVERS_WIFI_H_
#define MAIN_DRIVERS_WIFI_H_

/* Definizione costanti ----------------------------------------------------------*/

/* Definizione variabili globali ---------------------------------------------- */
extern TaskHandle_t rec_writer_handle;  // Handle del task di scrittura su file per la registrazione dei dati ADC

/* Definizione tipi --------------------------------------------------------------*/
typedef struct
{
    uint8_t scan_SSID[32+1];     // SSID della rete trovata tramite scansione (stringa, massimo 32 caratteri + terminatore)
    signed char scan_rssi[5+1];  // Livello di segnale RSSI della rete trovata (stringa, formato numerico con segno, es. "-45")
    uint8_t scan_ch[3+1];        // Canale Wi-Fi della rete trovata (stringa numerica, max 3 cifre, es. "6", "11")
    uint8_t scan_authType[1+1];  // Tipo di autenticazione della rete (stringa di un carattere, es. "0"=open, "4"=WPA/WPA2)
} WIFISCANRESULT;

/* Definizione prototipi ----------------------------------------------------------*/
/* WIFIinitAP: inizializza la modalità Access Point Wi-Fi con SSID e password specificati.
   inp: ap_ssid - SSID della rete Wi-Fi da creare (stringa terminata da null).
        ap_password - Password per la rete (se stringa vuota o NULL, la rete sarà aperta senza password).
   out: ESP_OK (0) se l'Access Point è stato avviato con successo; 
        altrimenti un codice di errore (esp_err_t) che indica il tipo di fallimento. */
esp_err_t WIFIinitAP(const char *ap_ssid, const char *ap_password);

/* tcp_server_task: task FreeRTOS che gestisce un server TCP su una porta predefinita (es. 1234).
   Questa funzione accetta connessioni da client, riceve comandi testuali e svolge le azioni 
   corrispondenti (avvio/arresto della registrazione dei dati ADC, invio di file di dati al client, ecc.).
   inp: pvParameters - parametri del task (non utilizzato in questo caso, può essere NULL).
   out: (nessun valore di ritorno; il task viene eseguito indefinitamente finché il sistema è attivo). */
void tcp_server_task(void *pvParameters);

/* recording_writer_task: task FreeRTOS per la scrittura dei dati ADC su file.
   Si occupa di monitorare i buffer circolari in cui vengono accumulati i campioni ADC 
   e, quando un buffer è pieno, scrive i campioni sul file di registrazione.
   inp: pvParameters - parametri del task (non utilizzato).
   out: (nessun valore di ritorno; il task esegue un loop infinito finché rimane attivo). */
void recording_writer_task(void *pvParameters);

/* send_file_over_tcp: invia il contenuto di un file su una connessione TCP al client.
   Legge il file dal percorso specificato e trasmette i dati binari sul socket.
   inp: sock - socket del client su cui inviare i dati.
        file_path - percorso del file da aprire e inviare.
   out: (nessun valore di ritorno; eventuali errori durante la lettura/invio vengono gestiti 
         all'interno della funzione, ad esempio inviando un messaggio di errore al client). */
void send_file_over_tcp(int sock, const char *file_path);

#endif /* MAIN_DRIVERS_WIFI_H_ */
/*EOF*/
