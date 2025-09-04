#include "usr_global.h"
#include "esp_timer.h"

/* Parametri di configurazione WiFi e dimensioni dei buffer */
#define PORT 1234                      // Porta TCP per la comunicazione con il client
#define RX_BUF_SIZE 128                // Dimensione del buffer di ricezione comandi (in byte)
#define NUM_BUFFERS 3                  // Numero di buffer circolari utilizzati per i campioni ADC
#define REC_ADC_CHUNK (256)            // Numero di campioni ADC per ogni buffer (chunk)

/* Variabili globali per buffer circolare e registrazione
 * Gestione di un buffer circolare composto da NUM_BUFFERS blocchi di dimensione REC_ADC_CHUNK.
 * - current_buffer_index: indice del buffer attualmente in uso per l'acquisizione dei nuovi campioni.
 * - writer_buffer_index: indice del prossimo buffer da cui il task di scrittura dovrà leggere per salvare su file.
 * - buffer_full: array di flag (0/1) che indica se ciascun buffer ha raggiunto la capacità ed è pronto per essere scritto su file.
 * - micro_rec_adc_data_chunck: memoria per i dati dei campioni ADC (array 2D [NUM_BUFFERS][REC_ADC_CHUNK]).
 * - micro_rec_ps: contatore di quanti campioni sono stati registrati nel buffer corrente (indice di posizione all'interno del buffer corrente).
 */
volatile uint8_t current_buffer_index = 0;
volatile uint8_t writer_buffer_index = 0;
volatile uint8_t buffer_full[NUM_BUFFERS] = {0, 0, 0};
int32_t micro_rec_adc_data_chunck[NUM_BUFFERS][REC_ADC_CHUNK];
volatile uint32_t micro_rec_ps = 0;

/* Flag di controllo registrazione e struttura dati ADC
 * - micro_rec_start: flag (0/1) che indica se la registrazione audio è attiva.
 * - ads131_data: struttura dati contenente i valori campionati dall'ADC (es. valori dei canali del convertitore ADS131M0x).
 * - micro_rec_sync_delay, micro_rec_flag: variabili ausiliarie per sincronizzazione (non pienamente utilizzate in questo codice).
 */
volatile uint8_t micro_rec_start = 0;
ads1310m0x_adc_t ads131_data;
uint8_t micro_rec_sync_delay = 0;
uint8_t micro_rec_flag = 0;

/* Variabili per monitorare la frequenza di campionamento e statistiche di registrazione
 * - sample_counter: conta il numero totale di campioni raccolti dall'inizio della registrazione corrente.
 * - samples_written_in_second: conta quanti campioni sono stati scritti su file nell'ultimo intervallo di un secondo (per monitorare la frequenza effettiva).
 * - last_written_time: timestamp (in millisecondi) dell'ultima volta in cui è stato scritto sul file il conteggio dei campioni per secondo.
 * - start_time, end_time: timestamp di inizio e fine della registrazione, usati per calcolare la durata totale.
 * - total_samples: contatore totale di campioni registrati (non utilizzato attivamente in questo codice).
 */
uint32_t last_written_time = 0;
uint32_t start_time = 0;

/* Altre variabili globali di stato 
 * - scan_done: flag impostato a 1 al completamento di una scansione WiFi (evento WIFI_EVENT_SCAN_DONE).
 * - rec_writer_handle: handle del task FreeRTOS che scrive su file (recording_writer_task), per evitare di crearne duplicati.
 * - flag: flag di attivazione della scrittura (1 se il task di scrittura deve attivo perché la registrazione è in corso).
 * - client_sock_global: socket TCP del client attualmente connesso (inizializzato a -1 quando nessun client è connesso).
 * - rec_file: puntatore al file di registrazione aperto su SD card.
 * - rec_file_path: percorso (path) del file di registrazione sulla SD card.
 */
volatile uint8_t scan_done = 0;
TaskHandle_t rec_writer_handle = NULL;
int flag = 0;
int client_sock_global = -1;  // inizialmente -1, indica nessun client connesso
FILE *rec_file;
char rec_file_path[64];

/* Gestore evento WiFi (completamento scansione)
 * Callback invocata automaticamente al verificarsi di eventi WiFi. In particolare, se la scansione WiFi (esp_wifi_scan_start) termina,
 * intercetta l'evento WIFI_EVENT_SCAN_DONE e imposta il flag scan_done a 1.
 */
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        scan_done = 1;
    }
}

/* Funzione di utilità per ottenere il tempo in millisecondi 
 * Ritorna il numero di millisecondi trascorsi dall'avvio (esp_timer_get_time fornisce microsecondi, divisi per 1000).
 */
