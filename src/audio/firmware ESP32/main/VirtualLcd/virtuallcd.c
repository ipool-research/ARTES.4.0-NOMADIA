/*
Formato comandi protocollo seriale:

HD1,HD2,STX,CMD,LEN,DATI,CRC
HD1  il carattere 0x55
HD2  il carattere 0xAA
STX  il carattere 0x02
CMD  il codice comando
LEN  la lunghezza dei DATI
DATI il campo DATI
CRC  il crc calcolato secondo la formula 1+somma di tutti i caratteri da STX all'ultimo dei DATI

Elenco comandi Display modo Testo:

VL_Clear(): Azzera il display LCD
0x55 0xAA STX 0x01 0x00 crc

VL_Curs(r,c): Posiziona il cursore sulla riga/colonna selezionata
0x55 0xAA STX 0x02 0x02 row col crc

VL_EnableCurs(): Abilita la visualizzazione del cursore sulla riga/colonna selezionata
0x55 0xAA STX 0x03 0x00 crc

VL_DisableCurs(): Disabilita la visualizzazione del cursore
0x55 0xAA STX 0x04 0x00 crc

VL_SetForeColor(n): Setta il colore del testo
0x55 0xAA STX 0x05 0x01 n crc
n = codice colore (0 Nero, 1 Blu, 2 Verde, 3 Giallo, 4 Rosso, 5 Magenta,
                  6 Marrone, 7 Grigio, 8 Celeste, 9 Arancio)

VL_Puts(*s): Scrive sul display la stringa ricevuta
0x55 0xAA STX 0x06 len <stringa> crc
Len = lunghezza stringa (compreso 0x00)

VL_SwClear(): Azzera lo scroll display 
0x55 0xAA STX 0x07 0x00 crc

VL_SwPuts(*s): Scrive sullo scroll-display la stringa ricevuta
0x55 0xAA STX 0x08 len <stringa> crc
Len = lunghezza stringa (compreso 0x00)

Gestione Tastiera senza repeat (senza essere informati sul rilascio del tasto):

Pacchetto di Tasto premuto inviato da Virtual LCD:
STX 0x10 code crc
code = codifica ASCII del tasto premuto.
Vengono gestiti solo i tasti numerici da 0 a 9 e quelli alfabetici oltre
ai tasti F1..F8 codificati da 0x70 a 0x78 e altri come Enter, virgola, punto,
Backspace, piu e meno.

Elenco comandi display grafico:

VL_ClearGraph(): Azzera il display LCD grafico 
0x55 0xAA STX 0x20 0x00 crc

VL_InitGraph(nome_asseX,max_X,risol_X,nome_asseY,max_Y,min_Y): Configura gli assi del grafico
0x55 0xAA STX 0x21 len <stringa nome_asse_X><valore massimo_asse_X><risoluzione_asse_X>
                                               <stringa nome_asse_Y><valore massimo_asse_Y><valore_minimo_asse_Y> crc
valore_massimo e risoluzione_asse = 2 bytes cadauno (Little endian)
valore_minimo = 2 bytes con segno (Little endian)

VL_SetGraph(stringa_nome1,colore1,stringa_nome2,colore2,stringa_nome3,colore3): imposta nome e colore serie dati 1,2,3
0x55 0xAA STX 0x22 0x06 s1 c1 s2 c2 s3 c3 crc 
i colori sono gli stessi del comando SetForeColor

VL_SetGraph2(stringa_nome4,colore4,stringa_nome5,colore5,stringa_nome6,colore6): imposta nome e colore serie dati 4,5,6 
0x55 0xAA STX 0x24 0x06 s4 c4 s5 c5 s6 c6 crc 
i colori sono gli stessi del comando SetForeColor

VL_DatoGraph(serie,valoreX,valoreY) : setta i valori X e Y per la serie specificata
0x55 0xAA STX 0x23 0x05 serie datoX datoY crc ETX
serie : 0=serie1, 1=serie2, 2=serie3
datoX = 2 bytes unsigned (Little endian)
datoY = 2 bytes signed (Little endian)
Se datoX = 0 l'asse X si incrementa in automatico di <risoluzione_asse_X>, altrimenti
il punto viene posizionato dove indicato. 

Timeout Rx pacchetto tasto premuto: 50msec
Timeout Attesa fine Tx: 100msec (sicurezza interna)

Il cursore viene gestito in automatico solo dopo la VL_Puts o dopo una VL_Get..
in ogni caso viene incrementato solo in orizzontale senza controllo su fine schermo

Se il campo valoreX della 'serie1' è = 0 il grafico può scorrere fino al limite del programma VirtualLCD,
altrimenti è compito del software macchina gestire lo scorrimento del grafico ed evitare eventuali
overflow sulla base dei tempi.

La 'serie1' (indice 0) funge anche da trigger unico per lo scorrimento della base dei tempi.

Le 'serie2' e 'serie3' se indicano valoreX=0 vengono posizionate sull'ultimo punto della serie1,
altrimenti è compito del software macchina gestire il posizionamento sul grafico e lo scorrimento.

Dopo una VL_ClearGraph si deve fare sempre VL_SetGraph e VL_InitGraph.

In caso di interfaccia via Uart0, si deve inserire la VL_Rx() nella USRuart0Rx() come segue:
void USRuart0Rx(uint8_t dat)
{
  VL_Rx(dat);  // Rx dati da Virtual LCD
}
e abilitare suddetta funzione con la UART0enable_UserRx()
*/

