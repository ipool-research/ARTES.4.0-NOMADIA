#ifndef ADS131M0x_h
#define ADS131M0x_h

/* Definizione costanti 
----------------------------------------------------------*/
// Configurazione per versione a 2 canali (ADS131M02)
#define IS_M02

// #define NO_CS_DELAY  // (Opzionale) Nessun ritardo dopo CS attivo in lettura ADC

/* Definizione tipi 
--------------------------------------------------------------*/
typedef struct {
    uint16_t status;    // Registro di stato ADC
    int32_t  ch0;       // Valore convertito canale 0
    int32_t  ch1;       // Valore convertito canale 1
#ifndef IS_M02
    int32_t  ch2;       // Valore convertito canale 2 (solo versione 4 canali)
    int32_t  ch3;       // Valore convertito canale 3 (solo versione 4 canali)
#endif
} ads1310m0x_adc_t;    // Struttura dati per campione ADC (status + canali)

/* Definizione costanti 
----------------------------------------------------------*/
#define DRDY_STATE_LOGIC_HIGH   0   // Linea DRDY inattiva a livello logico alto (default)
#define DRDY_STATE_HI_Z         1   // Linea DRDY inattiva in alta impedenza

#define POWER_MODE_VERY_LOW_POWER   0   // Modalità bassissimo consumo (risoluzione ridotta)
#define POWER_MODE_LOW_POWER        1   // Modalità basso consumo (risoluzione inferiore)
#define POWER_MODE_HIGH_RESOLUTION  2   // Modalità alta risoluzione (massima precisione, default)

// Valori per guadagno PGA canali (0 -> 1x, 1 -> 2x, ... 7 -> 128x)
#define CHANNEL_PGA_1   0
#define CHANNEL_PGA_2   1
#define CHANNEL_PGA_4   2
#define CHANNEL_PGA_8   3
#define CHANNEL_PGA_16  4
#define CHANNEL_PGA_32  5
#define CHANNEL_PGA_64  6
#define CHANNEL_PGA_128 7

// Selezione ingresso canale (multiplexer)
#define INPUT_CHANNEL_MUX_AIN0P_AIN0N             0   // Ingresso differenziale AIN0P-AIN0N (default)
#define INPUT_CHANNEL_MUX_INPUT_SHORTED           1   // Ingresso cortocircuitato (riferimento zero)
#define INPUT_CHANNEL_MUX_POSITIVE_DC_TEST_SIGNAL 2   // Segnale di test DC positivo interno
#define INPUT_CHANNEL_MUX_NEGATIVE_DC_TEST_SIGNAL 3   // Segnale di test DC negativo interno

// Rapporto di oversampling (OSR)
#define OSR_128   0   // Oversampling 128x
#define OSR_256   1   // Oversampling 256x
#define OSR_512   2   // Oversampling 512x
#define OSR_1024  3   // Oversampling 1024x (predefinito)
#define OSR_2018  4   // Oversampling 2018x
#define OSR_4096  5   // Oversampling 4096x
#define OSR_8192  6   // Oversampling 8192x
#define OSR_16384 7   // Oversampling 16384x

// Comandi (codici operativi per ADS131M0x)
#define CMD_NULL      0x0000   // Comando NULLO (ritorna il registro STATUS)
#define CMD_RESET     0x0011   // Comando di reset software del dispositivo
#define CMD_STANDBY   0x0022   // Pone il dispositivo in standby (basso consumo)
#define CMD_WAKEUP    0x0033   // Riattiva il dispositivo dallo standby
#define CMD_LOCK      0x0555   // Blocca l'accesso in scrittura ai registri
#define CMD_UNLOCK    0x0655   // Sblocca l'accesso in scrittura ai registri
#define CMD_READ_REG  0xA000   // Comando base per lettura registri (indirizzo e numero codificati)
#define CMD_WRITE_REG 0x6000   // Comando base per scrittura registri (indirizzo e valore codificati)

