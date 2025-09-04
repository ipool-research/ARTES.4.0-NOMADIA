#include "global.h"

/* Definizione variabili esterne 
-------------------------------------------------*/

/* Definizione costanti 
----------------------------------------------------------*/


/*******************************************************************************
*****
 * Funzione     : Float_UM1_to_UM2
 * input        : fDat = dato in U.M.1 (float)
 *                p    = puntatore alla tabella di conversione (FLOATUM2UMTAB)
 * output       : valore convertito in U.M.2 (float)
 * descr        : Converte un valore float da Unità di Misura 1 a Unità di Misura 2
 *                Esempio:
 *                  valori in ADC        valori convertiti
 *                  FLOATUM2UMTAB Prova = {{0, 512, 563, 597, 1023, 1023},
 *                                         {0, 300, 330, 350, 600, 600}};
 *                  UM2UM_LIN_POINTS = 7 punti fissi di linearizzazione (i valori ADC sono in ordine crescente)
 *******************************************************************************
****/
float Float_UM1_to_UM2(float fDat, FLOATUM2UMTAB *p)
{
    float x;
    ushort j = 0;
    float Dat, Dat1, Dat2, Dat3, Dat4;

    // La tabella di linearizzazione contiene valori crescenti
    // Trova il segmento corretto per fDat nella tabella di conversione
    while (fDat > p->dat[j] && j < (UM2UM_LIN_POINTS - 1) && p->dat[j + 1] != 0) {
        j++;
    }

    if (j == 0) j = 1;  // Se fDat è inferiore al primo punto tabellato, usa il primo intervallo

    Dat  = p->dat[j - 1];  // Punto ADC precedente
    Dat1 = p->dat[j];      // Punto ADC successivo
    Dat2 = p->um[j - 1];   // Valore convertito corrispondente al punto precedente
    Dat3 = p->um[j];       // Valore convertito corrispondente al punto successivo
    Dat4 = fDat;           // Valore da convertire

    x = TransfLinFloat(Dat, Dat1, Dat2, Dat3, Dat4);  // Interpolazione lineare tra i due punti tabella
    return x;
}

/*******************************************************************************
*****
 * Funzione     : TransfLinFloat
 * input        : x1, x2 = estremi asse X; y1, y2 = valori corrispondenti su asse Y; x = valore da trasformare
 * output       : risultato interpolato (float)
 * descr        : Calcola la trasformazione lineare (interpolazione) per il valore x
 *******************************************************************************
****/
float TransfLinFloat(float x1, float x2, float y1, float y2, float x)
{
    float v, v1, v2, v3;

    // Interpolazione lineare tra i punti (x1,y1) e (x2,y2)
    if (x2 != x1) {
        v1 = y2 - y1;
        v2 = x - x1;
        v3 = x2 - x1;
        v = (v1 * v2 / v3) + y1;
    } else {
        v = 0;  // Evita divisione per zero se x1 == x2
    }

    return v;
}

/* EOF */