#include "global.h"

/* Definizione delle variabili esterne --------------------------------------*/
extern uint16_t tickCadenza;
extern uint8_t  uart0PktTxed;

/* Definizione delle costanti -----------------------------------------------*/
#define VL_MODOTX  VL_INTERRUPT  // Selezione modalità Tx: VL_POLLING (invio sincrono) o VL_INTERRUPT (invio con interrupt)

/* Definizione delle variabili -----------------------------------------------*/
uint8_t VLrxBuf[VL_LRXBUF];   // Buffer di ricezione dati da Virtual LCD
uint8_t VLrxIdx;             // Indice corrente nel buffer di ricezione
uint8_t VLtxBuf[VL_LTXBUF];   // Buffer di trasmissione dati verso Virtual LCD
uint8_t VLtxIdx;             // Indice corrente nel buffer di trasmissione
uint8_t VLtxTout;            // Contatore timeout per completamento trasmissione
uint8_t VLtxData[VL_LTXBUF];  // Buffer per il pacchetto da trasmettere (dati da inviare)
uint8_t VLlenTxData;         // Lunghezza del pacchetto dati in preparazione
uint8_t VLcrc;               // Valore CRC calcolato per l'ultimo pacchetto
uint8_t VLrxTout;            // Contatore timeout per completamento ricezione
uint8_t VLcursRow;           // Riga corrente del cursore (posizione verticale)
uint8_t VLcursCol;           // Colonna corrente del cursore (posizione orizzontale)

/* Basso livello 
--------------------------------------------------------------*/

/* Gestione Timeout Tx e Rx */
void VL_Tout()
{
    // Aggiorna i contatori di timeout di trasmissione e ricezione ad ogni tick di sistema
    if (VLtxTout >= tickCadenza) VLtxTout -= tickCadenza;
    else VLtxTout = 0;

    if (VLrxTout >= tickCadenza) VLrxTout -= tickCadenza;
    else VLrxTout = 0;
}

/* Ricezione pacchetto da seriale (interrupt)
   inp: dat = dato ricevuto da Uart0
   out: se VLrxIdx >= 4 allora pacchetto valido ricevuto */