// Risposte previste
#ifdef IS_M02
#define RSP_RESET_OK  0xFF22   // Risposta OK al comando RESET (ADS131M02)
#else
#define RSP_RESET_OK  0xFF24   // Risposta OK al comando RESET (ADS131M04)
#endif
#define RSP_RESET_NOK 0x0011   // Risposta di errore al comando RESET

// Indirizzi registri (Read Only)
#define REG_ID      0x00   // Registro ID dispositivo
#define REG_STATUS  0x01   // Registro di stato

// Indirizzi registri impostazioni globali (comuni a tutti i canali)
#define REG_MODE    0x02
#define REG_CLOCK   0x03
#define REG_GAIN    0x04
#define REG_CFG     0x06
#define REG_THRSHLD_MSB 0x07
#define REG_THRSHLD_LSB 0x08

// Indirizzi registri specifici canale 0
#define REG_CH0_CFG      0x09
#define REG_CH0_OCAL_MSB 0x0A
#define REG_CH0_OCAL_LSB 0x0B
#define REG_CH0_GCAL_MSB 0x0C
#define REG_CH0_GCAL_LSB 0x0D

// Indirizzi registri specifici canale 1
#define REG_CH1_CFG      0x0E
#define REG_CH1_OCAL_MSB 0x0F
#define REG_CH1_OCAL_LSB 0x10
#define REG_CH1_GCAL_MSB 0x11
#define REG_CH1_GCAL_LSB 0x12

// Indirizzi registri specifici canale 2
#define REG_CH2_CFG      0x13
#define REG_CH2_OCAL_MSB 0x14
#define REG_CH2_OCAL_LSB 0x15
#define REG_CH2_GCAL_MSB 0x16
#define REG_CH2_GCAL_LSB 0x17

// Indirizzi registri specifici canale 3
#define REG_CH3_CFG      0x18
#define REG_CH3_OCAL_MSB 0x19
#define REG_CH3_OCAL_LSB 0x1A
#define REG_CH3_GCAL_MSB 0x1B
#define REG_CH3_GCAL_LSB 0x1C

// Indirizzo registro CRC della mappa di registri
#define REG_MAP_CRC      0x3E

// Maschere per comando READ_REG
#define REGMASK_CMD_READ_REG_ADDRESS  0x1F80   // Estrae l'indirizzo dal comando READ_REG
#define REGMASK_CMD_READ_REG_BYTES    0x007F   // Estrae il numero di registri dal comando READ_REG

// Maschere bit nel registro STATUS
#define REGMASK_STATUS_LOCK     0x8000
#define REGMASK_STATUS_RESYNC   0x4000
#define REGMASK_STATUS_REGMAP   0x2000
#define REGMASK_STATUS_CRC_ERR  0x1000
#define REGMASK_STATUS_CRC_TYPE 0x0800
#ifdef IS_M02
#define REGMASK_STATUS_RESET    0x0200
#else
#define REGMASK_STATUS_RESET    0x0400
#endif
#define REGMASK_STATUS_WLENGTH  0x0300
#define REGMASK_STATUS_DRDY3    0x0008
#define REGMASK_STATUS_DRDY2    0x0004
#define REGMASK_STATUS_DRDY1    0x0002
#define REGMASK_STATUS_DRDY0    0x0001

// Maschere bit nel registro MODE
#define REGMASK_MODE_REG_CRC_EN 0x2000
#define REGMASK_MODE_RX_CRC_EN  0x1000
#define REGMASK_MODE_CRC_TYPE   0x0800
#define REGMASK_MODE_RESET      0x0400
#define REGMASK_MODE_WLENGTH    0x0300
#define REGMASK_MODE_TIMEOUT    0x0010
#define REGMASK_MODE_DRDY_SEL   0x000C
#define REGMASK_MODE_DRDY_HiZ   0x0002
#define REGMASK_MODE_DRDY_FMT   0x0001

