/*
 *	Progetto: 82037601
 *	nome    : utils.h
 *	descr	:
 *
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>

/* Definizioni Macro software ----------------------------------------------- */

#define BREAK_UINT32( var, ByteNum ) \
          (uint8_t)((uint32_t)(((var) >>((ByteNum) * 8)) & 0x00FF))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((uint32_t)((uint32)((Byte0) & 0x00FF) \
          + ((uint32_t)((Byte1) & 0x00FF) << 8) \
          + ((uint32_t)((Byte2) & 0x00FF) << 16) \
          + ((uint32_t)((Byte3) & 0x00FF) << 24)))

#define BUILD_UINT16(loByte, hiByte) \
          ((uint16_t)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

#define BUILD_UINT8(hiByte, loByte) \
          ((uint8_t)(((loByte) & 0x0F) + (((hiByte) & 0x0F) << 4)))

#define HI_UINT8(a) (((a) >> 4) & 0x0F)
#define LO_UINT8(a) ((a) & 0x0F)

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define sign(a)  ((a < 0) ? (-1) : (1))


/* Definizioni di tipo ------------------------------------------------------ */

/* Definizione delle costanti ----------------------------------------------- */
#define N_LED           2               // Numero LED da gestire
#define LED1_IDX        0               // LED 1
#define LED2_IDX        1               // LED 2


/* Definizione delle Prototypes --------------------------------------------- */

void 	RESETcpu (void);

esp_err_t LEDinit(void);
esp_err_t LEDon(uint8_t led);
esp_err_t LEDoff(uint8_t led);
esp_err_t LEDblink(uint8_t led, uint8_t nBlink, uint16_t tout);
void      LEDmanage (uint32_t t);

#endif /* UTILS_H_ */

/*EOF*/