void VL_Rx(uint8_t dat)
{
    if (VLrxTout == 0 && VLrxIdx) {
        VLrxIdx = 0;  // Se scade il timeout a metà ricezione, resetta il protocollo
    }

    switch (VLrxIdx)
    {
        case 0:
            if (dat == STX) {
                VLrxBuf[VLrxIdx++] = dat;
                VLrxTout = VL_TOUT_RX;
            }
            // atteso byte STX iniziale
            break;

        case 1:
            if (dat == VL_CMD_KEYB) {
                VLrxBuf[VLrxIdx++] = dat;
            }
            // atteso comando KEYB come secondo byte
            break;

        case 2:
            VLrxBuf[VLrxIdx++] = dat;
            // terzo byte (codice tasto o LEN) memorizzato
            break;

        case 3:
            VLrxBuf[VLrxIdx] = dat;  // quarto byte ricevuto (CRC)
            dat = 1;
            dat += VLrxBuf[0];
            dat += VLrxBuf[1];
            dat += VLrxBuf[2];
            if (dat == VLrxBuf[VLrxIdx]) {
                VLrxIdx++;  // CRC corretto, pacchetto completo (VLrxIdx diventa 4)
            } else {
                VLrxIdx = 0;  // CRC errato, scarto il pacchetto e riparto da zero
            }
            break;

        default:
            break;
    }
}

/* Funzioni principali -------------------------------------------------------*/

/* Inizializzazione Seriale su UART0
   out: Indici buffer Tx e Rx azzerati, interrupt Rx abilitato.
        Le porte Tx/Rx e il baud rate devono essere configurati nel Main Program per Uart0 */
void VL_Init(void)
{
    VLrxIdx = 0;
    VLtxIdx = 0;
    VLlenTxData = 0;
    // Buffer e indici inizializzati; la configurazione hardware UART0 va effettuata esternamente
}

/* Invia un comando formattato su porta UART0 (uso interno)
   inp: len = lunghezza del pacchetto da spedire (numero di byte in VLtxData)
   out: Pacchetto completo inserito in coda di trasmissione (VLtxIdx=0 a fine invio) */
void VL_Send(uint8_t len)
{
    uint16_t i;

    // Prepara il pacchetto nel buffer di trasmissione (header + dati + CRC) e lo invia
    VLtxBuf[0] = HD1;  // Header 1 (0x55)
    VLtxBuf[1] = HD2;  // Header 2 (0xAA)
    VLtxBuf[2] = STX;  // Byte STX (inizio frame)
    VLcrc = 1 + STX;   // Inizializza CRC (1 + STX)

    for (i = 0; i < len; i++) {
        VLtxBuf[3 + i] = VLtxData[i];
        VLcrc += VLtxData[i];  // Somma ogni byte dei dati per calcolare CRC
    }
    VLtxBuf[3 + i] = VLcrc;  // Appende il CRC al termine del buffer dati
    VLlenTxData = len + 4;   // Lunghezza totale del pacchetto (HD1,HD2,STX + Dati + CRC)

    #if (VL_MODOTX == VL_POLLING)
    // Trasmissione in polling (invio byte sincrono, utile per debug o interfacce lente)
    /*
    i = 0;
    while (i < VLlenTxData) {
        Delay_us(3000);              // attesa 3ms tra byte (adattamento per convertitore USB-RS232)
        uart0PktTxed = TRUE;         // forza flag per invio singolo byte
        UART0sendMsg(&VLtxBuf[i], 1);
        i++;
    }
    */
    UARTsendMsgPolling(VLtxBuf, VLlenTxData);
    VLtxIdx = 0;  // Fine trasmissione (polling)
    #else
    // Trasmissione con interrupt (non blocca la CPU durante l'invio)
    UART0sendMsg(VLtxBuf, VLlenTxData);
    #endif
}

/* Azzera LCD modo Testo */
void VL_Clear(void)
{
    VLcursRow = 0;
    VLcursCol = 0;
    VLtxData[0] = VL_CMD_CLEAR;
    VLtxData[1] = 0;
    VL_Send(2);
}

