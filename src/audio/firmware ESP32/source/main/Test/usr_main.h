/*
 *      Contiene definizioni e dichiarazioni correlate al codice principale dell'applicazione utente. In particolare,
 *      definisce costanti globali specifiche dell'applicazione e i prototipi delle funzioni di gestione 
 *      (timer di sistema, ricezione UART, eventi Wi-Fi) nonché la funzione di avvio dell'applicazione stessa.
 */
#ifndef USR_MAIN_H_
#define USR_MAIN_H_

#include <stdint.h>
#include "esp_http_client.h"

/* Definizione pin ---------------------------------------------------------- */

/* Definizione macro I/O ---------------------------------------------------- */

/* Definizione macro software ----------------------------------------------- */

/* Definizione delle costanti di Debug -------------------------------------- */

/* Definizione delle costanti ----------------------------------------------- */

// Mappatura indirizzi flash interna
#define FLASH_INT_START 0   // Indirizzo iniziale (offset 0) della memoria flash interna utilizzato per il mapping

/* Definizione delle variabili ---------------------------------------------- */

/* Definizioni di tipo per salvataggio Parametri sdu E2DATA ----------------- */

/* Definizioni di tipo ------------------------------------------------------ */

/* Definizione dei prototipi ------------------------------------------------ */

/* USRtickTimer: funzione eseguita ad ogni tick di sistema (SysTick).
   Viene chiamata periodicamente dall'interrupt di SysTick per eseguire operazioni a cadenza fissa.
   (Ad esempio, può gestire il lampeggio di un LED di segnalazione o aggiornare conteggi temporali.) */
void USRtickTimer(void);

/* USRuart0Rx: gestore di ricezione dati per la porta UART0.
   Viene invocata ogni volta che si riceve un byte sulla UART0, permettendo di definire 
   la logica di elaborazione dei dati in ingresso su questa porta seriale.
   inp: dat - il byte ricevuto dalla UART0. */
void USRuart0Rx(uint8_t dat);

/* USRuart1Rx: gestore di ricezione dati per la porta UART1 (analogo a USRuart0Rx).
   inp: dat - il byte ricevuto sulla UART1. */
void USRuart1Rx(uint8_t dat);

/* USRuart2Rx: gestore di ricezione dati per la porta UART2 (analogo agli handler precedenti).
   inp: dat - il byte ricevuto sulla UART2. */
void USRuart2Rx(uint8_t dat);

/* USRwifiIrq: gestore di interrupt/eventi Wi-Fi per l'applicazione utente.
   Questa funzione viene chiamata quando si verifica un evento Wi-Fi di interesse per l'utente (ad esempio, 
   pacchetti dati ricevuti o variazioni di stato della rete), consentendo di reagire a tali eventi nel contesto dell'applicazione. */
void USRwifiIrq(void);

/* USRmain: funzione principale dell'applicazione utente.
   Viene eseguita dopo l'inizializzazione di base del sistema e avvia la logica specifica dell'applicazione.
   All'interno di questa funzione vengono tipicamente inizializzate le periferiche utente (ad es. abilitazione UART, Wi-Fi), 
   invocate le funzioni di test o configurazione iniziale (es. chiamata a Test_WIFI()) e infine 
   avviato il loop principale dell'applicazione che gestisce le attività operative. */
void USRmain(void);

/* Definizione delle variabili esterne -------------------------------------- */

/* Definizione delle Funzioni esterne --------------------------------------- */

#endif /* USR_MAIN_H_ */
/*EOF*/
