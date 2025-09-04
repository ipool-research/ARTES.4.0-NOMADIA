/*
 *	Progetto: 82037601
 *	nome    : utils.c
 *	descr	:
 *
 */

#include "global.h"



/* Definizione variabili esterne ---------------------------------------------*/
extern uint16_t	tickCadenza;

/* Definizione variabili globali ---------------------------------------------*/

struct
{
	uint8_t		Status;			// Stato led: acceso, spento
	uint16_t	Pin;            // Pin assegnato al led
	uint8_t		Blink;			// Numero di blink da eseguire
	uint16_t	Tout;			// Timeout blink
	uint16_t	RelTout;		// Timeout ricarica blink
}LEDstruct[N_LED];

/* Procedure -----------------------------------------------------------------*/

/* Auto Reset CPU ------------------------------------------------------------*/

/* Reset software della scheda						*/

void RESETcpu (void)
{
	esp_restart();
}


/* Procedure -----------------------------------------------------------------*/

/**
  * @brief  LEDinit: inizializzazione struttura gestione led
  * @param  None
  * @retval ESP_OK o errore
  */

esp_err_t LEDinit(void)
{
    esp_err_t ret;

	LEDstruct[LED1_IDX].Status = 0;
	LEDstruct[LED1_IDX].Blink = 0;
	LEDstruct[LED1_IDX].Pin = LED1_GPIO;
	ret = gpio_set_level(LEDstruct[LED1_IDX].Pin, 0);

	LEDstruct[LED2_IDX].Status = 0;
	LEDstruct[LED2_IDX].Blink = 0;
	LEDstruct[LED2_IDX].Pin = LED2_GPIO;
	ret |= gpio_set_level(LEDstruct[LED2_IDX].Pin, 0);

	if(ret == ESP_OK)
		return ESP_OK;
	else
		return ESP_FAIL;
}


/**
  * @brief  LEDon: accende il led selezionato
  * @param  led = indice del led da accendere
  * @retval ESP_OK o errore
  */

esp_err_t LEDon(uint8_t led)
{
    esp_err_t ret;

	if (led != LED1_IDX && led != LED2_IDX)
		return (ESP_FAIL);

	ret = gpio_set_level(LEDstruct[led].Pin, 1);
	if(ret == ESP_OK)
	{
		LEDstruct[led].Status = 1;
		LEDstruct[led].Blink = 0;
	}

	return (ret);
}


/**
  * @brief  LEDoff: spegne il led selezionato
  * @param  led = indice del led da spegnere
  * @retval ESP_OK o errore
  */

esp_err_t LEDoff(uint8_t led)
{
    esp_err_t ret;

	if (led != LED1_IDX && led != LED2_IDX)
		return (ESP_FAIL);
		
	ret = gpio_set_level(LEDstruct[led].Pin, 0);
	if(ret == ESP_OK)
    {
        LEDstruct[led].Status = 0;
        LEDstruct[led].Blink = 0;
    }

    return (ret);
}


/**
  * @brief  LEDblink: inizializza il blink del led selezionato
  * @param  led = indice del led da far blinkare
  * @param  nBlink = numero di blink da far eseguire (0xFF blink continuo)
  * @param  tout = timeout per blink
  * @retval ESP_OK o errore
  */


esp_err_t LEDblink(uint8_t led, uint8_t nBlink, uint16_t tout)
{
    esp_err_t ret;

	if (led != LED1_IDX && led != LED2_IDX)
		return (ESP_FAIL);

	LEDstruct[led].Tout = tout;
	LEDstruct[led].RelTout = tout;
	LEDstruct[led].Blink = nBlink;

	return (ESP_OK);
}


/**
  * @brief  LEDmanage: gestione blink dei leds
  * @param  t = cadenza Tick timer in msec
  * @retval None
  */

void LEDmanage (uint32_t t)
{
	uint8_t i;

	for(i=0;i<N_LED;i++)
	{
		if(LEDstruct[i].Blink)
		{
			if (LEDstruct[i].Tout > t) LEDstruct[i].Tout -= t;
			else
			{
				if(LEDstruct[i].Status)
				{
				    if(gpio_set_level(LEDstruct[i].Pin, 0) == ESP_OK)
					{
						LEDstruct[i].Tout = LEDstruct[i].RelTout;
						LEDstruct[i].Status = 0;

						if(LEDstruct[i].Blink!= 0xFF)
							LEDstruct[i].Blink--;
					}
				}
				else
				{
				    if(gpio_set_level(LEDstruct[i].Pin, 1) == ESP_OK)
					{
						LEDstruct[i].Tout = LEDstruct[i].RelTout;
						LEDstruct[i].Status = 1;
					}
				}
			}
		}
	}
}


/*EOF*/