/* Posiziona il cursore sulla riga/colonna selezionata */
void VL_Curs(uint8_t row, uint8_t col)
{
    VLcursRow = row;
    VLcursCol = col;
    VLtxData[0] = VL_CMD_CURS;
    VLtxData[1] = 2;
    VLtxData[2] = row;
    VLtxData[3] = col;
    VL_Send(4);
}

/* Abilita la visualizzazione del cursore */
void VL_EnableCurs(void) 
{
    VLtxData[0] = VL_CMD_ENACURS;
    VLtxData[1] = 0;
    VL_Send(2);
}

/* Disabilita la visualizzazione del cursore */
void VL_DisableCurs(void) 
{
    VLtxData[0] = VL_CMD_DISCURS;
    VLtxData[1] = 0;
    VL_Send(2);
}

/* Setta il colore del testo */
void VL_SetForeColor(uint8_t n)
{
    VLtxData[0] = VL_CMD_SFCOL;
    VLtxData[1] = 1;
    VLtxData[2] = n;
    VL_Send(3);
}

/* Azzera scroll display */
void VL_SwClear(void)
{
    VLtxData[0] = VL_CMD_SWCLEAR;
    VLtxData[1] = 0;
    VL_Send(2);
}

/* Invia una stringa al display 
   inp: s = puntatore alla stringa ASCII da visualizzare */
void VL_Puts(uint8_t *s)
{
    uint8_t len, i, dat;

    len = strlen((const char*)s) + 1;
    VLtxData[0] = VL_CMD_PUTS;
    VLtxData[1] = len;
    for (i = 0; i < len; i++) {
        VLtxData[i + 2] = *s;
        s++;
    }
    VL_Send(len + 2);

    dat = VLcursCol + len - 1;
    // Calcola posizione finale del cursore dopo aver scritto la stringa
    if (dat >= VL_LROW) {  // Se supera l'ultima colonna della riga
        if (VLcursRow == VL_LCOL) {
            // Se il cursore era sull'ultima riga, lo posiziona sull'ultimo carattere disponibile
            dat = VL_LROW - 1;
        } else {
            // Altrimenti sposta il cursore a inizio riga successiva
            dat = 0;
            VLcursRow++;
        }
    }
    VL_Curs(VLcursRow, dat);  // Aggiorna la posizione del cursore sul Virtual LCD
}

/* Invia una stringa allo scroll-display 
   inp: s = puntatore alla stringa ASCII da visualizzare su scroll */
void VL_SwPuts(uint8_t *s)
{
    uint8_t len, i;

    len = strlen((char*)s) + 1; 
    VLtxData[0] = VL_CMD_SWPUTS;
    VLtxData[1] = len;
    for (i = 0; i < len; i++) {
        VLtxData[i + 2] = *s;
        s++;
    }
    VL_Send(len + 2);
}

/* Azzera LCD Grafico 
   (Dopo questa chiamata eseguire sempre VL_SetGraph e VL_InitGraph, come richiesto dal software VirtualLCD) */
void VL_ClearGraph(void)
{
    VLtxData[0] = VL_CMD_GRAPHCLR;
    VLtxData[1] = 0;
    VL_Send(2);
}

/* Configura gli assi dell' LCD Grafico 
   inp: name_axisX = stringa nome asse orizzontale
        max_X = valore massimo asse X
        resol_X = risoluzione (incremento base tempi) asse X
        name_axisY = stringa nome asse verticale
        max_Y = valore massimo asse Y
        min_Y = valore minimo asse Y
   Nota: la lunghezza totale di name_axisX + name_axisY non deve superare 40 caratteri */
