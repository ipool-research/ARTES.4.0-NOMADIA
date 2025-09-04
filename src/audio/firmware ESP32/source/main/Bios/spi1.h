/*
 *	Progetto: 82037601
 *	nome    : spi1.h
 *	descr	:
 *
 */

#ifndef _SPI1_H
#define _SPI1_H

#include "esp_err.h"
#include "driver/spi_master.h"


#define SPI1_CHANNEL      SPI3_HOST

esp_err_t spi1_init();

#endif //  _SPI1_H
