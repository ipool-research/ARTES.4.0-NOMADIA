/*
 *	Progetto: 82037601
 *	nome    : timer.h
 *	descr	:
 *
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

/* Definizioni Macro software ----------------------------------------------- */

/* Definizioni di tipo ------------------------------------------------------ */

/* Definizione delle costanti ----------------------------------------------- */

/* Definizione delle Prototypes --------------------------------------------- */


void	WaitMsec(uint16_t t);
void	SetMsec (uint16_t t);
uint8_t isWaitEnd (void);

void TIMERcallback(void);
#endif /* TIMER_H_ */

/*EOF*/