void VL_InitGraph(uint8_t *name_axisX, uint16_t max_X, uint16_t resol_X, uint8_t *name_axisY, uint16_t max_Y, signed int min_Y)
{
    uint8_t i;

    VLtxData[0] = VL_CMD_GRAPHINIT;
    i = 2;  // indice nel buffer dati (0=CMD, 1=verrà lunghezza dati)
    while (*name_axisX) {
        VLtxData[i++] = *name_axisX++;
    }
    VLtxData[i++] = 0x00;  // terminatore stringa asse X
    VLtxData[i++] = (uint8_t) max_X;
    VLtxData[i++] = (uint8_t) (max_X >> 8);
    VLtxData[i++] = (uint8_t) resol_X;
    VLtxData[i++] = (uint8_t) (resol_X >> 8);
    while (*name_axisY) {
        VLtxData[i++] = *name_axisY++;
    }
    VLtxData[i++] = 0x00;  // terminatore stringa asse Y
    VLtxData[i++] = (uint8_t) max_Y;
    VLtxData[i++] = (uint8_t) (max_Y >> 8);
    VLtxData[i++] = (uint8_t) min_Y;
    VLtxData[i++] = (uint8_t) (min_Y >> 8);
    VLtxData[1] = i;  // imposta lunghezza totale dati
    VL_Send(i + 2);
}

/* Imposta nome e colore per le serie di dati 1,2,3 sul LCD Grafico 
   inp: name1 = stringa nome serie1, color1 = codice colore serie1
        name2 = stringa nome serie2, color2 = codice colore serie2
        name3 = stringa nome serie3, color3 = codice colore serie3
   Nota: la lunghezza totale delle tre stringhe non deve superare 40 caratteri */
void VL_SetGraph(char *name1, uint8_t color1, char *name2, uint8_t color2, char *name3, uint8_t color3)
{
    uint8_t i;

    VLtxData[0] = VL_CMD_GRAPHSET;
    i = 2;
    while (*name1) {
        VLtxData[i++] = *name1++;
    }
    VLtxData[i++] = 0x00;
    VLtxData[i++] = color1;
    while (*name2) {
        VLtxData[i++] = *name2++;
    }
    VLtxData[i++] = 0x00;
    VLtxData[i++] = color2;
    while (*name3) {
        VLtxData[i++] = *name3++;
    }
    VLtxData[i++] = 0x00;
    VLtxData[i++] = color3;
    VLtxData[1] = i;
    VL_Send(i + 2);
}

/* Imposta nome e colore per le serie di dati 4,5,6 sul LCD Grafico 
   inp: name4 = stringa nome serie4, color4 = codice colore serie4
        name5 = stringa nome serie5, color5 = codice colore serie5
        name6 = stringa nome serie6, color6 = codice colore serie6
   Nota: la lunghezza totale delle tre stringhe non deve superare 40 caratteri */
void VL_SetGraph2(uint8_t *name4, uint8_t color4, uint8_t *name5, uint8_t color5, uint8_t *name6, uint8_t color6)
{
    uint8_t i;

    VLtxData[0] = VL_CMD_GRAPHSET2;
    i = 2;
    while (*name4) {
        VLtxData[i++] = *name4++;
    }
    VLtxData[i++] = 0x00;
    VLtxData[i++] = color4;
    while (*name5) {
        VLtxData[i++] = *name5++;
    }
    VLtxData[i++] = 0x00;
    VLtxData[i++] = color5;
    while (*name6) {
        VLtxData[i++] = *name6++;
    }
    VLtxData[i++] = 0x00;
    VLtxData[i++] = color6;
    VLtxData[1] = i;
    VL_Send(i + 2);
}

/* Setta i valori X e Y per la serie specificata
   inp: series = indice serie (0,1,2)
        valX = 0 per incremento automatico, altrimenti posizione specifica asse X
        valY = valore per asse Y (signed) */
void VL_DatoGraph(uint8_t series, uint16_t valX, signed int valY)
{
    VLtxData[0] = VL_CMD_GRAPHDAT;
    VLtxData[1] = 5;
    VLtxData[2] = series;
    VLtxData[3] = (uint8_t) valX;
    VLtxData[4] = (uint8_t) (valX >> 8);
    VLtxData[5] = (uint8_t) valY;
    VLtxData[6] = (uint8_t) (valY >> 8);
    VL_Send(7);
}

