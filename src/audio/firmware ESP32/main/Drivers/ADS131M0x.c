#include "global.h"
#include "gpio.h"

/* Definizione delle costanti ------------------------------------------ */
/* Abilitare la seguente define se si utilizza la scheda di valutazione (ADS131M0x EVB).
   In questo progetto la disattiviamo per usare la scheda personalizzata (PINT). */
#define ADS131M0x_EVB                   // Per scheda di valutazione (EVB)
#undef ADS131M0x_EVB                    // Disabilitato: usare scheda PINT

#define ADS131M0x_HOST  SPI0_CHANNEL    // Host SPI utilizzato (canale SPI0)

/* Se si utilizza la variante ADS131M02 (2 canali), definisce macro per stampa di un messaggio informativo a compile-time */
#ifdef IS_M02
#define DO_PRAGMA(x) _Pragma (#x)
#define INFO(x) DO_PRAGMA(message ("\nREMARK: " #x))
// INFO Version for ADS131M02
#endif

/* Definizione delle variabili esterne --------------------------------- */
/* (nessuna variabile esterna dichiarata in questo modulo) */

/* Definizione delle Funzioni esterne ---------------------------------- */
/* (dichiarazioni di funzioni esposte pubblicamente, se necessario) */

/* Definizione delle variabili ----------------------------------------- */
spi_device_handle_t ADS1310Mspi;       // Handle per il dispositivo SPI dell'ADC ADS131M0x

uint8_t ads1310mTxBuffer[128];         // Buffer di trasmissione SPI (fino a 128 byte)
uint8_t ads1310mRxBuffer[128];         // Buffer di ricezione SPI (fino a 128 byte)

uint8_t ads1310m_csPin;               // GPIO utilizzato come Chip Select (CS) per l'ADC
uint8_t ads1310m_drdyPin;             // GPIO utilizzato per il segnale Data Ready (DRDY) dall'ADC
uint8_t ads1310m_resetPin;            // GPIO utilizzato per il reset hardware dell'ADC

/* Prototipi delle funzioni interne (procedure) ------------------------ */
esp_err_t ADS131M0xwriteRegister(uint8_t address, uint16_t value);
esp_err_t ADS131M0xreadRegister(uint8_t address, uint16_t *data);
esp_err_t ADS131M0xwriteRegisterMasked(uint8_t address, uint16_t value, uint16_t mask);

/**
 * @brief Scrive un valore in un registro dell'ADS131M0x tramite SPI.
 * 
 * La funzione costruisce il comando SPI per la scrittura e invia il valore specificato al registro indicato.
 * Dopo la trasmissione, verifica che l'ADC abbia confermato l'indirizzo di registro scritto.
 * 
 * @param address Indirizzo del registro da scrivere (7 bit significativi validi).
 * @param value   Valore a 16 bit da scrivere nel registro.
 * @return esp_err_t Ritorna ESP_OK se la scrittura è riuscita (registro confermato), altrimenti ESP_FAIL.
 */