uint32_t millis() {
    return esp_timer_get_time() / 1000;  // converte i microsecondi ottenuti in millisecondi
}

/* ISR (Interrupt Service Routine) del ADC - gestore dell'interrupt di DRDY
 * Questa routine viene eseguita ad ogni impulso sul pin DRDY del convertitore ADC (segnale di "data ready").
 * Se la registrazione è attiva (micro_rec_start == 1), legge un nuovo campione dall'ADC ADS131M0x e lo memorizza nel buffer circolare:
 *   - Incrementa il contatore totale dei campioni raccolti (sample_counter).
 *   - Legge il valore ADC chiamando ADS131M0xreadADC(&ads131_data) e preleva il canale di interesse (ads131_data.ch0).
 *   - Salva il campione nel buffer corrente (micro_rec_adc_data_chunck[current_buffer_index] all'indice micro_rec_ps) e incrementa micro_rec_ps.
 *   - Se il buffer corrente è pieno (micro_rec_ps raggiunge REC_ADC_CHUNK), marca il buffer come completo (buffer_full) e passa al buffer successivo ciclicamente, resettando micro_rec_ps.
 * La macro IRAM_ATTR indica che la funzione è posizionata in IRAM per un'esecuzione rapida in contesto di interrupt.
 */
static void IRAM_ATTR micro_isr_handler(void* arg) {
    if (micro_rec_start == 1) {                      // Esegue le operazioni solo se la registrazione è attiva
        ADS131M0xreadADC(&ads131_data);              // Legge un nuovo campione dal convertitore ADC (popola ads131_data)
        int32_t lDat = ads131_data.ch0;              // Estrae il valore del canale 0 (microfono analogico) dai dati ADC
        micro_rec_adc_data_chunck[current_buffer_index][micro_rec_ps] = lDat;  // Salva il campione nel buffer circolare corrente
        micro_rec_ps++;                              // Avanza l'indice nel buffer corrente
        if (micro_rec_ps >= REC_ADC_CHUNK) {         // Se il buffer corrente è pieno:
            buffer_full[current_buffer_index] = 1;                           //   - Segna il buffer corrente come completo e pronto per la scrittura su file
            current_buffer_index = (current_buffer_index + 1) % NUM_BUFFERS; //   - Passa al buffer successivo (ciclo circolare sugli indici dei buffer)
            micro_rec_ps = 0;                                                //   - Resetta l'indice del buffer (inizia a riempire il prossimo buffer dall'inizio)
        }
    }
}

/* Task FreeRTOS per la scrittura dei campioni ADC su file (SD card)
 * Questo task viene creato all'avvio della registrazione ('s') e rimane in esecuzione finché il dispositivo è acceso.
 * Si occupa di verificare continuamente se vi sono buffer completi di dati ADC da scrivere su file:
 *   - Controlla l'indice writer_buffer_index; se il buffer corrispondente è pieno (buffer_full == 1) e la registrazione è attiva (flag == 1),
 *     scrive tutti i campioni di quel buffer nel file di registrazione (rec_file) su SD card.
 *   - Durante la scrittura di ogni campione incrementa samples_written_in_second. Quando è trascorso ~1 secondo dall'ultimo aggiornamento (>=1000 ms),
 *     scrive sul file il numero di campioni registrati in quell'ultimo secondo ("Recorded X") e fa flush per assicurare la scrittura su SD, quindi azzera il contatore per il secondo successivo.
 *   - Dopo aver scritto tutti i campioni del buffer, effettua un fflush finale, segna il buffer come libero (buffer_full = 0) e passa al buffer successivo (writer_buffer_index avanzato ciclicamente).
 * Infine chiama vTaskDelay(1) per cedere la CPU ed evitare di impegnarla continuamente (lasciando tempo ad altri task, inclusa l'ISR ADC, di operare).
 */
void recording_writer_task(void *pvParameters) {
    while (1) {
        if (flag == 1 && buffer_full[writer_buffer_index]) {
            // Buffer indicato da writer_buffer_index completo e scrittura attiva
            for (int j = 0; j < REC_ADC_CHUNK; j++) {
                fprintf(rec_file, "%ld\n", micro_rec_adc_data_chunck[writer_buffer_index][j]);  // Scrive il campione j-esimo sul file (in formato testo)
                if ((millis() - last_written_time) >= 1000) {  // Se è passato ~1 secondo dall'ultimo aggiornamento sul file
                    fprintf(rec_file, "\n");  // Scrive sul file quanti campioni sono stati registrati in questo secondo
                    fflush(rec_file);  // Scarica su SD i dati finora scritti (svuota buffer di file system)
                    last_written_time = millis();   // Aggiorna il timestamp dell'ultimo aggiornamento
                }
            }
            fflush(rec_file);  // Assicura che tutti i dati del buffer siano scritti su file
            buffer_full[writer_buffer_index] = 0;  // Marca il buffer come elaborato (libero per essere riempito di nuovo)
            writer_buffer_index = (writer_buffer_index + 1) % NUM_BUFFERS;  // Aggiorna l'indice del buffer da scrivere (ciclico)
        }
        vTaskDelay(1);  // Attende 1 tick (cedendo la CPU ad altri task)
    }
}