/* Controlla se è stato ricevuto un pacchetto valido (tasto premuto)
   out: 0 = no, 1 = sì */
uint8_t VL_IsKey(void)
{
    if (VLrxIdx >= 4)
        return 1;
    return 0;
}

/* Ritorna il codice del tasto premuto
   output: codice ASCII del tasto premuto */
uint8_t VL_GetKey(void)
{
    uint8_t dat;
    dat = VLrxBuf[2];
    VLrxIdx = 0;
    return dat;
}

/* Attende finché un tasto non viene premuto
   output: codice ASCII del tasto premuto */
uint8_t VL_WaitKey(void)
{
    while (!VL_IsKey());
    return VL_GetKey();  
}

/* Attesa inserimento campo numerico intero 8-bit senza segno
   inp: par = 0, accetta 3 cifre decimali + CR (inserimento obbligatorio)
        par = 1, accetta 2 cifre esadecimali + CR (inserimento obbligatorio) */
uint8_t VL_GetByteValue(uint8_t par)
{
    uint8_t i, key;
    uint8_t kb[8];
    uint8_t kk[2];
    uint16_t byDat;

    i = 0;
    key = 0;
    while (key != K_CR)
    {
        key = VL_WaitKey();
        if (i && key == K_DEL) {
            // Cancellazione ultimo carattere
            i--;
            kb[i] = 0x00;
            kk[0] = ' ';
            kk[1] = 0x00;
            VLcursCol--;
            VL_Curs(VLcursRow, VLcursCol);
            VL_Puts(kk);
            VLcursCol--;
            VL_Curs(VLcursRow, VLcursCol);
        } else {
            if (i == 0 && key == K_CR) key = 0;  // Non accetta CR come primo carattere
            if (par == 0 && i < 3 && (key >= '0' && key <= '9')) 
            { 
                // Accetto solo 3 cifre decimali
                kb[i++] = key;
                kk[0] = key;
                kk[1] = 0x00;
                VL_Puts(kk);  // Echo del carattere inserito
            }
            if (par == 1 && i < 2 && ((key >= '0' && key <= '9') || key == 'A' || key == 'a' ||
                                      key == 'B' || key == 'b' || key == 'C' || key == 'c' ||
                                      key == 'D' || key == 'd' || key == 'E' || key == 'e' ||
                                      key == 'F' || key == 'f')) 
            {
                // Accetto solo 2 cifre esadecimali
                kb[i++] = key;
                kk[0] = key;
                kk[1] = 0x00;
                VL_Puts(kk);
            }
        }
    }
    kb[i] = 0x00;  // Sostituisce CR con terminatore stringa 0x00
    if (par == 0) byDat = (uint8_t) atoi((const char*)kb);  // Converte stringa in numero decimale
    else if (par == 1) sscanf((const char*)kb, "%2hx", &byDat);  // Converte stringa in numero esadecimale

    return byDat;
}

/* Attesa inserimento campo numerico intero 16-bit senza segno 
   inp: par = 0, accetta 5 cifre decimali + CR (inserimento obbligatorio)
        par = 1, accetta 4 cifre esadecimali + CR (inserimento obbligatorio) */