esp_err_t ADS131M0xwriteRegister(uint8_t address, uint16_t value)
{
    esp_err_t ret;
    spi_transaction_t t;    
    uint16_t wDat;
    uint8_t addressRcv;
    uint8_t i = 0;
    uint16_t cmd = 0;

    // Costruisce il comando a 16 bit per scrivere in un registro: combina il codice di comando e l'indirizzo
    cmd = (CMD_WRITE_REG) | (address << 7) | 0;
    // Divide il comando a 16 bit in due byte (MSB e LSB) e aggiunge un terzo byte 0x00 come padding
    ads1310mTxBuffer[i++] = HI_UINT16(cmd);
    ads1310mTxBuffer[i++] = LO_UINT16(cmd);
    ads1310mTxBuffer[i++] = 0x00;
    // Inserisce il valore da scrivere nel registro, anch'esso suddiviso in due byte, seguito da un byte 0x00 di padding
    ads1310mTxBuffer[i++] = HI_UINT16(value);
    ads1310mTxBuffer[i++] = LO_UINT16(value);
    ads1310mTxBuffer[i++] = 0x00;
    // Aggiunge byte dummy addizionali a 0x00 per completare il frame SPI richiesto dall'ADC
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    // (Il numero totale di byte inviati dipende dal numero di canali: 
    //  nel caso di ADS131M04 si prevedono più byte di risposta rispetto all'ADS131M02)
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
#ifndef IS_M02
    // Se il dispositivo è ADS131M04 (4 canali), aggiunge ulteriori byte dummy 
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
#endif
    // Altri byte dummy finali per completare il pacchetto (includendo eventualmente CRC o dati non utilizzati)
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
#ifndef IS_M02
    // Ulteriori byte dummy (solo per versione a 4 canali) 
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
#endif

    memset(&t, 0, sizeof(t));                     // Inizializza la struttura di transazione SPI azzerandola
    t.length = i * 8;                             // Imposta la lunghezza in bit (numero di byte * 8)
    // t.flags = SPI_TRANS_USE_RXDATA;            // (Opzionale: flag per usare buffer interno RXDATA)
    t.tx_buffer = &ads1310mTxBuffer[0];           // Puntatore al buffer di trasmissione
    t.rx_buffer = &ads1310mRxBuffer[0];           // Puntatore al buffer di ricezione

    ret = spi_device_polling_transmit(ADS1310Mspi, &t);  // Esegue la trasmissione SPI (modalità polling)
    if (ret == ESP_OK)
    {
#ifdef IS_M02
        // Per ADS131M02 (2 canali): legge la risposta dai byte ricevuti corrispondenti 
        wDat = BUILD_UINT16(ads1310mRxBuffer[13], ads1310mRxBuffer[12]);
#else
        // Per ADS131M04 (4 canali): legge la risposta dai byte ricevuti corrispondenti
        wDat = BUILD_UINT16(ads1310mRxBuffer[19], ads1310mRxBuffer[18]);
#endif
        // Estrae l'indirizzo di registro confermato dall'ADC (dai bit del comando di risposta)
        addressRcv = (wDat & REGMASK_CMD_READ_REG_ADDRESS) >> 7;
        if (addressRcv == address)
            return ESP_OK;   // L'indirizzo corrisponde: scrittura riuscita
        else
            return ESP_FAIL; // Indirizzo non corrisponde: la scrittura non è stata confermata
    }
    else
    {
        return ESP_FAIL;     // Errore nella transazione SPI
    }
}

/**
 * @brief Legge il contenuto di un registro dell'ADS131M0x tramite SPI.
 * 
 * La funzione invia un comando di lettura per l'indirizzo specificato e legge il valore a 16 bit del registro dall'ADC.
 * 
 * @param address Indirizzo del registro da leggere.
 * @param data Puntatore a variabile dove verrà scritto il valore letto (16 bit).
 * @return esp_err_t Ritorna ESP_OK se la lettura ha avuto successo, altrimenti ESP_FAIL.
 */
esp_err_t ADS131M0xreadRegister(uint8_t address, uint16_t *data)
{
    esp_err_t ret;
    spi_transaction_t t;    
    uint8_t i = 0;
    uint16_t cmd = 0;

    // Costruisce il comando a 16 bit per la lettura di un registro (codice comando + indirizzo)
    cmd = CMD_READ_REG | (address << 7) | 0;
    // Divide il comando in due byte e aggiunge un byte 0x00 di padding (frame di 24 bit)
    ads1310mTxBuffer[i++] = HI_UINT16(cmd);
    ads1310mTxBuffer[i++] = LO_UINT16(cmd);
    ads1310mTxBuffer[i++] = 0x00;
    // Aggiunge byte dummy 0x00 per generare clock ed effettuare la lettura (spazio per il valore da leggere)
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
#ifndef IS_M02
    // Se ADC a 4 canali, aggiunge ulteriori byte dummy per i dati extra nel frame di risposta
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
#endif
    // Aggiunge altri byte dummy finali (es. CRC o padding)
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
#ifndef IS_M02
    // Ulteriori byte dummy finali (per frame ADS131M04 più lungo)
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
#endif

    memset(&t, 0, sizeof(t));                     // Reset della struttura di transazione SPI
    t.length = i * 8;                             // Lunghezza in bit della transazione (i byte * 8)
    // t.flags = SPI_TRANS_USE_RXDATA;            // (non usiamo buffer interno RXDATA)
    t.tx_buffer = &ads1310mTxBuffer[0];           // Buffer con comando di lettura (trasmissione)
    t.rx_buffer = &ads1310mRxBuffer[0];           // Buffer per ricevere i dati dal dispositivo

    ret = spi_device_polling_transmit(ADS1310Mspi, &t);  // Esegue la transazione SPI di lettura
    if (ret == ESP_OK)
    {
#ifdef IS_M02
        // Per ADS131M02 (2 canali): combina i byte ricevuti appropriati per ottenere il valore richiesto
        *data = BUILD_UINT16(ads1310mRxBuffer[13], ads1310mRxBuffer[12]);
#else
        // Per ADS131M04 (4 canali): estrae il valore dai byte corrispondenti nel buffer di ricezione
        *data = BUILD_UINT16(ads1310mRxBuffer[19], ads1310mRxBuffer[18]);
#endif
        return ESP_OK;
    }
    else
    {
        return ESP_FAIL;  // Errore durante la comunicazione SPI
    }
}

