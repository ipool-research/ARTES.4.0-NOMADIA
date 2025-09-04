#ifndef WAVFILEWRITER_H_
#define WAVFILEWRITER_H_

#include <sys/types.h>
#include <stdint.h>

/* Definizione costanti 
----------------------------------------------------------*/

/* Definizione tipi 
--------------------------------------------------------------*/

/* Definizione prototipi 
---------------------------------------------------------*/
esp_err_t WAVWRITERinit(void);                                 // Inizializza il modulo di scrittura WAV (monta la SD card)
esp_err_t WAVWRITERcreateFile(char *file_name, int sample_rate); // Crea un nuovo file WAV con il nome e sample_rate specificati
esp_err_t WAVWRITERwriteFile(int16_t *samples, int count);     // Scrive un buffer di 'count' campioni audio 16-bit nel file WAV aperto
esp_err_t WAVWRITERcloseFile(void);                            // Chiude il file WAV, aggiornando l'header con le dimensioni finali

#endif /* WAVFILEWRITER_H_ */

/*EOF*/