uint16_t VL_GetWordValue(uint8_t par)
{
    uint8_t i, key;
    uint8_t kb[8];
    uint8_t kk[2];
    uint16_t wDat;

    i = 0;
    key = 0;
    while (key != K_CR)
    {
        key = VL_WaitKey();
        if (i && key == K_DEL) {
            // Cancellazione ultimo carattere
            i--;
            kb[i] = 0x00;
            kk[0] = ' ';
            kk[1] = 0x00;
            VLcursCol--;
            VL_Curs(VLcursRow, VLcursCol);
            VL_Puts(kk);
            VLcursCol--;
            VL_Curs(VLcursRow, VLcursCol);
        } else {
            if (i == 0 && key == K_CR) key = 0;  // Non accetta CR come primo carattere
            if (par == 0 && i < 5 && (key >= '0' && key <= '9')) {
                // Accetto solo 5 cifre decimali
                kb[i++] = key;  
                kk[0] = key;
                kk[1] = 0x00;
                VL_Puts(kk);
            }
            if (par == 1 && i < 4 && ((key >= '0' && key <= '9') || key == 'A' || key == 'a' ||
                                      key == 'B' || key == 'b' || key == 'C' || key == 'c' ||
                                      key == 'D' || key == 'd' || key == 'E' || key == 'e' ||
                                      key == 'F' || key == 'f')) 
            {
                // Accetto solo 4 cifre esadecimali
                kb[i++] = key;  
                kk[0] = key;
                kk[1] = 0x00;
                VL_Puts(kk);
            }
        }
    }
    kb[i] = 0x00;  // Sostituisce CR con terminatore stringa 0x00
    if (par == 0) wDat = (uint16_t) atoi((const char*)kb);
    else if (par == 1) sscanf((const char*)kb, "%4hx", &wDat);

    return wDat; 
}

/* Attesa inserimento campo numerico intero 32-bit con segno 
   Accetta 10 cifre + CR (inserimento obbligatorio)
   inp: sign = K_MINUS per accettare valori negativi */
signed long VL_GetIntValue(uint8_t sign)
{
    uint8_t i, key;
    uint8_t kb[12];  // 10 cifre più eventuale segno + CR
    uint8_t kk[2];
    signed long lDat;

    i = 0;
    key = 0;
    while (key != K_CR)
    {
        key = VL_WaitKey();
        if (i && key == K_DEL) {
            // Cancellazione ultimo carattere
            i--;
            kb[i] = 0x00;
            kk[0] = ' ';
            kk[1] = 0x00;
            VLcursCol--;
            VL_Curs(VLcursRow, VLcursCol);
            VL_Puts(kk);
            VLcursCol--;
            VL_Curs(VLcursRow, VLcursCol);
        } else {
            if (i == 0 && key == K_CR) key = 0;  // Non accetta CR come primo carattere
            if (i < 10 && ((key >= '0' && key <= '9') || (key == sign && i == 0))) {
                // Accetta solo 10 cifre numeriche o il segno all'inizio
                kb[i++] = key;          
                kk[0] = key;
                kk[1] = 0x00;
                VL_Puts(kk);  // Echo del carattere
            }
        }
    }
    kb[i] = 0x00;  // Sostituisce CR con terminatore stringa 0x00
    lDat = (signed long) atoi((const char*)kb);
    return lDat; 
}

/* Attesa inserimento campo numerico decimale (float) con segno 
   Accetta 16 cifre + CR (inserimento obbligatorio)
   inp: sign = K_MINUS per accettare valori negativi
   out: valore convertito in virgola mobile (float) */
float VL_GetFloatValue(uint8_t sign)
{
    uint8_t i, key;
    uint8_t kb[18];  // 16 cifre + eventuale punto/segno + CR
    uint8_t kk[2];
    float flDat;

    i = 0;
    key = 0;
    while (key != K_CR)
    {
        key = VL_WaitKey();
        if (i && key == K_DEL) {
            // Cancellazione ultimo carattere
            i--;
            kb[i] = 0x00;
            kk[0] = ' ';
            kk[1] = 0x00;
            VLcursCol--;
            VL_Curs(VLcursRow, VLcursCol);
            VL_Puts(kk);
            VLcursCol--;
            VL_Curs(VLcursRow, VLcursCol);
        } else {
            if (i == 0 && key == K_CR) key = 0;  // Non accetta CR come primo carattere
            if (i < 16 && ((key >= '0' && key <= '9') || 
                           (key == sign && i == 0) ||  // accetta '-' in prima posizione
                           (key == K_DOT && i > 0)))   // accetta '.' se non primo carattere
            {
                // Accetta solo 16 caratteri numerici (o segno/punto secondo regole)
                kb[i++] = key;          
                kk[0] = key;
                kk[1] = 0x00;
                VL_Puts(kk);
            }
        }
    }
    kb[i] = 0x00;  // Sostituisce CR con terminatore stringa 0x00
    flDat = atof((const char*)kb);  // Converte stringa in float
    return flDat;        
}

