/************************************************************************************
 Questo file include e organizza tutti i file di intestazione necessari al progetto.
 Le inclusioni sono suddivise per categorie: librerie di sistema C, FreeRTOS, componenti ESP-IDF,
 driver hardware di basso livello ("Bios"), driver generici ("Drivers") e moduli applicativi ("App").
 In questo modo, tutte le definizioni e i riferimenti globali risultano disponibili a tutto il programma.
 ***********************************************************************************/
#ifndef MAIN_GLOBAL_H_
#define MAIN_GLOBAL_H_

/* Sistema:
 *   Librerie standard del C (funzionalità generali e di base)
 */
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* FreeRTOS:
 *   Header del sistema operativo FreeRTOS (task, code, semafori, eventi)
 */
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

/* esp-idf:
 *   Header delle componenti ESP-IDF (driver ESP32, stack di rete, servizi di sistema, ecc.)
 */
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "driver/gptimer.h"
#include "driver/uart.h"

#include "esp_mac.h"
#include "esp_app_format.h"
#include "esp_event.h"
#include "esp_flash.h"
#include "esp_flash_spi_init.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_types.h"
#include "esp_tls.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_wifi.h"
#include "sdkconfig.h"
#include "nvs_flash.h"

/* Bios:
 *   Driver di basso livello per le periferiche di base (ADC, GPIO, Timer, SPI, UART, ecc.)
 */
#include "adc.h"
#include "cutmain.h"
#include "gpio.h"
#include "timer.h"
#include "spi0.h"
#include "spi1.h"
#include "uart0.h"
#include "utils.h"

/* Drivers:
 *   Driver di componenti esterni o funzioni avanzate (ADS131M0x, SD card, WiFi, file WAV, ecc.)
 */
#include "ADS131M0x.h"
#include "driver_utils.h"
#include "sdcard.h"
#include "wifi.h"
#include "wav_file/WAVFile.h"
#include "wav_file/WAVFileWriter.h"

/* App:
 *   Header dell'applicazione utente
 */
#include "usr_global.h"

#endif /* MAIN_GLOBAL_H_ */
/* EOF */