/**
 * @brief Scrive un valore in un registro dell'ADC modificando solo i bit specificati.
 * 
 * La funzione legge prima il contenuto attuale del registro, poi sostituisce soltanto i bit indicati dalla maschera fornita.
 * 
 * @param address Indirizzo del registro da modificare.
 * @param value   Valore da scrivere (già allineato alla posizione dei bit desiderati).
 * @param mask    Maschera dei bit da modificare (bit a 1 indicano i bit su cui intervenire).
 * @return esp_err_t Ritorna ESP_OK se l'operazione ha avuto successo, altrimenti un codice di errore.
 */
esp_err_t ADS131M0xwriteRegisterMasked(uint8_t address, uint16_t value, uint16_t mask)
{
    esp_err_t ret;
    uint16_t register_contents;
    
    // Legge il contenuto corrente del registro target
    ret = ADS131M0xreadRegister(address, &register_contents);
    if (ret != ESP_OK)
        return ret;   // Se la lettura fallisce, esce restituendo l'errore
    
    // Applica la maschera: azzera i bit da modificare nel valore corrente
    // (~mask ha bit a 1 per i bit da lasciare invariati e 0 per quelli da cambiare)
    register_contents = register_contents & ~mask;
    // Imposta i nuovi bit desiderati effettuando un OR con il valore fornito (già posizionato correttamente)
    register_contents = register_contents | value;
    
    // Scrive il nuovo valore calcolato nel registro
    ret = ADS131M0xwriteRegister(address, register_contents);
    return ret;
}

/**
 * @brief Inizializzazione di base dell'ADS131M0x e dell'interfaccia SPI.
 * 
 * Configura i pin di controllo (CS, DRDY, RESET), inizializza il bus SPI e l'aggiunge il dispositivo ADC,
 * esegue un reset hardware e imposta alcuni registri iniziali e parametri (canali abilitati, OSR, ecc.).
 * 
 * @param cs_pin    GPIO del chip select (CS) dell'ADC.
 * @param drdy_pin  GPIO del segnale Data Ready (DRDY) dell'ADC.
 * @param reset_pin GPIO del segnale di reset hardware dell'ADC.
 * @return esp_err_t Ritorna ESP_OK se l'inizializzazione è avvenuta con successo, altrimenti un errore (ESP_FAIL o codice ESP-IDF).
 */
esp_err_t ADS131M0xinit(uint8_t cs_pin, uint8_t drdy_pin, uint8_t reset_pin)
{
    esp_err_t ret;
    bool status;
    
    // Memorizza e configura i pin hardware per CS, DRDY e RESET
    ads1310m_csPin = cs_pin;
    ads1310m_drdyPin = drdy_pin;
    ads1310m_resetPin = reset_pin;
    gpio_reset_pin(ads1310m_csPin);
    gpio_set_direction(ads1310m_csPin, GPIO_MODE_OUTPUT);
    gpio_set_level(ads1310m_csPin, 1);   // Imposta CS alto (non selezionato, idle)
    gpio_reset_pin(ads1310m_resetPin);
    gpio_set_direction(ads1310m_resetPin, GPIO_MODE_OUTPUT);
    gpio_set_level(ads1310m_resetPin, 1); // Imposta RESET alto (dispositivo non in reset)
    gpio_reset_pin(ads1310m_drdyPin);
    gpio_set_direction(ads1310m_drdyPin, GPIO_MODE_INPUT);
    
    // Configurazione del bus SPI
    spi_bus_config_t buscfg = {
        .miso_io_num = MISO0_GPIO,
        .mosi_io_num = MOSI0_GPIO,
        .sclk_io_num = SCK0_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 128    // dimensione massima trasferimento in byte
    };
    
    // Inizializza il bus SPI con la configurazione specificata e utilizzo di DMA (canale SPI_DMA_CH2)
    ret = spi_bus_initialize(ADS131M0x_HOST, &buscfg, SPI_DMA_CH2);
    if (ret != ESP_OK)
        return ret;
    
#ifdef ADS131M0x_EVB
    // *** Configurazione speciale per scheda di valutazione (EVB) ***
    // Aggancia temporaneamente il dispositivo ADC al bus SPI con parametri specifici per EVB
    spi_device_interface_config_t devcfg_tolgiere = {
        .clock_speed_hz = 10 * 1000 * 1000,   // Clock SPI a 10 MHz
        .mode = 0,                            // Modalità SPI 0 (CPOL=0, CPHA=0)
        .spics_io_num = LED2_GPIO,            // Pin CS utilizzato sulla scheda EVB (ad esempio LED2)
        .queue_size = 7                       // Queue per 7 transazioni
    };
    ret = spi_bus_add_device(ADS131M0x_HOST, &devcfg_tolgiere, &ADS1310Mspi);
    if (ret != ESP_OK)
        return ret;
    // Invia una transazione SPI specifica (ad esempio un comando di reset/sblocco per l'ADC sulla EVB)
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));                 // Inizializza la struttura transazione a zero
    ads1310mTxBuffer[0] = 0xCF;
    ads1310mTxBuffer[1] = 0x62;
    t.length = 2 * 8;                         // 2 byte (16 bit) da trasmettere
    // t.flags = SPI_TRANS_USE_TXDATA;        // (potrebbe usare buffer interno TXDATA, ma qui usiamo buffer esterno)
    t.tx_buffer = &ads1310mTxBuffer[0];
    t.rx_buffer = &ads1310mRxBuffer[0];
    ret = spi_device_polling_transmit(ADS1310Mspi, &t);  // Trasmette i 2 byte sul bus SPI
    // Rimuove il dispositivo temporaneo dal bus SPI (fine configurazione EVB)
    spi_bus_remove_device(ADS1310Mspi);
