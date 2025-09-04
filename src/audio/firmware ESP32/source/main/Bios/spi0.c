/*
 *	Progetto: 82037601
 *	nome    : spi0.c
 *	descr	:
 *
 */

#include "global.h"


esp_err_t spi0_init()
{
    esp_err_t ret;

    spi_bus_config_t buscfg = {
        .miso_io_num = MISO0_GPIO,
        .mosi_io_num = MOSI0_GPIO,
        .sclk_io_num = SCK0_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 8,
        .flags = SPICOMMON_BUSFLAG_MASTER
    };

    // Initialize the SPI bus
    ret = spi_bus_initialize(SPI0_CHANNEL, &buscfg, 0);
    //ESP_ERROR_CHECK(ret);

    return ret;
}
