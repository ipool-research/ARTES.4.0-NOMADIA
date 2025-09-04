/************************************************************************************
* Questo modulo fornisce funzioni per inizializzare la scheda SD e per 
 * gestire la lettura/scrittura di file su filesystem FAT (montaggio, apertura, 
 * lettura, scrittura e creazione di file) in ambiente ESP32.
 * Note     : 
 *   La tabella seguente mostra la corrispondenza tra le modalità POSIX per i file e i flag equivalenti di FatFs.
 * 
 * 		POSIX	FatFs
 * 		"r"		FA_READ
 * 		"r+"	FA_READ | FA_WRITE
 * 		"w"		FA_CREATE_ALWAYS | FA_WRITE
 * 		"w+"	FA_CREATE_ALWAYS | FA_WRITE | FA_READ
 * 		"a"		FA_OPEN_APPEND | FA_WRITE
 * 		"a+"	FA_OPEN_APPEND | FA_WRITE | FA_READ
 * 		"wx"	FA_CREATE_NEW | FA_WRITE
 * 		"w+x"	FA_CREATE_NEW | FA_WRITE | FA_READ
 *
 ***********************************************************************************/
#include "global.h"
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

/* Definizione costanti ----------------------------------------------------------*/

#define SDCARD_MAX_BUFFER_WRITE		(16 * 1024)  // Dimensione massima (16 KB) per buffer di scrittura (unità di allocazione FAT)

/* Definizione variabili esterne -------------------------------------------------*/

/* Definizione variabili  --------------------------------------------------------*/
const char mount_point[] = MOUNT_POINT;  // Percorso di mount del filesystem SD (definito da MOUNT_POINT)

sdmmc_host_t host = SDSPI_HOST_DEFAULT();  // Configurazione host SPI (interfaccia SDSPI) di default
sdmmc_card_t *card;  // Puntatore alla struttura di descrizione della scheda SD
FILE 		 *file;  // Puntatore al file corrente aperto sulla SD (usato per operazioni di I/O)

/* Definizione prototype ---------------------------------------------------------*/

/* SDCARDinit: inizializza la scheda SD e monta il filesystem FAT
   Questa funzione configura l'interfaccia SPI (bus SPI3) impostando i pin dedicati
   e utilizza la libreria esp_vfs_fat per montare il filesystem FAT della SD card
   sul percorso definito da MOUNT_POINT (es. "/sdcard").
   In caso di primo montaggio fallito, la scheda **non** viene formattata automaticamente.
   out: ESP_OK (0) se l'inizializzazione e il montaggio avvengono con successo,
        altrimenti restituisce un codice di errore (esp_err_t) indicante la causa del fallimento.
*/
esp_err_t SDCARDinit(void)
{
	esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,  // Non formatta la scheda se il montaggio del filesystem fallisce
        .max_files = 5,				// Max number of open files
        .allocation_unit_size = SDCARD_MAX_BUFFER_WRITE  // Unità di allocazione del filesystem (16 KB)
    };
                
    printf("Initializing SD card\r\n");

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = MOSI1_GPIO,  // GPIO MOSI (Master Out Slave In) del bus SPI per SD
        .miso_io_num = MISO1_GPIO,  // GPIO MISO (Master In Slave Out) del bus SPI per SD
        .sclk_io_num = SCK1_GPIO,   // GPIO SCK (clock) del bus SPI per SD
        .quadwp_io_num = -1,        // Non utilizzato (disabilitata linea Write Protect)
        .quadhd_io_num = -1,        // Non utilizzato (disabilitata linea HOLD)
        .max_transfer_sz = 4000,    // Dimensione massima di trasferimento SPI (byte)
    };
    host.slot = SPI3_HOST;  // Seleziona lo slot SPI3 (VSPI) per la comunicazione con la SD
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH1);  // Inizializza il bus SPI sullo slot specificato (usa DMA canale 1)
	if (ret != ESP_OK) {
		printf("Failed to initialize bus.\r\n");
		return ret;
	}

    // Inizializza lo slot SPI senza utilizzare segnali di card detect (CD) e write protect (WP).
    // Se la scheda di sviluppo dispone di questi segnali, impostare gpio_cd e gpio_wp in slot_config.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = CSSD_GPIO;  // GPIO di Chip Select della SD card
    slot_config.host_id = host.slot;  // Imposta l'ID dell'host SPI (coincide con lo slot configurato sopra)

    printf("Mounting filesystem\r\n");
    // Monta il filesystem FAT dalla SD card sul mount point specificato
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

	if (ret != ESP_OK) {
		// Controlla l'esito del montaggio del filesystem sulla SD
		if (ret == ESP_FAIL) {
			// Errore nel montare il filesystem (ad es. filesystem assente o danneggiato)
			printf("Failed to mount filesystem.\r\n");
		} else {
			// Errore generico di inizializzazione della scheda SD
			printf("Failed to initialize the card (%s).\r\n", esp_err_to_name(ret));
		}
		return ret;
	}
	printf("Filesystem mounted\r\n");

    // La scheda SD è stata inizializzata; è possibile stampare le sue proprietà (vedi sdmmc_card_print_info).
	//    sdmmc_card_print_info(stdout, card);

	return ESP_OK;
}


