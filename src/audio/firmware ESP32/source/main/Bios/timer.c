/*
 *	Progetto: 82037601
 *	nome    : TIMER.c
 *	descr	: definizione timer software
 *
 */

#include "global.h"

#define TIMER_DIVIDER         80  								//  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

/* Definizione variabili esterne ---------------------------------------------*/

/* Definizione variabili globali ---------------------------------------------*/

uint16_t			tickCadenza = 10;		// Cadenza del SysTick timer			[msec]
volatile uint16_t	tickCounter;			// Timer software per SysTick


/* Procedure -----------------------------------------------------------------*/


/* Timer software ------------------------------------------------------------*/

/* Attesa passiva di t millisecondi basata sulla cadenza del Tick Timer */

void WaitMsec(uint16_t t)
{
	tickCounter = t;
	while(tickCounter);
}


/* Setta per attesa di t millisecondi */

void SetMsec (uint16_t t)
{
	tickCounter = t;
}


/* Test timer software scaduto
   1 = scaduto
   0 = non scaduto							*/

uint8_t isWaitEnd (void)
{
	if (tickCounter) return(0);
	return(1);
}

void TIMERcallback(void)
{
	if (tickCounter > tickCadenza) tickCounter -= tickCadenza;
	else tickCounter = 0;

	USRtickTimer();			// CallBack per utente
}

/*EOF*/
