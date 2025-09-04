/*
 *      Progetto: 82037601
 *      nome    : test.h
 *      descr   : Header con dichiarazioni di funzioni di test per i moduli hardware e software
 *
 *      Questo file raccoglie i prototipi delle funzioni di test utilizzate per verificare 
 *      il corretto funzionamento dei diversi componenti del sistema (Wi-Fi, ADC esterno, 
 *      scheda SD, LED, microfono, ecc.). Ciascuna funzione implementa un test specifico 
 *      per un modulo o una periferica.
 */
#ifndef TEST_H_
#define TEST_H_

#include <stdint.h>

/* Definizione pin ---------------------------------------------------------- */

/* Definizione macro I/O ---------------------------------------------------- */

/* Definizione macro software ----------------------------------------------- */

/* Definizione delle costanti di Debug -------------------------------------- */

/* Definizione delle costanti ----------------------------------------------- */

/* Definizione delle variabili ---------------------------------------------- */

/* Definizione delle variabili esterne -------------------------------------- */

/* Definizione delle Funzioni esterne --------------------------------------- */

/* Test_WIFI: inizializza e verifica la connettività Wi-Fi creando un Access Point 
   e avviando un server TCP per ricevere comandi di test (es. avvio/stop registrazione dati). */
void Test_WIFI(void);

/* Test_ADC: esegue un test sul convertitore analogico-digitale (ADC), inizializzando 
   l'ADC e leggendo valori di prova per verificarne il funzionamento. */
void Test_ADC(void);

/* Test_SDCARD: verifica la disponibilità della scheda SD e del file system, 
   testando operazioni di base come creazione, scrittura e lettura di un file. */
void Test_SDCARD(void);

/* Test_LED: effettua un test dei LED di stato o di segnalazione, ad esempio lampeggiando 
   i LED connessi per verificarne il corretto controllo tramite GPIO. */
void Test_LED(void);

/* Test_ADS131M0X: testa il convertitore ADC esterno ADS131M0x, inizializzandolo 
   e leggendo campioni dai suoi canali per verificare la comunicazione SPI e la correttezza dei dati acquisiti. */
void Test_ADS131M0X(void);

/* Test_MICROPHONE: prova la funzionalità di acquisizione audio da un microfono collegato, 
   ad esempio avviando la raccolta di campioni audio e controllando che vengano registrati correttamente. */
void Test_MICROPHONE(void);

/* Test_MICROPHONE_2: simile a Test_MICROPHONE, esegue un secondo test di acquisizione audio (ad esempio, 
   utilizzando un secondo microfono o una modalità differente) per approfondire la verifica del sistema di registrazione audio. */
void Test_MICROPHONE_2(void);

/* Test_MAC_ADDRESS: legge l'indirizzo MAC dell'ESP32 e lo visualizza, per confermare 
   la disponibilità e correttezza dell'identificativo univoco del dispositivo. */
void Test_MAC_ADDRESS(void);

#endif /* TEST_H_ */
/*EOF*/
