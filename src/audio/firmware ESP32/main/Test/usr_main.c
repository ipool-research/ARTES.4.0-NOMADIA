#include "usr_global.h"

/* Definizione delle variabili esterne -------------------------------------- */
/* Variabili esterne fornite dal sistema BIOS/framework (codice di base) */
extern const char BiosCode[9];
extern const char BiosVer[5];
extern uint16_t tickCadenza;

/* Definizioni delle costanti software -------------------------------------- */
/* Identificativo e versione del software applicativo */
const char SoftCode[5] = { "PINT" };  // Codice software applicativo
const char SoftVer[5]  = { "0.01" };  // Versione del software

/* Definizione delle variabili ---------------------------------------------- */
/* Variabile globale per la data/ora corrente (Real Time Clock) */
struct tm RTCstruct;

/* Funzione utente nel SysTick interrupt
 * Viene invocata ad ogni tick di sistema (tipicamente ogni 1 ms) per eseguire operazioni periodiche a livello utente.
 * In questo caso richiama LEDmanage(10) per gestire il lampeggio di un LED di stato ogni 10 tick (funzione heartbeat di attività).
 */
void USRtickTimer(void) {
    LEDmanage(10);
}

/* Funzione di callback per ricezione dati da UART0
 * Viene chiamata quando viene ricevuto un byte sulla UART0.
 * (In questa applicazione non esegue alcuna azione specifica ed è lasciata vuota)
 */
void USRuart0Rx(uint8_t dat) {
    // Nessuna azione specifica per dati ricevuti da UART0
}

/* Funzione di callback per ricezione dati da UART1
 * (Non utilizzata in questa applicazione)
 */
void USRuart1Rx(uint8_t dat) {
    // Nessuna azione specifica per dati ricevuti da UART1
}

/* Funzione di callback per ricezione dati da UART2
 * (Non utilizzata in questa applicazione)
 */
void USRuart2Rx(uint8_t dat) {
    // Nessuna azione specifica per dati ricevuti da UART2
}

/* Funzione di callback per ricezione dati via WiFi
 * (Non utilizzata: la gestione dei dati WiFi avviene nel task tcp_server_task)
 */
void USRwifiReceive(void) {
    // Nessuna azione specifica per dati WiFi in arrivo
}

/* Funzione principale utente (startup e configurazione)
 * Punto di ingresso dell'applicazione utente, eseguito dopo l'inizializzazione di sistema.
 * Abilita la ricezione su UART0 per eventuali comunicazioni (ad esempio debug o comandi da console seriale), attivando la relativa routine utente (USRuart0Rx).
 * Richiama quindi la funzione Test_WIFI() che si occupa di inizializzare WiFi, ADC e SD card, e di avviare il server TCP per la comunicazione con il PC.
 * Infine, entra in un loop infinito inattivo, poiché tutte le operazioni principali (acquisizione dati ADC, scrittura su SD e comunicazione WiFi) sono gestite da interrupt o task FreeRTOS separati.
 */
void USRmain(void) {
    UART0startRx();         // Abilita la ricezione dati su UART0
    UART0enable_UserRx();   // Attiva la callback utente per i dati ricevuti su UART0 (USRuart0Rx)

    Test_WIFI();            // Inizializza WiFi, ADC, SD card e avvia il task server TCP

    while (1) {
        // Loop infinito in attesa: il programma principale non ha operazioni cicliche da eseguire
        // (le funzionalità sono gestite tramite interrupt e task dedicati creati in precedenza)
    }
}