/* SDCARDdeInit: smonta il filesystem e de-inizializza la scheda SD
   Questa funzione rimuove il filesystem montato dalla scheda SD e libera le risorse SPI.
   Dopo questa chiamata la scheda SD non è più accessibile finché non viene nuovamente inizializzata.
   out: ESP_OK (0) se lo smontaggio e la de-inizializzazione avvengono con successo,
        altrimenti restituisce un codice di errore esp_err_t.
*/
esp_err_t SDCARDdeInit(void)
{
	esp_err_t ret;
	
	// Smonta il filesystem dalla SD card e la disconnette dal VFS
    esp_vfs_fat_sdcard_unmount(mount_point, card);

    // Libera il bus SPI utilizzato dalla scheda SD
    ret = spi_bus_free(host.slot);
	
	return ret;
}


/* SDCARDopenFile: apre un file sulla scheda SD
   Questa funzione compone il percorso completo del file (MOUNT_POINT/nomefile)
   e apre il file in modalità lettura/scrittura ("r+"). Il file specificato deve esistere;
   in caso contrario l'operazione fallisce.
   inp: fileName: nome del file da aprire (relativo alla directory di mount della SD)
   out: ESP_OK se il file viene aperto correttamente, altrimenti ESP_FAIL (errore).
   Nota: il file rimane aperto dopo la chiamata e dovrà essere chiuso manualmente con SDCARDcloseFile.
*/
esp_err_t SDCARDopenFile(char *fileName)
{    
    char path[32];  // Buffer per costruire il percorso completo del file (directory + nome)
    sprintf(path, "%s/%s", MOUNT_POINT, fileName);  // Compone il percorso completo (es. "/sdcard/<nome>")
    printf("Opening file %s: ", path);
    file = fopen(path, "r+");  // Apre il file in lettura/scrittura; fallisce se il file non esiste
	if (file == NULL) {
		// Verifica se il file è stato aperto correttamente
		printf("error\r\n");
		return ESP_FAIL;
	}

    printf("ok\r\n");
    return ESP_OK;
}


/* SDCARDcloseFile: chiude il file precedentemente aperto sulla scheda SD
   Questa funzione chiude il file aperto tramite SDCARDopenFile, liberando le risorse associate.
   out: ESP_OK se la chiusura del file avviene con successo, altrimenti ESP_FAIL.
   Nota: il file da chiudere deve essere stato aperto in precedenza con SDCARDopenFile (FILE* valido).
*/
esp_err_t SDCARDcloseFile(void)
{
	int ret;
    printf("Closing file: ");
    ret = fclose(file);  // Chiude il file aperto in precedenza; restituisce 0 se successo
	if (ret != 0) {
		// Controlla il risultato della fclose (0 = successo, !=0 = errore)
		printf("error\r\n");
		return ESP_FAIL;
	}

    printf("ok\r\n");
    return ESP_OK;
}