// Maschere bit nel registro CLOCK
#define REGMASK_CLOCK_CH3_EN    0x0800
#define REGMASK_CLOCK_CH2_EN    0x0400
#define REGMASK_CLOCK_CH1_EN    0x0200
#define REGMASK_CLOCK_CH0_EN    0x0100
#define REGMASK_CLOCK_OSR       0x001C
#define REGMASK_CLOCK_PWR       0x0003

// Maschere bit nel registro GAIN
#define REGMASK_GAIN_PGAGAIN3   0x7000
#define REGMASK_GAIN_PGAGAIN2   0x0700
#define REGMASK_GAIN_PGAGAIN1   0x0070
#define REGMASK_GAIN_PGAGAIN0   0x0007

// Maschere bit nel registro CFG
#define REGMASK_CFG_GC_DLY      0x1E00
#define REGMASK_CFG_GC_EN       0x0100
#define REGMASK_CFG_CD_ALLCH    0x0080
#define REGMASK_CFG_CD_NUM      0x0070
#define REGMASK_CFG_CD_LEN      0x000E
#define REGMASK_CFG_CD_EN       0x0001

// Maschere bit nel registro THRSHLD_LSB
#define REGMASK_THRSHLD_LSB_CD_TH_LSB 0xFF00
#define REGMASK_THRSHLD_LSB_DCBLOCK   0x000F

// Maschere bit nel registro CHx_CFG
#define REGMASK_CHX_CFG_PHASE       0xFFC0
#define REGMASK_CHX_CFG_DCBLKX_DIS0 0x0004
#define REGMASK_CHX_CFG_MUX         0x0003

// Maschera bit nel registro CHx_OCAL_LSB
#define REGMASK_CHX_OCAL0_LSB       0xFF00

// Maschera bit nel registro CHx_GCAL_LSB
#define REGMASK_CHX_GCAL0_LSB       0xFF00

// Modalità di conversione
#define CONVERSION_MODE_CONT    0   // Conversione continua (streaming dati)
#define CONVERSION_MODE_SINGLE  1   // Conversione singola (one-shot)

// Formato dati in uscita
#define DATA_FORMAT_TWO_COMPLEMENT  0   // Dati in complemento a due (signed)
#define DATA_FORMAT_BINARY          1   // Dati binari (offset, unsigned)

// Modalità di misura (range ingresso)
#define MEASURE_UNIPOLAR  1   // Misura unipolare (0 - full scale)
#define MEASURE_BIPOLAR   0   // Misura bipolare (± full scale)

// Sorgente clock
#define CLOCK_EXTERNAL 1   // Clock esterno (fornito dall'esterno)
#define CLOCK_INTERNAL 0   // Clock interno (oscillatore interno ADC)

// Valori possibili guadagno PGA (identici a CHANNEL_PGA_x)
#define PGA_GAIN_1    0   // 1x
#define PGA_GAIN_2    1   // 2x
#define PGA_GAIN_4    2   // 4x
#define PGA_GAIN_8    3   // 8x
#define PGA_GAIN_16   4   // 16x
#define PGA_GAIN_32   5   // 32x
#define PGA_GAIN_64   6   // 64x
#define PGA_GAIN_128  7   // 128x

// Tipi di filtro di ingresso
#define FILTER_SINC     0   // Filtro sinc (decimatore integratore)
#define FILTER_FIR      2   // Filtro FIR (media mobile)
#define FILTER_FIR_IIR  3   // Combinazione di filtri FIR/IIR

// Modalità parola dati (ampiezza campione ADC)
#define DATA_MODE_24BITS 0   // Dati a 24 bit
#define DATA_MODE_32BITS 1   // Dati a 32 bit

