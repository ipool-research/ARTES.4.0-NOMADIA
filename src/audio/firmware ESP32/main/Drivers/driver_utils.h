#ifndef DRIVER_UTILS_H_
#define DRIVER_UTILS_H_

#include <sys/types.h>
#include <stdint.h>

/* Definizione costanti 
----------------------------------------------------------*/
#define UM2UM_LIN_POINTS 7   // Numero di punti per la linearizzazione U.M.1 -> U.M.2

/* Definizione tipi 
--------------------------------------------------------------*/
typedef struct {
    float dat[UM2UM_LIN_POINTS];  // Valori campione (es. letture ADC) in U.M.1
    float um[UM2UM_LIN_POINTS];   // Valori corrispondenti convertiti in U.M.2
} FLOATUM2UMTAB;  // Struttura dati per tabella di conversione tra due unità di misura

/* Definizione prototipi 
---------------------------------------------------------*/
float Float_UM1_to_UM2(float fDat, FLOATUM2UMTAB *p);  // Converte fDat da U.M.1 a U.M.2 utilizzando la tabella di conversione p
float TransfLinFloat(float x1, float x2, float y1, float y2, float x);  // Restituisce l'interpolazione lineare per il valore x sui punti (x1,y1)-(x2,y2)

#endif /* DRIVER_UTILS_H_ */

/*EOF*/