/* SDCARDwriteFile: scrive dati sul file aperto sulla scheda SD
   Questa funzione copia byte dal buffer specificato all'interno del file attualmente aperto.
   inp: data: puntatore al buffer contenente i dati da scrivere
        len: numero di byte da scrivere dal buffer
        numBytesWrite: puntatore a variabile dove viene restituita la quantità di byte effettivamente scritta
   out: ESP_OK se tutti i dati sono stati scritti correttamente (numBytesWrite == len),
        altrimenti ESP_FAIL in caso di errore di scrittura.
   Nota: richiede che un file sia già stato aperto tramite SDCARDopenFile.
*/
esp_err_t SDCARDwriteFile(char *data, uint16_t len, uint16_t *numBytesWrite)
{
    printf("Writing file: ");
	*numBytesWrite = fwrite(data, 1, len, file);  // Scrive 'len' byte dal buffer 'data' sul file; restituisce byte realmente scritti (salvati in numBytesWrite)
	if (*numBytesWrite != len) {
		// Verifica se tutti i byte richiesti sono stati scritti (confronta len con numBytesWrite)
		printf("error\r\n");
		return ESP_FAIL;
	}

    printf("ok\r\n");
    return ESP_OK;
}


/* SDCARDreadFile: legge dati dal file aperto sulla scheda SD
   Questa funzione legge fino a len byte dal file attualmente aperto e li memorizza nel buffer di destinazione.
   inp: data: puntatore al buffer di destinazione dove salvare i dati letti
        len: numero massimo di byte da leggere
        numBytesRead: puntatore a variabile dove viene restituito il numero di byte effettivamente letti
   out: ESP_OK se almeno un byte è stato letto (numBytesRead > 0), altrimenti ESP_FAIL se non vengono letti dati.
   Nota: richiede che un file sia già stato aperto tramite SDCARDopenFile. Un ritorno di ESP_FAIL può indicare un errore
         oppure che è stata raggiunta la fine del file (EOF).
*/
esp_err_t SDCARDreadFile(char *data, uint16_t len, uint16_t *numBytesRead)
{
    printf("Reading file: ");
	*numBytesRead = fread((char*)data, 1, len, file);  // Legge fino a 'len' byte dal file nel buffer 'data'; salva il numero di byte letti in numBytesRead
	if (*numBytesRead == 0) {
		// Se non sono stati letti byte (possibile EOF o errore), segnala fallimento
		printf("error\r\n");
		return ESP_FAIL;
	}

    printf("ok\r\n");
    return ESP_OK;
}


/* SDCARDcreateFile: crea un nuovo file sulla scheda SD
   Questa funzione crea (o sovrascrive se esiste) un file sul filesystem della SD.
   inp: fileName: nome del file da creare
   out: ESP_OK se il file viene creato/troncato con successo, altrimenti ESP_FAIL in caso di errore.
   Nota: il file viene aperto in modalità "w" (scrittura) e chiuso immediatamente dopo la creazione.
*/
esp_err_t SDCARDcreateFile(char *fileName)
{    
    char path[32];  // Buffer per costruire il percorso completo del nuovo file
    sprintf(path, "%s/%s", MOUNT_POINT, fileName);  // Compone il percorso completo del file (directory di mount + nome)
    printf("Creating file %s: ", path);
    file = fopen(path, "w");  // Crea il file in modalità scrittura ("w", sovrascrive se esiste)
    if (file == NULL) {
        printf("error\r\n");
        return ESP_FAIL;
    }
    fclose(file);  // Chiude subito il file appena creato
    printf("ok\r\n");
    return ESP_OK;
}

/* EOF */
