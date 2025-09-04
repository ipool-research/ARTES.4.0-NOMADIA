#include "usr_global.h"

/* Definizione delle variabili ----------------------------------------- */
/* Flag che indicano se il WiFi e l'ADC sono stati inizializzati almeno una volta */
uint8_t wifiFirstTime = 0;
uint8_t ads131m0xFirstTime = 0;

/* Procedure ----------------------------------------------------------- */
/* Test_WIFI: Inizializza WiFi (AP), ADC e SD card, poi avvia il task server TCP
 * Questa funzione viene chiamata all'avvio dell'applicazione utente per configurare i moduli principali:
 * - Se il WiFi non è ancora stato inizializzato (wifiFirstTime == 0), configura l'ESP32 come Access Point WiFi con SSID "PINT" (rete aperta senza password). Imposta wifiFirstTime = 1 dopo la prima inizializzazione.
 * - Se l'ADC ADS131M0x non è ancora inizializzato (ads131m0xFirstTime == 0), inizializza il convertitore ADC specificando i pin CS (CSADC_GPIO), DRDY (DRDY_GPIO) e SYNC (SYNC_GPIO). Imposta ads131m0xFirstTime = 1 dopo l'avvio riuscito dell'ADC.
 * - Inizializza la scheda SD (monta il filesystem) chiamando SDCARDinit().
 * Se tutte le inizializzazioni hanno successo, crea il task FreeRTOS tcp_server_task che gestirà la connessione TCP con il PC.
 * In caso di errore in una delle fasi di inizializzazione, la funzione salta alla fine (label 'uscita') senza avviare il server.
 */
void Test_WIFI(void) {
    if (wifiFirstTime == 0) {
        // WiFi non ancora inizializzato: avvia l'Access Point WiFi con SSID "PINT"
        if (WIFIinitAP("PINT", "") == ESP_OK) {
            // Access Point avviato con successo (SSID "PINT", nessuna password)
        } else {
            goto uscita;  // Errore durante l'inizializzazione WiFi, esce dalla funzione
        }
        wifiFirstTime = 1;  // Segnala che il WiFi è stato inizializzato (non ripeterà l'init in futuro)
    } else {
        // WiFi era già inizializzato in precedenza, nessuna azione necessaria
    }

    if (ads131m0xFirstTime == 0) {
        // ADC non ancora inizializzato: inizializza ADS131M0x specificando pin CS, DRDY e SYNC
        if (ADS131M0xinit(CSADC_GPIO, DRDY_GPIO, SYNC_GPIO) == ESP_OK) {
            ads131m0xFirstTime = 1;  // ADC inizializzato correttamente (marcato come fatto)
        } else {
            goto uscita;  // Errore durante l'inizializzazione dell'ADC, esce dalla funzione
        }
    }

    if (SDCARDinit() == ESP_OK) {
        // Scheda SD inizializzata correttamente (file system montato)
    } else {
        goto uscita;  // Errore durante l'inizializzazione della SD, esce dalla funzione
    }

    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);  // Crea e avvia il task server TCP (priorità 5, stack 4096 byte)

uscita:  // Etichetta di uscita in caso di errore di inizializzazione
    return;
}
