/*
 *	Progetto: 82037601
 *	nome    : spi0.h
 *	descr	:
 *
 */

#ifndef _SPI0_H
#define _SPI0_H

#include "esp_err.h"
#include "driver/spi_master.h"


#define SPI0_CHANNEL      SPI2_HOST

esp_err_t spi0_init();

#endif //  _SPI0_H