#endif
    
    // Aggancia il dispositivo ADC esterno al bus SPI con la configurazione standard
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,   // Clock SPI a 10 MHz
        .mode = 1,                            // Modalità SPI 1 (CPOL=0, CPHA=1) compatibile con ADS131M0x
        .spics_io_num = ads1310m_csPin,       // Pin CS effettivo dell'ADC
        .cs_ena_pretrans = 1,                // CS attivo leggermente prima della trasmissione
        .cs_ena_posttrans = 1,               // CS tenuto attivo anche subito dopo la trasmissione
        .queue_size = 7                      // Fino a 7 transazioni in coda
    };
    ret = spi_bus_add_device(ADS131M0x_HOST, &devcfg, &ADS1310Mspi);
    if (ret != ESP_OK)
        return ret;
    
    // Esegue un reset hardware dell'ADC (toggle del pin reset)
    ADS131M0xreset();
    
    // Configurazione di default del registro MODE: ad esempio attiva impulso basso su DRDY
    ret = ADS131M0xwriteRegister(REG_MODE, 0x0511);
    if (ret != ESP_OK)
        return ret;
    // Configurazione di default del registro CLOCK: attiva canali 0 e 1
    ret = ADS131M0xwriteRegister(REG_CLOCK, 0x030F);
    if (ret != ESP_OK)
        return ret;
    
    // Abilita il canale 0 in modalità ingresso analogico normale
    status = ADS131M0xsetInputChannelSelection(0, INPUT_CHANNEL_MUX_AIN0P_AIN0N);
    if (status != true)
        return ESP_FAIL;
    // Configura il canale 1 in modalità cortocircuitata (ingressi collegati per zero)
    status = ADS131M0xsetInputChannelSelection(1, INPUT_CHANNEL_MUX_AIN0P_AIN0N);
    if (status != true)
        return ESP_FAIL;
    
    // Imposta l'oversampling ratio (OSR) a 2 (es. corrisponde a 8 kHz di output data rate)
    status = ADS131M0xsetOsr(2);
    if (status != true)
        return ESP_FAIL;
    
    return ESP_OK;
}

/**
 * @brief Esegue un reset hardware dell'ADS131M0x tramite il pin dedicato.
 * 
 * Imposta il pin di reset basso per un breve intervallo e poi lo rilascia.
 * Utilizza la variabile globale ads1310m_resetPin configurata in ADS131M0xinit().
 */
void ADS131M0xreset(void)
{
    gpio_set_level(ads1310m_resetPin, 1);   // Assicura il reset disattivato (livello alto)
    vTaskDelay(100 / portTICK_PERIOD_MS);   // Attende 100 ms
    gpio_set_level(ads1310m_resetPin, 0);   // Attiva il reset (porta il pin a livello basso)
    vTaskDelay(100 / portTICK_PERIOD_MS);   // Attende 100 ms (durata del reset)
    gpio_set_level(ads1310m_resetPin, 1);   // Disattiva nuovamente il reset (livello alto)
}

