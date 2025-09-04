#ifndef SDCARD_H_
#define SDCARD_H_

#include <sys/types.h>
#include <stdint.h>

/* Definizione costanti 
----------------------------------------------------------*/
#define MOUNT_POINT "/sdcard"  // Punto di mount del filesystem sulla scheda SD

/* Definizione tipi 
--------------------------------------------------------------*/

/* Definizione prototipi 
---------------------------------------------------------*/
esp_err_t SDCARDinit(void);                                // Inizializza la scheda SD e monta il filesystem
esp_err_t SDCARDdeInit(void);                              // Smonta il filesystem e disconnette la scheda SD
esp_err_t SDCARDopenFile(char *fileName);                  // Apre il file di nome fileName sulla SD (modalità predefinita di lettura/scrittura)
esp_err_t SDCARDcloseFile(void);                           // Chiude il file attualmente aperto sulla SD
esp_err_t SDCARDwriteFile(char *data, uint16_t len, uint16_t *numBytesWrite); // Scrive len byte dal buffer data sul file aperto (ritorna in numBytesWrite i byte scritti)
esp_err_t SDCARDreadFile(char *data, uint16_t len, uint16_t *numBytesRead);   // Legge fino a len byte dal file aperto nel buffer data (ritorna in numBytesRead i byte letti)
esp_err_t SDCARDcreateFile(char *fileName);                // Crea un nuovo file sulla SD con nome fileName

#endif /* SDCARD_H_ */

/*EOF*/
