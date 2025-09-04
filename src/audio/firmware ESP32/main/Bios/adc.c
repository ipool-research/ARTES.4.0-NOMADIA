/*
 *	Progetto: 82037601
 *	nome    : adc.c
 *	descr	: driver adc
 *
 *  Created on: 17 lug 2020
 *  Author: CUT1
 */

#include "adc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "gpio.h"

#define ADC1_BITWIDTH			ADC_BITWIDTH_DEFAULT
#define ADC1_ATTEN				ADC_ATTEN_DB_11					//attenuazione di 11dB per espandere la dinamica dell'ADC tra 0 e 3300mV
#define ADC1_NO_OF_SAMPLES		8							//n° di campioni su cui fare la media della misura dell'ADC p

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/

adc_oneshot_unit_handle_t adc1_handle;						//dichiarazione dell' adc1_handler


/**
 * @brief Initialization of ADC1
 *
 * @param[in]  NULL
 *
 * @return
 *			-ESP_OK: On success
 *			-ESP_ERR_INVALID_ARG: Invalid arguments
 *			-ESP_ERR_NO_MEM: No memory
 *			-ESP_ERR_NOT_FOUND: The ADC peripheral to be claimed is already in use
 *			-ESP_FAIL: Clock source isn’t initialised correctly
 */

esp_err_t ADC1init(void)
{
	adc_oneshot_unit_init_cfg_t init_config1 = {
		.unit_id = ADC_UNIT_1,
	};
	return adc_oneshot_new_unit(&init_config1, &adc1_handle);
}


/**
 * @brief Deinitialization of ADC1
 *
 * @param[in]  NULL
 *
 * @return
 *			-ESP_OK: On success
 *			-ESP_ERR_INVALID_ARG: Invalid arguments
 *			-ESP_ERR_NO_MEM: No memory
 *			-ESP_ERR_NOT_FOUND: The ADC peripheral to be claimed is already in use
 *			-ESP_FAIL: Clock source isn’t initialised correctly
 */

esp_err_t ADC1deInit(void)
{
	return adc_oneshot_del_unit(adc1_handle);
}


/**
 * @brief calibration of ADC1
 *
 * function that take the channel of ADC1 and return the pointer to the calibrated output handle
 *
 * @param[in] ADC channel
 * @param[out] pointer to ADC calibration handle
 *
 * @return
 *			-ESP_OK: On success
 *			-ESP_ERR_INVALID_ARG: Invalid arguments
 *			-ESP_ERR_NO_MEM: No memory
 *			-ESP_ERR_NOT_SUPPORTED: Scheme required eFuse bits not burnt
 */


esp_err_t ADC1calibrationChannel(adc_channel_t channel, adc_cali_handle_t *out_handle)
{
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC1_ATTEN,
        .bitwidth = ADC1_BITWIDTH,
    };

    return adc_cali_create_scheme_line_fitting(&cali_config, out_handle);
}

/**
 * @brief read of ADC1
 *
 * Configure ADC channel and calculate the average of ADC1_NO_OF_SAMPLES measurements.
 *
 * @param[in] channel ADC channel
 * @param[out] wAdc pointer to read value
 *
 * @return
 *			-ESP_OK: On success
 *			-ESP_ERR_INVALID_ARG: Invalid arguments
 *			-ESP_ERR_NO_MEM: No memory
 *			-ESP_ERR_NOT_SUPPORTED: Scheme required eFuse bits not burnt
 */
esp_err_t ADC1read(adc_channel_t channel, uint16_t *wAdc)
{
	esp_err_t ret;
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC1_BITWIDTH,
        .atten = ADC1_ATTEN,
    };
    uint32_t adc_reading1 = 0;
    uint16_t adc_raw, j=0;

    ret = adc_oneshot_config_channel(adc1_handle, channel, &config);
    if(ret == ESP_OK)													// config is returned OK
    {
		for(uint8_t idx=0; idx < ADC1_NO_OF_SAMPLES; idx++)				//calculation of average
		{
			if(adc_oneshot_read(adc1_handle, channel, (int*)&adc_raw) == ESP_OK)
			{
				adc_reading1 += adc_raw;								//raw data average
				j++;
			}
		}
		if (j == 0)
			return ESP_FAIL;											//adc_oneshot_config_channel ERROR
		else
		{
			*wAdc = (uint16_t)(adc_reading1 / j);						//the pointed element is the average of the ADC1_NO_OF_SAMPLES measurements
			return ESP_OK;
		}
    }
    else
    	return ret;
}