/* Funzione di supporto: invio di un file di registrazione via TCP
 * Apre il file specificato da file_path in modalità lettura e ne invia il contenuto sul socket TCP `sock` al client connesso.
 * Se il file non esiste o non può essere aperto, invia un messaggio di errore al client.
 * Al termine, chiude il file aperto.
 */
void send_file_over_tcp(int sock, const char *file_path) {
    FILE *fr = fopen(file_path, "r");
    if (fr == NULL) {
        printf("Error opening file for reading: %s\n", file_path);
        const char *err = "ERROR: File not found or cannot be opened.\n";
        send(sock, err, strlen(err), 0);  // Notifica l'errore al client via socket
        return;
    }
    char buffer[256];
    size_t bytes_read;
    // Legge dal file in blocchi e invia ogni blocco al client finché ci sono dati
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fr)) > 0) {
        send(sock, buffer, bytes_read, 0);
    }
    fclose(fr);
    printf("Sent contents of %s to client.\n", file_path);
}

/* Inizializzazione Access Point WiFi (modalità AP)
 * Configura l'ESP32 come Access Point WiFi con SSID e password specificati, quindi avvia la rete WiFi.
 * Passi:
 * 1. Inizializza la flash NVS (usata da esp_wifi) e, se necessario, la cancella in caso di errori di partizionamento.
 * 2. Inizializza lo stack di rete di base (esp_netif) e il loop di eventi di default.
 * 3. Crea un'interfaccia di rete WiFi in modalità AP con parametri di default (esp_netif_create_default_wifi_ap).
 * 4. Imposta manualmente l'indirizzo IP dell'AP a 192.168.4.1 (gateway uguale) con netmask 255.255.255.0.
 * 5. Inizializza il modulo WiFi (esp_wifi_init) con configurazione di default.
 * 6. Configura i parametri dell'Access Point (wifi_config):
 *    - SSID (nome rete) e password (se fornita; se la password è vuota configura l'AP come aperto senza autenticazione).
 *    - Modalità di autenticazione: WPA2-PSK se c'è password, OPEN altrimenti.
 *    - Canale WiFi (1), numero massimo di stazioni connesse (4) e intervallo beacon (100 ms).
 * 7. Imposta la modalità WiFi su AP e applica la configurazione, quindi avvia l'AP (esp_wifi_start).
 * Ritorna ESP_OK se l'AP è stato avviato con successo, altrimenti un codice di errore esp_err_t.
 */
esp_err_t WIFIinitAP(const char *ap_ssid, const char *ap_password) {
    // Inizializza NVS (Non-Volatile Storage) per il WiFi
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
        if (ret != ESP_OK) return ret;
    }
    // Inizializza stack di rete e loop di eventi di default (ignora ESP_ERR_INVALID_STATE se già inizializzato)
    ret = esp_netif_init();
    if (ret != ESP_OK) return ret;
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) return ret;
    // Crea l'interfaccia di rete WiFi per l'AP
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    if (ap_netif == NULL) return ESP_FAIL;
    // Imposta IP, gateway e netmask dell'AP (IP fisso 192.168.4.1)
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    // Inizializza il modulo WiFi con la configurazione di default
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) return ret;
    // Configura i parametri dell'Access Point (SSID, password e sicurezza)
    wifi_config_t wifi_config = { 0 };
    strncpy((char *)wifi_config.ap.ssid, ap_ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid_len = strlen(ap_ssid);
    if (ap_password && strlen(ap_password) > 0) {
        strncpy((char *)wifi_config.ap.password, ap_password, sizeof(wifi_config.ap.password) - 1);
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    wifi_config.ap.channel = 1;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.beacon_interval = 100;
    // Imposta la modalità WiFi in Access Point e applica la configurazione
    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK) return ret;
    ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (ret != ESP_OK) return ret;
    // Avvia l'Access Point WiFi
    return esp_wifi_start();
}

