/*
 *	Progetto: 82037601
 *	nome    : uart0.h
 *	descr	: driver della uart 0
 *
 */

#include "global.h"

#define UART0_RTS 			(UART_PIN_NO_CHANGE)
#define UART0_CTS 			(UART_PIN_NO_CHANGE)
#define BUF_SIZE 			(1024)
#define PATTERN_CHR_NUM    	(3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/
#define RD_BUF_SIZE 		(BUF_SIZE)

static const int RX_BUF_SIZE = 1024;

//static const char *tag= "UART DRIVER";

unsigned char  	uart0TxBuffer[UART0_TXBUFLEN];		// buffer di trasmissione
uint16_t 		uart0TxIndex;						// Indice Tx buffer
uint16_t 		uart0LenTxMsg = 0;					// Lunghezza pacchetto da trasmettere
unsigned char	uart0PktTxed;						// Se = 1 pacchetto trasmesso
unsigned char 	uart0RxBuffer[UART0_RXBUFLEN];		// Buffer di ricezione
uint16_t		uart0ps,uart0pl;					// Puntatori Rx Buffer
unsigned char	uart0_enable_UserRx = 0;			// Flag abilitazione funzione Utente

uint8_t uart0_buff[1024];
/*
 * Task di ricezione
 */
static void UART0rxTask(void *arg)
{
	uint8_t dat;

    while (1)
    {
    	 const int rxBytes = uart_read_bytes(UART_NUM_0, uart0_buff, RX_BUF_SIZE, 100 / portTICK_PERIOD_MS);
    	 if (rxBytes > 0)
    	 {
			int i;
			for(i = 0; i < rxBytes; i++)
			{
				uart0RxBuffer[uart0ps] = uart0_buff[i];
				dat = uart0RxBuffer[uart0ps];
				uart0ps++;													// Incremento puntatore a scrittura
				if (uart0ps >= UART0_RXBUFLEN)
				{
						uart0ps = 0;										// Buffer circolare
				}
				if (uart0ps == uart0pl)
				{									// Se buffer pieno invalido il carattere
						uart0ps--;
						if (uart0ps == 0xFFFF)
						{
							uart0ps = UART0_RXBUFLEN-1;
						}
				}
				if (uart0_enable_UserRx)
				{
					USRuart0Rx(dat);
				}
			}
	 	 }
    }
}

esp_err_t UART0init(uint32_t baud)
{
	esp_err_t ret;

	// Azzeramento buffers UART2, puntatori e flags
	for (int i = 0; i < UART0_RXBUFLEN; i++) uart0RxBuffer[i] = 0;
	for (int i = 0; i < UART0_TXBUFLEN; i++) uart0TxBuffer[i] = 0;
	uart0TxIndex = uart0LenTxMsg = 0;
	uart0PktTxed = true;
	uart0ps = uart0pl = 0;

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;

    // uart0 driver gi� installato, lo cancello e reinizializzo
    if (uart_is_driver_installed(UART_NUM_0)){
    	uart_driver_delete(UART_NUM_0);
    }

    ret = uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags);
    if(ret != ESP_OK)
    	return ret;
    ret = uart_param_config(UART_NUM_0, &uart_config);
    if(ret != ESP_OK)
    	return ret;
    ret = uart_set_pin(UART_NUM_0, TXD0_GPIO, RXD0_GPIO, UART0_RTS, UART0_CTS);
    if(ret != ESP_OK)
    	return ret;

    return (ESP_OK);
}

/* Abilita ricezione da Uart0 */
void UART0startRx(void)
{
	uart_enable_rx_intr(UART_NUM_0);

	//Attiva task di ricezione
	xTaskCreate(UART0rxTask, "UART0rxTask", 1024*2, NULL, configMAX_PRIORITIES, NULL);

}

/* Svuota il buffer di ricezione */

void UART0flushRxBuffer(void)
{
	uart_flush(UART_NUM_0);
	uart0ps = uart0pl = 0;
}

/* Ritorna la flag di fine trasmissione pacchetto
   out : TRUE  = fine trasmissione
		 FALSE = trasmissione in corso					*/
uint8_t UART0isTxEnd (void)
{
	return (uart0PktTxed);
}

/* Invia messaggio in polling (no interrupt) su seriale Uart0
	input: c    = puntatore al messaggio
		   size = lunghezza del messaggio

	Tx interrupt disabilitato						*/

void UART0sendMsgPolling (uint8_t *c, uint16_t size)
{
	uart0PktTxed = false;				// Resetto flag di Pacchetto Trasmesso

	uart_write_bytes(UART_NUM_0, (const char*)c, size);

	uart_wait_tx_done(UART_NUM_0, 20 / portTICK_PERIOD_MS); // wait timeout is 100 RTOS ticks (TickType_t)

	uart0PktTxed = true;				// Setto flag di Pacchetto Trasmesso
}


/* Invia messaggio su seriale Uart0
	input: c    = puntatore al messaggio
		   size = lunghezza del messaggio	*/

void UART0sendMsg (uint8_t *c, uint16_t size)
{
	uart0PktTxed = false;				// Resetto flag di Pacchetto Trasmesso

	uart_write_bytes(UART_NUM_0, (const char*)c, size);

	uart_wait_tx_done(UART_NUM_0, 20 / portTICK_PERIOD_MS); // wait timeout is 100 RTOS ticks (TickType_t)

	uart0PktTxed = true;				// Setto flag di Pacchetto Trasmesso

}


/* Ritorna lo stato del buffer di ricezione
   out : n = numero caratteri ricevuti			*/


uint16_t UART0isRxData (void)
{
	if (uart0ps == uart0pl) return (0);

	if (uart0ps > uart0pl)
		return (uart0ps-uart0pl);
	else
		return (UART0_RXBUFLEN-uart0pl+uart0ps);
}


/* Legge un carattere alla volta dal buffer di ricezione
   E' compito del Main program verificare preventivamente che il Buffer non sia
	vuoto, altrimenti ritorna per default 0x00											*/


uint8_t UART0receiveData(void)
{
	uint8_t dat;

	if (uart0ps != uart0pl)
	{
		dat = uart0RxBuffer[uart0pl++];
		if (uart0pl >= UART0_RXBUFLEN)
			uart0pl = 0;				// Buffer circolare
		return(dat);
	}
	return(0);
}


/* Abilita chiamata funzione Utente */

void UART0enable_UserRx(void)
{
	uart0_enable_UserRx = 1;
}


/* Disabilita chiamata funzione Utente */

void UART0disable_UserRx(void)
{
	uart0_enable_UserRx = 0;
}
/* EOF */