/* Attesa inserimento campo stringa
   Accetta fino a VL_LSTR caratteri + CR (inserimento obbligatorio)
   inp: lmax = numero massimo di caratteri della stringa (escluso terminatore)
        *p = puntatore al buffer dove salvare la stringa inserita
   out: *p = stringa inserita (massimo lmax caratteri + terminatore 0x00) */
void VL_GetString(uint8_t lmax, uint8_t *p)
{
    uint8_t i, key;
    uint8_t kk[2];
  
    i = 0;
    key = 0;
    if (lmax == 0 || lmax > VL_LSTR) lmax = VL_LSTR;
    while (key != K_CR)
    {
        key = VL_WaitKey();
        if (i && key == K_DEL) {
            // Cancellazione ultimo carattere
            i--;
            p--;
            *p = 0x00;
            kk[0] = ' ';
            kk[1] = 0x00;
            VLcursCol--;
            VL_Curs(VLcursRow, VLcursCol);
            VL_Puts(kk);
            VLcursCol--;
            VL_Curs(VLcursRow, VLcursCol);
        } else {
            if (i == 0 && key == K_CR)
                key = 0;  // Non accetta CR come primo carattere (stringa vuota non consentita)
            if (i < lmax) {
                i++;
                *p++ = key;
                kk[0] = key;
                kk[1] = 0x00;
                if (kk[0] != K_CR) {
                    VL_Puts(kk);  // Echo del carattere (tranne CR)
                }
            }
        }
    }
    if (i < lmax) p--;
    *p = 0x00;  // Termina la stringa con 0x00 (sostituisce CR)
}

/* Attesa inserimento opzionale di un campo stringa
   Accetta fino a VL_LSTR caratteri + CR (inserimento non obbligatorio: ESC per annullare)
   inp: lmax = numero massimo di caratteri della stringa (escluso terminatore)
        *p   = puntatore al buffer dove salvare la stringa inserita
   out: *p = stringa inserita (max lmax caratteri + terminatore 0x00)
        return: 0 se uscita con ESC (annullato), altrimenti numero caratteri inseriti+1 */
uint8_t VL_GetOptString(uint8_t lmax, uint8_t *p)
{
    uint8_t i, key;
    uint8_t kk[2];
  
    i = 0;
    key = 0;
    // if (lmax == 0 || lmax > VL_LSTR) lmax = VL_LSTR;
    while (key != K_CR)
    {
        key = VL_WaitKey();
        if (i && key == K_DEL) {
            // Cancellazione ultimo carattere
            i--;
            p--;
            *p = 0x00;
            kk[0] = ' ';
            kk[1] = 0x00;
            VLcursCol--;
            VL_Curs(VLcursRow, VLcursCol);
            VL_Puts(kk);
            VLcursCol--;
            VL_Curs(VLcursRow, VLcursCol);
        } else {
            if (key == K_ESC) return 0;        // Esce per ESC premuto (annulla inserimento)
            if (key == K_CR && i == 0) return 1; // Esce per CR premuto subito (stringa opzionale vuota)
            if (i < lmax) {
                i++;
                *p++ = key;
                kk[0] = key;
                kk[1] = 0x00;
                if (kk[0] != K_CR) {
                    VL_Puts(kk);  // Echo del carattere (tranne il CR)
                }
            }
        }
    }
    if (i < lmax) {
        p--;
        i--;
    }
    *p = 0x00;  // Termina la stringa con 0x00
    return (i + 1);
}

/*EOF*/
