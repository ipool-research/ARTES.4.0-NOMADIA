#ifndef VIRTUALLCD_H_
#define VIRTUALLCD_H_

//#include "typedefine.h"

/* Definizione costanti 
----------------------------------------------------------*/
#define VL_LROW     40   // Lunghezza (numero colonne) del display testuale
#define VL_LCOL     20   // Numero di righe del display testuale

#define VL_POLLING  0    // Modalità TX in polling
#define VL_INTERRUPT 1   // Modalità TX con interrupt

#define VL_LRXBUF   8    // Lunghezza buffer ricezione seriale
#define VL_LTXBUF   64   // Lunghezza buffer trasmissione seriale

#define VL_LSTR     30   // Lunghezza massima accettata per stringhe in input

#define HD1         0x55 // Carattere speciale Header1 (inizio pacchetto)
#define HD2         0xAA // Carattere speciale Header2
#define STX         0x02 // Carattere Start-of-Text
#define K_F1        0x0E
#define K_F2        0x0F
#define K_F3        0x10
#define K_F4        0x11
#define K_F5        0x12
#define K_F6        0x13
#define K_F7        0x14
#define K_F8        0x15
#define K_CR        0x0D
#define K_DEL       0x08
#define K_PLUS      '+'
#define K_MINUS     '-'
#define K_SLASH     '/'
#define K_BACKSLASH 0x5C  // '\'
#define K_ASTERISK  '*'
#define K_COMM      ','
#define K_DOT       '.'
#define K_COLON     ':'
#define K_SEMICOLON ';'
#define K_UNDER     '_'
#define K_EQUAL     '='
#define K_AT        '@'
#define K_PIPE      '|'
#define K_ESC       0x1B
#define K_PGUP      0x21
#define K_PGDW      0x22
#define K_LEFT      0x25
#define K_UP        0x26
#define K_RIGHT     0x27
#define K_DOWN      0x28

#define VL_CMD_CLEAR    0x01  // Comandi protocollo Virtual LCD (modalità testo)
#define VL_CMD_CURS     0x02
#define VL_CMD_ENACURS  0x03
#define VL_CMD_DISCURS  0x04
#define VL_CMD_SFCOL    0x05
#define VL_CMD_PUTS     0x06
#define VL_CMD_SWCLEAR  0x07
#define VL_CMD_SWPUTS   0x08
#define VL_CMD_KEYB     0x10
#define VL_CMD_GRAPHCLR 0x20
#define VL_CMD_GRAPHINIT 0x21
#define VL_CMD_GRAPHSET 0x22
#define VL_CMD_GRAPHDAT 0x23
#define VL_CMD_GRAPHSET2 0x24

#define VL_BLACK   0   // Codici colore
#define VL_BLUE    1
#define VL_GREEN   2
#define VL_YELLOW  3
#define VL_RED     4
#define VL_MAGENTA 5
#define VL_BROWN   6
#define VL_GREY    7
#define VL_CYAN    8
#define VL_ORANGE  9

#define VL_TOUT_TX 100  // Timeout trasmissione pacchetto [ms]
#define VL_TOUT_RX 50   // Timeout ricezione pacchetto [ms]

/* Definizione dei Prototipi ---------------------------------------------*/
// Funzioni di gestione Virtual LCD (implementate in virtuallcd.c)
void VL_Tout(void);
void VL_Rx(uint8_t dat);
void VL_Init(void);
void VL_Send(uint8_t len);
void VL_Clear(void);
void VL_Curs(uint8_t row, uint8_t col);
void VL_EnableCurs(void);
void VL_DisableCurs(void);
void VL_SetForeColor(uint8_t n);
void VL_SwClear(void);
void VL_Puts(uint8_t *s);
void VL_SwPuts(uint8_t *s);
void VL_ClearGraph(void);
void VL_InitGraph(uint8_t *name_axisX, uint16_t max_X, uint16_t resol_X, uint8_t *name_axisY, uint16_t max_Y, signed int min_Y);
void VL_SetGraph(char *name1, uint8_t color1, char *name2, uint8_t color2, char *name3, uint8_t color3);
void VL_SetGraph2(uint8_t *name4, uint8_t color4, uint8_t *name5, uint8_t color5, uint8_t *name6, uint8_t color6);
void VL_DatoGraph(uint8_t series, uint16_t valX, signed int valY);
uint8_t VL_IsKey(void);
uint8_t VL_GetKey(void);
uint8_t VL_WaitKey(void);
uint8_t VL_GetByteValue(uint8_t par);
uint16_t VL_GetWordValue(uint8_t par);
signed long VL_GetIntValue(uint8_t sign);
float VL_GetFloatValue(uint8_t sign);
void VL_GetString(uint8_t lmax, uint8_t *p);
uint8_t VL_GetOptString(uint8_t lmax, uint8_t *p);

#endif /* VIRTUALLCD_H_ */

/*EOF*/