/**
 * @brief Imposta il formato del segnale DRDY (Data Ready) dell'ADC.
 * 
 * Permette di scegliere tra due formati (consultare datasheet ADS131M0x):
 * ad esempio impulso basso oppure livello costante.
 * 
 * @param drdyFormat Formato DRDY (valido 0 o 1).
 * @return true se il formato è stato impostato correttamente, false per parametri non validi o errore di scrittura.
 */
bool ADS131M0xsetDrdyFormat(uint8_t drdyFormat)
{
    esp_err_t ret;
    if (drdyFormat > 1)
    {
        return false;   // Formato non valido (deve essere 0 o 1)
    }
    else
    {
        // Scrive il bit di formato DRDY nel registro MODE utilizzando la maschera dedicata
        ret = ADS131M0xwriteRegisterMasked(REG_MODE, drdyFormat, REGMASK_MODE_DRDY_FMT);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
}

/**
 * @brief Imposta lo stato del segnale DRDY quando i dati non sono disponibili.
 * 
 * Configura se il pin DRDY deve andare in alta impedenza (HiZ) o mantenere un livello logico quando non vi sono nuovi dati.
 * 
 * @param drdyState Stato desiderato (0 per HiZ attivo, 1 per mantenere livello).
 * @return true se impostato correttamente, false se parametro non valido o errore di scrittura.
 */
bool ADS131M0xsetDrdyStateWhenUnavailable(uint8_t drdyState)
{
    esp_err_t ret;
    if (drdyState > 1)
    {
        return false;  // Valore non valido, può essere solo 0 o 1
    }
    else
    {
        // Scrive il bit DRDY_HiZ nel registro MODE (valore invertito: 1 se drdyState=0, 0 se drdyState=1)
        ret = ADS131M0xwriteRegisterMasked(REG_MODE, drdyState < 1, REGMASK_MODE_DRDY_HiZ);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
}

/**
 * @brief Imposta la modalità di alimentazione (power mode) dell'ADC.
 * 
 * @param powerMode Codice della modalità di potenza (0-3 valore valido).
 * @return true se la modalità è stata impostata correttamente, false se parametro fuori range o errore.
 */
bool ADS131M0xsetPowerMode(uint8_t powerMode)
{
    esp_err_t ret;
    if (powerMode > 3)
    {
        return false;  // Modalità non valida (accetta solo 0-3)
    }
    else
    {
        ret = ADS131M0xwriteRegisterMasked(REG_CLOCK, powerMode, REGMASK_CLOCK_PWR);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
}

/**
 * @brief Imposta l'Oversampling Ratio (OSR) del filtro digitale dell'ADC.
 * 
 * @param osr Valore OSR da impostare (0-7).
 * @return true se l'OSR è stato impostato correttamente, false se valore non valido o errore.
 */
bool ADS131M0xsetOsr(uint16_t osr)
{
    esp_err_t ret;
    if (osr > 7)
    {
        return false;  // OSR fuori range
    }
    else
    {
        // I bit OSR nel registro vanno posizionati a partire dal bit 2, quindi si esegue uno shift a sinistra di 2
        ret = ADS131M0xwriteRegisterMasked(REG_CLOCK, osr << 2, REGMASK_CLOCK_OSR);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
}

/**
 * @brief Abilita o disabilita un canale dell'ADC.
 * 
 * @param channel Indice del canale (0-3, dove 2-3 sono validi solo per ADS131M04).
 * @param enable  1 per abilitare il canale, 0 per disabilitarlo.
 * @return true se l'operazione ha avuto successo, false se parametri non validi o errore di scrittura.
 */
bool ADS131M0xsetChannelEnable(uint8_t channel, uint16_t enable)
{
    esp_err_t ret;
    if (channel > 3)
    {
        return false;  // Canale fuori range
    }
    if (channel == 0)
    {
        // Imposta il bit di abilitazione per il canale 0 (shift a posizione 8) nel registro CLOCK
        ret = ADS131M0xwriteRegisterMasked(REG_CLOCK, enable << 8, REGMASK_CLOCK_CH0_EN);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
    else if (channel == 1)
    {
        // Canale 1: bit a posizione 9
        ret = ADS131M0xwriteRegisterMasked(REG_CLOCK, enable << 9, REGMASK_CLOCK_CH1_EN);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
#ifndef IS_M02
    else if (channel == 2)
    {
        // Canale 2: bit a posizione 10 (solo ADS131M04)
        ret = ADS131M0xwriteRegisterMasked(REG_CLOCK, enable << 10, REGMASK_CLOCK_CH2_EN);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
    else if (channel == 3)
    {
        // Canale 3: bit a posizione 11 (solo ADS131M04)
        ret = ADS131M0xwriteRegisterMasked(REG_CLOCK, enable << 11, REGMASK_CLOCK_CH3_EN);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
#endif
    return false;  // Se nessuna condizione ha fatto return, l'input era fuori range o errore generico
}

/**
 * @brief Imposta il guadagno PGA per il canale specificato.
 * 
 * @param channel Indice del canale (0-3).
 * @param pga     Valore di gain (codice PGA) da impostare.
 * @return true se il guadagno è stato impostato, false se parametro non valido o errore.
 */
bool ADS131M0xsetChannelPGA(uint8_t channel, uint16_t pga)
{
    esp_err_t ret;
    if (channel == 0)
    {
        // Canale 0: i bit di gain sono nei bit meno significativi del registro GAIN (nessuno shift necessario)
        ret = ADS131M0xwriteRegisterMasked(REG_GAIN, pga, REGMASK_GAIN_PGAGAIN0);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
    else if (channel == 1)
    {
        // Canale 1: i bit di gain iniziano dal bit 4 (shift a sinistra di 4)
        ret = ADS131M0xwriteRegisterMasked(REG_GAIN, pga << 4, REGMASK_GAIN_PGAGAIN1);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
#ifndef IS_M02
    else if (channel == 2)
    {
        // Canale 2: shift di 8
        ret = ADS131M0xwriteRegisterMasked(REG_GAIN, pga << 8, REGMASK_GAIN_PGAGAIN2);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
    else if (channel == 3)
    {
        // Canale 3: shift di 12
        ret = ADS131M0xwriteRegisterMasked(REG_GAIN, pga << 12, REGMASK_GAIN_PGAGAIN3);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
#endif
    return false;
}

/**
 * @brief Abilita o disabilita la funzione Global Chop.
 * 
 * @param global_chop Valore 1 per abilitare, 0 per disabilitare il Global Chop.
 * @return esp_err_t Ritorna ESP_OK se il valore è stato scritto, altrimenti codice di errore.
 */
esp_err_t ADS131M0xsetGlobalChop(uint16_t global_chop)
{
    // Scrive il bit Global Chop Enable nel registro CFG (spostato al bit 8)
    return ADS131M0xwriteRegisterMasked(REG_CFG, global_chop << 8, REGMASK_CFG_GC_EN);
}

/**
 * @brief Imposta il ritardo del Global Chop.
 * 
 * @param delay Valore di ritardo (unità secondo datasheet, ad es. ms o us - da verificare).
 * @return esp_err_t Ritorna ESP_OK se scritto correttamente, altrimenti codice di errore.
 */
esp_err_t ADS131M0xsetGlobalChopDelay(uint16_t delay)
{
    // Scrive il valore di ritardo (shift al bit 9) nel registro CFG con la maschera appropriata
    return ADS131M0xwriteRegisterMasked(REG_CFG, delay << 9, REGMASK_CFG_GC_DLY);
}

/**
 * @brief Seleziona la sorgente di ingresso (multiplexer) per un canale dell'ADC.
 * 
 * Permette di scegliere quale ingresso fisico o configurazione interna viene campionata su ciascun canale.
 * 
 * @param channel Canale da configurare (0-3).
 * @param input   Codice dell'ingresso da selezionare (definito nelle costanti INPUT_CHANNEL_MUX_*).
 * @return true se impostato correttamente, false se canale non valido o errore.
 */
bool ADS131M0xsetInputChannelSelection(uint8_t channel, uint8_t input)
{
    esp_err_t ret;
    if (channel == 0)
    {
        ret = ADS131M0xwriteRegisterMasked(REG_CH0_CFG, input, REGMASK_CHX_CFG_MUX);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
    else if (channel == 1)
    {
        ret = ADS131M0xwriteRegisterMasked(REG_CH1_CFG, input, REGMASK_CHX_CFG_MUX);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
#ifndef IS_M02
    else if (channel == 2)
    {
        ret = ADS131M0xwriteRegisterMasked(REG_CH2_CFG, input, REGMASK_CHX_CFG_MUX);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
    else if (channel == 3)
    {
        ret = ADS131M0xwriteRegisterMasked(REG_CH3_CFG, input, REGMASK_CHX_CFG_MUX);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
#endif
    return false;
}

/**
 * @brief Imposta il valore di calibrazione dell'offset per un canale.
 * 
 * L'offset di calibrazione è un valore a 24 bit diviso tra due registri (parte MSB e LSB).
 * 
 * @param channel Canale da calibrare (0-3).
 * @param offset  Valore di offset (24 bit significativi, passato come int32).
 * @return true se la calibrazione è stata impostata, false in caso di errore.
 */
bool ADS131M0xsetChannelOffsetCalibration(uint8_t channel, int32_t offset)
{
    esp_err_t ret;
    uint16_t MSB = offset >> 8;      // Estrae i 16 bit più significativi dell'offset
    uint8_t LSB = offset & 0xFF;    // Estrae gli 8 bit meno significativi
    
    if (channel == 0)
    {
        // Scrive i 16 bit MSB nel registro CH0_OCAL_MSB (tutti i bit del registro, maschera 0xFFFF)
        ret = ADS131M0xwriteRegisterMasked(REG_CH0_OCAL_MSB, MSB, 0xFFFF);
        // Scrive gli 8 bit LSB (posizionati negli 8 bit più alti del registro LSB) nel registro CH0_OCAL_LSB
        ret |= ADS131M0xwriteRegisterMasked(REG_CH0_OCAL_LSB, LSB << 8, REGMASK_CHX_OCAL0_LSB);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
    else if (channel == 1)
    {
        ret = ADS131M0xwriteRegisterMasked(REG_CH1_OCAL_MSB, MSB, 0xFFFF);
        ret |= ADS131M0xwriteRegisterMasked(REG_CH1_OCAL_LSB, LSB << 8, REGMASK_CHX_OCAL0_LSB);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
#ifndef IS_M02
    else if (channel == 2)
    {
        ret = ADS131M0xwriteRegisterMasked(REG_CH2_OCAL_MSB, MSB, 0xFFFF);
        ret |= ADS131M0xwriteRegisterMasked(REG_CH2_OCAL_LSB, LSB << 8, REGMASK_CHX_OCAL0_LSB);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
    else if (channel == 3)
    {
        ret = ADS131M0xwriteRegisterMasked(REG_CH3_OCAL_MSB, MSB, 0xFFFF);
        ret |= ADS131M0xwriteRegisterMasked(REG_CH3_OCAL_LSB, LSB << 8, REGMASK_CHX_OCAL0_LSB);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
#endif
    return false;
}

/**
 * @brief Imposta il valore di calibrazione del guadagno per un canale.
 * 
 * Il valore di calibrazione del gain è un numero a 24 bit suddiviso in due registri (MSB e LSB).
 * 
 * @param channel Canale di cui impostare la calibrazione (0-3).
 * @param gain    Valore di calibrazione a 24 bit (formato uint32_t).
 * @return true se l'operazione riesce, false in caso di errore.
 */
bool ADS131M0xsetChannelGainCalibration(uint8_t channel, uint32_t gain)
{
    esp_err_t ret;
    uint16_t MSB = gain >> 8;
    uint8_t LSB = gain & 0xFF;
    
    if (channel == 0)
    {
        ret = ADS131M0xwriteRegisterMasked(REG_CH0_GCAL_MSB, MSB, 0xFFFF);
        ret |= ADS131M0xwriteRegisterMasked(REG_CH0_GCAL_LSB, LSB << 8, REGMASK_CHX_GCAL0_LSB);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
    else if (channel == 1)
    {
        ret = ADS131M0xwriteRegisterMasked(REG_CH1_GCAL_MSB, MSB, 0xFFFF);
        ret |= ADS131M0xwriteRegisterMasked(REG_CH1_GCAL_LSB, LSB << 8, REGMASK_CHX_GCAL0_LSB);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
#ifndef IS_M02
    else if (channel == 2)
    {
        ret = ADS131M0xwriteRegisterMasked(REG_CH2_GCAL_MSB, MSB, 0xFFFF);
        ret |= ADS131M0xwriteRegisterMasked(REG_CH2_GCAL_LSB, LSB << 8, REGMASK_CHX_GCAL0_LSB);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
    else if (channel == 3)
    {
        ret = ADS131M0xwriteRegisterMasked(REG_CH3_GCAL_MSB, MSB, 0xFFFF);
        ret |= ADS131M0xwriteRegisterMasked(REG_CH3_GCAL_LSB, LSB << 8, REGMASK_CHX_GCAL0_LSB);
        if (ret == ESP_OK)
            return true;
        else
            return false;
    }
#endif
    return false;
}

/**
 * @brief Verifica se sono pronti nuovi dati dall'ADC (controllando il pin DRDY).
 * 
 * @return true se il pin DRDY indica dati pronti, false altrimenti.
 * 
 * @note Il segnale DRDY dell'ADS131M0x è attivo basso: questa funzione restituisce true quando il livello logico del pin è basso (0).
 */
bool ADS131M0xisDataReady()
{
    if (gpio_get_level(ads1310m_drdyPin))
    {
        return false;   // DRDY è alto: dati non pronti
    }
    return true;        // DRDY è basso: nuovi dati disponibili
}

/**
 * @brief Legge i valori ADC correnti di tutti i canali e lo status.
 * 
 * Questa funzione effettua una lettura completa dell'ADC, ottenendo lo status e i dati convertiti di ciascun canale.
 * I dati vengono convertiti in formato int32 (con segno) nella struttura fornita in ingresso.
 * 
 * @param data Puntatore alla struttura ads1310m0x_adc_t in cui verranno salvati lo status e i valori dei canali.
 * @return esp_err_t ESP_OK se la lettura è riuscita, ESP_FAIL in caso di errore di comunicazione.
 */
esp_err_t ADS131M0xreadADC(ads1310m0x_adc_t *data)
{
    esp_err_t ret;
    spi_transaction_t t;
    uint8_t i = 0;
    uint8_t x = 0;
    uint8_t x2 = 0;
    uint8_t x3 = 0;
    int32_t aux;
    
    // Prepara i byte dummy per la lettura: lo STATUS (3 byte)...
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    // ...i dati del canale 0 (3 byte)...
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    // ...i dati del canale 1 (3 byte)...
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
#ifndef IS_M02
    // ...i dati del canale 2 (3 byte, solo se ADC a 4 canali)...
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    // ...i dati del canale 3 (3 byte, solo se ADC a 4 canali)...
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
#endif
    // ...e infine i byte dummy per il CRC (3 byte, se abilitato, altrimenti padding)
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    ads1310mTxBuffer[i++] = 0x00;
    
    memset(&t, 0, sizeof(t));                 // Inizializza la struttura di transazione SPI
    t.length = i * 8;                         // Lunghezza in bit (i byte * 8)
    // t.flags = SPI_TRANS_USE_RXDATA;
    t.tx_buffer = &ads1310mTxBuffer[0];       // Buffer di trasmissione (dummy data)
    t.rx_buffer = &ads1310mRxBuffer[0];       // Buffer di ricezione (conterrà status + dati)
    
    ret = spi_device_polling_transmit(ADS1310Mspi, &t);  // Esegue la transazione SPI di lettura dati
    if (ret == ESP_OK)
    {
        // Estrae lo status a 16 bit (dai primi 2 byte ricevuti)
        data->status = BUILD_UINT16(ads1310mRxBuffer[1], ads1310mRxBuffer[0]);
        
        // Estrae i 3 byte del Canale 0 e li combina in un valore a 24 bit
        x  = ads1310mRxBuffer[3];
        x2 = ads1310mRxBuffer[4];
        x3 = ads1310mRxBuffer[5];
        aux = ((x << 16) | (x2 << 8) | x3) & 0x00FFFFFF;
        // Verifica il bit di segno del valore a 24 bit: se è negativo (bit 23 = 1), converte in complemento a due negativo
        if (aux > 0x7FFFFF)
            data->ch0 = ((~aux & 0x00FFFFFF) + 1) * -1;
        else
            data->ch0 = aux;
        
        // Elabora allo stesso modo i 3 byte del Canale 1
        x  = ads1310mRxBuffer[6];
        x2 = ads1310mRxBuffer[7];
        x3 = ads1310mRxBuffer[8];
        aux = ((x << 16) | (x2 << 8) | x3) & 0x00FFFFFF;
        if (aux > 0x7FFFFF)
            data->ch1 = ((~aux & 0x00FFFFFF) + 1) * -1;
        else
            data->ch1 = aux;
        
#ifndef IS_M02
        // Canale 2 (solo per ADS131M04), combinazione e conversione come sopra
        x  = ads1310mRxBuffer[9];
        x2 = ads1310mRxBuffer[10];
        x3 = ads1310mRxBuffer[11];
        aux = ((x << 16) | (x2 << 8) | x3) & 0x00FFFFFF;
        if (aux > 0x7FFFFF)
            data->ch2 = ((~aux & 0x00FFFFFF) + 1) * -1;
        else
            data->ch2 = aux;
        
        // Canale 3 (solo per ADS131M04)
        x  = ads1310mRxBuffer[12];
        x2 = ads1310mRxBuffer[13];
        x3 = ads1310mRxBuffer[14];
        aux = ((x << 16) | (x2 << 8) | x3) & 0x00FFFFFF;
        if (aux > 0x7FFFFF)
            data->ch3 = ((~aux & 0x00FFFFFF) + 1) * -1;
        else
            data->ch3 = aux;
#endif
        return ESP_OK;
    }
    else
    {
        return ESP_FAIL;
    }
}
