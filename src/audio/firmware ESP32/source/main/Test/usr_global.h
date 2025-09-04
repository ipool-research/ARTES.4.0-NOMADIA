/*
 *      Questo file funge da header centralizzato per l'applicazione utente, includendo al suo interno 
 *      tutti gli altri header globali necessari. Includendo "usr_global.h" in un sorgente, si ha accesso 
 *      a tutte le definizioni, i prototipi e le strutture condivise tra i vari moduli del progetto.
 */
#ifndef USR_GLOBAL_H_
#define USR_GLOBAL_H_

#include "global.h"     // Definizioni e inclusioni globali di sistema e driver di base (framework Espressif e hardware)
#include "usr_main.h"   // Dichiarazioni delle funzioni principali dell'applicazione utente
#include "usr_utils.h"  // Funzioni di utilità generiche per il programma utente
#include "test.h"       // Prototipi delle funzioni di test hardware/software
#include "virtuallcd.h" // Interfaccia per la gestione di un display LCD virtuale (utilizzato per debug o simulazione)

#endif /* USR_GLOBAL_H_ */
/*EOF*/
