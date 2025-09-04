/*
 *	Progetto: 82037601
 *	nome    : uart0.h
 *	descr	: driver della uart 0
 *
 */

#ifndef UART0_H_
#define UART0_H_

#include <stdint.h>
#include "esp_err.h"

#define	UART0_RXBUFLEN		1024
#define	UART0_TXBUFLEN		1024

/* Definizione delle Prototypes --------------------------------------------- */

esp_err_t 	UART0init(uint32_t baud);
void		UART0startTx(void);
void 		UART0startRx(void);
void 		UART0stopRx(void);
void 		UART0flushRxBuffer(void);
uint8_t 	UART0isTxEnd (void);
void 		UART0sendMsgPolling (uint8_t *c, uint16_t size);
void 		UART0sendMsg (uint8_t *c, uint16_t size);
uint16_t 	UART0isRxData (void);
uint8_t 	UART0receiveData(void);
void 		UART0enable_UserRx(void);
void 		UART0disable_UserRx(void);

#endif /* UART0_H_ */