/* Task server TCP principale 
 * Crea un socket TCP in ascolto sulla porta specificata (PORT) e gestisce la comunicazione con un client (PC) connesso via WiFi.
 * Operazioni:
 *   - Creazione del socket server (IPv4, TCP) e binding sulla porta PORT. Se fallisce, termina il task.
 *   - Messa in ascolto (listen) del socket per accettare una connessione alla volta.
 *   - Attesa di una connessione in arrivo (accept bloccante). Quando un client si connette, configura l'interrupt del pin DRDY dell'ADC e registra la ISR micro_isr_handler per acquisire i campioni in tempo reale.
 *   - Loop di gestione comandi dal client tramite socket TCP:
 *       > **s** (Start): avvia la registrazione audio.
 *         - Resetta parametri di sincronizzazione (micro_rec_sync_delay, micro_rec_ps, micro_rec_flag) e genera un impulso sul pin SYNC dell'ADC per sincronizzare il convertitore.
 *         - Apre (crea/sovrascrive) il file di registrazione sulla SD card (percorso in rec_file_path).
 *         - Imposta i flag di avvio: micro_rec_start = 1 (attiva acquisizione nell'ISR) e flag = 1 (attiva il writer task).
 *         - Se il task di scrittura su file non è già stato creato, lo crea in questo momento.
 *         - Registra il tempo di inizio (start_time) e inizializza last_written_time per il conteggio dei campioni al secondo.
 *       > **n** (Stop): interrompe la registrazione in corso.
 *         - Disattiva l'acquisizione (micro_rec_start = 0) e la scrittura su file (flag = 0).
 *         - Attende 50 tick (~50 ms) per consentire al writer task di completare eventuali scritture in corso sui buffer pieni.
 *         - Scrive su file gli eventuali campioni rimanenti nel buffer corrente (che potrebbe non essere pieno al momento dello stop).
 *         - Registra il tempo di fine (end_time) e calcola la durata totale della registrazione in secondi.
 *         - Calcola la frequenza di campionamento media effettiva (sample_counter / elapsed_time).
 *         - Scrive alla fine del file un riepilogo con il totale dei campioni registrati, la durata e la frequenza di campionamento (S/s), quindi chiude il file.
 *       > **r** (Read/Send): apre il file di registrazione salvato e lo invia interamente al client via socket TCP. Se il file non esiste o non è apribile, invia un messaggio di errore al client.
 *       > **x** (Exit): termina la connessione con il client (il loop dei comandi viene interrotto e si chiuderà il socket).
 *   - Quando il client si disconnette o invia 'x', la funzione rimuove l'ISR dall'ADC, ripristina il pin DRDY e chiude il socket client. Il task torna quindi ad aspettare un nuovo client (loop principale).
 *   - In caso di errore sull'accept (client_sock < 0), esce dal loop principale, chiude il socket di ascolto e termina il task.
 */