// Codici frequenza di campionamento (Data Rate)
#define DATA_RATE_0   0
#define DATA_RATE_1   1
#define DATA_RATE_2   2
#define DATA_RATE_3   3
#define DATA_RATE_4   4
#define DATA_RATE_5   5
#define DATA_RATE_6   6
#define DATA_RATE_7   7
#define DATA_RATE_8   8
#define DATA_RATE_9   9
#define DATA_RATE_10  10
#define DATA_RATE_11  11
#define DATA_RATE_12  12
#define DATA_RATE_13  13
#define DATA_RATE_14  14
#define DATA_RATE_15  15

// Modalità di sincronizzazione
#define SYNC_CONTINUOUS 1   // Sincronizzazione continua (conversioni continue)
#define SYNC_PULSE      0   // Sincronizzazione a impulsi (trigger esterno per conversione)

// Configurazione I/O digitale (GPIO del chip)
#define DIO_OUTPUT      1   // Imposta il pin DIO come uscita digitale
#define DIO_INPUT       0   // Imposta il pin DIO come ingresso digitale

/* Definizione prototipi 
----------------------------------------------------------*/
esp_err_t ADS131M0xwriteRegister(uint8_t address, uint16_t value);               // Scrive un valore 16-bit nel registro specificato dell'ADS131M0x
esp_err_t ADS131M0xreadRegister(uint8_t address, uint16_t *data);                // Legge un valore 16-bit dal registro specificato (risultato in *data)
esp_err_t ADS131M0xwriteRegisterMasked(uint8_t address, uint16_t value, uint16_t mask); // Scrive solo i bit indicati da mask nel registro (preserva gli altri)

esp_err_t ADS131M0xinit(uint8_t cs_pin, uint8_t drdy_pin, uint8_t reset_pin);     // Inizializza comunicazione con ADS131M0x (configura SPI e pin CS, DRDY, RESET)
void      ADS131M0xreset(void);                                                  // Esegue il reset hardware/software dell'ADS131M0x
bool      ADS131M0xsetDrdyFormat(uint8_t drdyFormat);                            // Configura il formato del segnale DRDY (impulso vs livello continuo)
bool      ADS131M0xsetDrdyStateWhenUnavailable(uint8_t drdyState);               // Configura lo stato della linea DRDY quando dati non pronti (alto logico vs alta impedenza)
bool      ADS131M0xsetPowerMode(uint8_t powerMode);                              // Imposta la modalità di potenza (trade-off consumo vs risoluzione)
bool      ADS131M0xsetOsr(uint16_t osr);                                         // Imposta il rapporto di oversampling (OSR)
bool      ADS131M0xsetChannelEnable(uint8_t channel, uint16_t enable);           // Abilita (1) o disabilita (0) il canale ADC specificato
bool      ADS131M0xsetChannelPGA(uint8_t channel, uint16_t pga);                 // Imposta il guadagno PGA per il canale specificato
esp_err_t ADS131M0xsetGlobalChop(uint16_t global_chop);                          // Abilita o disabilita la funzione Global Chop (stabilizzazione dell'offset)
esp_err_t ADS131M0xsetGlobalChopDelay(uint16_t delay);                           // Imposta il ritardo per la funzione Global Chop (in passi di clock)
bool      ADS131M0xsetInputChannelSelection(uint8_t channel, uint8_t input);     // Seleziona la sorgente di ingresso per il canale (analogico, cortocircuito, test interno, ecc.)
bool      ADS131M0xsetChannelOffsetCalibration(uint8_t channel, int32_t offset); // Imposta il valore di calibrazione offset per il canale specificato
bool      ADS131M0xsetChannelGainCalibration(uint8_t channel, uint32_t gain);    // Imposta il valore di calibrazione guadagno per il canale specificato
bool      ADS131M0xisDataReady(void);                                            // Verifica se è disponibile un nuovo dato ADC (linea DRDY attiva)
esp_err_t ADS131M0xreadADC(ads1310m0x_adc_t *data);                              // Legge i valori ADC correnti di tutti i canali e li salva nella struttura data

#endif /* ADS131M0x_h */