void tcp_server_task(void *pvParameters) {
    char rx_buffer[RX_BUF_SIZE];
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);
    // Crea un socket TCP IPv4
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0) {
        vTaskDelete(NULL);  // Errore nella creazione del socket, termina il task
        return;
    }
    // Configura l'indirizzo e la porta del server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // Accetta connessioni su qualsiasi interfaccia di rete
    server_addr.sin_port = htons(PORT);               // Porta TCP di ascolto
    // Effettua il bind del socket all'indirizzo e porta specificati
    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        close(listen_sock);
        vTaskDelete(NULL);  // Errore nel binding (es. porta già in uso), termina il task
        return;
    }
    // Mette il socket in ascolto (backlog massimo 1 connessione pendente)
    if (listen(listen_sock, 1) != 0) {
        close(listen_sock);
        vTaskDelete(NULL);  // Errore nell'entrare in ascolto, termina il task
        return;
    }
    // Prepara il percorso completo del file di registrazione (montando la directory di SD card e nome file)
    sprintf(rec_file_path, "%s/%s", MOUNT_POINT, "TEST.txt");
    // Loop principale: accetta un client alla volta
    while (1) {
        // Attende una connessione TCP in ingresso (bloccante finché un client non si connette)
        client_sock_global = accept(listen_sock, (struct sockaddr *)&client_addr, (socklen_t*)&addr_len);
        if (client_sock_global < 0) {
            break;  // esce dal loop principale in caso di errore di accept
        }
        // Configura il pin DRDY dell'ADC come input con interrupt sul fronte di discesa, e collega la ISR di gestione ADC
        gpio_config_t io_conf = {0};
        io_conf.intr_type = GPIO_INTR_NEGEDGE;
        io_conf.pin_bit_mask = (1ULL << DRDY_GPIO);
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        gpio_config(&io_conf);
        if (gpio_install_isr_service(0) == ESP_OK) {
            gpio_isr_handler_add(DRDY_GPIO, micro_isr_handler, (void*) DRDY_GPIO);
        }
        // Loop di gestione dei comandi inviati dal client tramite TCP
        while (1) {
            int len = recv(client_sock_global, rx_buffer, RX_BUF_SIZE - 1, 0);
            if (len <= 0) {
                break;  // il client ha chiuso la connessione o si è verificato un errore di ricezione
            }
            rx_buffer[len] = '\0';  // Termina la stringa ricevuta con carattere null
            if (strcmp(rx_buffer, "x") == 0) {
                break;  // comando 'x': richiesta di terminazione della connessione
            }
            if (strcmp(rx_buffer, "s") == 0) {
                // Comando 's' (start): avvia una nuova registrazione
                micro_rec_sync_delay = 0;
                micro_rec_ps = 0;
                micro_rec_flag = 0;
                // Genera un impulso di sincronizzazione sul pin SYNC dell'ADC (allinea/azzera il convertitore)
                gpio_set_level(SYNC_GPIO, 0);
                gpio_set_level(SYNC_GPIO, 1);
                // Apre (crea) il file di registrazione su SD card in modalità scrittura
                rec_file = fopen(rec_file_path, "w");
                if (rec_file == NULL) {
                    printf("Error opening file for writing\n");
                    break;  // errore nell'apertura del file, esce senza avviare la registrazione
                }
                micro_rec_start = 1;  // attiva l'acquisizione nell'ISR ADC
                flag = 1;             // abilita il task di scrittura su file
                // Crea il task di scrittura su file se non già avviato
                if (rec_writer_handle == NULL) {
                    xTaskCreate(recording_writer_task, "rec_writer", 4096, NULL, tskIDLE_PRIORITY + 1, &rec_writer_handle);
                }
                start_time = millis();        // registra il tempo di inizio della registrazione
                last_written_time = millis(); // inizializza il riferimento temporale per il conteggio campioni/sec
            }
            else if (strcmp(rx_buffer, "n") == 0) {
                // Comando 'n' (stop): termina la registrazione corrente
                micro_rec_start = 0;  // disabilita ulteriori acquisizioni dall'ADC
                flag = 0;             // indica al task di scrittura di fermarsi dopo aver svuotato i buffer
                vTaskDelay(50);       // attende ~50 ms per permettere al writer task di completare la scrittura dei buffer pieni
                // Scrive sul file eventuali campioni residui nel buffer corrente (non completo al momento dello stop)
                if (micro_rec_ps > 0) {
                    for (int i = 0; i < micro_rec_ps; i++) {
                        char line[16];
                        int len_line = snprintf(line, sizeof(line), "%ld\n", micro_rec_adc_data_chunck[current_buffer_index][i]);
                        fwrite(line, 1, len_line, rec_file);
                    }
                }
                fprintf(rec_file, ".\n");
                fflush(rec_file);
                fclose(rec_file);  // chiude il file di registrazione
            }
            else if (strcmp(rx_buffer, "r") == 0) {
                // Comando 'r' (read/send file): invia il file di registrazione al client via WiFi
                printf("Received 'r' command. Trying to open file: %s\n", rec_file_path);
                FILE *fr = fopen(rec_file_path, "r");
                if (fr != NULL) {
                    // File aperto con successo: invia il contenuto al client tramite socket
                    fclose(fr);
                    send_file_over_tcp(client_sock_global, rec_file_path);
                } else {
                    // Errore nell'apertura del file (file non trovato o non accessibile)
                    printf("Error opening file for reading: %s\n", rec_file_path);
                    char err[128];
                    snprintf(err, sizeof(err), "ERROR: File not found or cannot be opened: %s\n", rec_file_path);
                    send(client_sock_global, err, strlen(err), 0);
                }
            }
        }  // Fine del loop di ricezione comandi dal client
        // Pulizia delle risorse dopo la disconnessione del client
        gpio_isr_handler_remove(DRDY_GPIO);        // Rimuove l'ISR dal pin DRDY
        gpio_reset_pin(DRDY_GPIO);
        gpio_set_direction(DRDY_GPIO, GPIO_MODE_INPUT);
        gpio_uninstall_isr_service();              // Disinstalla il servizio di interrupt GPIO (libera risorse)
        close(client_sock_global);                 // Chiude il socket con il client
        client_sock_global = -1;
    }  // Fine del loop principale di accept (attesa nuovi client)
    // Se esce dal loop principale, chiude il socket di ascolto e termina il task
    close(listen_sock);
    vTaskDelete(NULL);
}
