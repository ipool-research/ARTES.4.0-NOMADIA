/*
 *	Progetto: 82037601
 *	nome    : adc.h
 *	descr	: driver adc
 *
 *  Created on: 17 lug 2020
 *  Author: CUT1
 */
#ifndef MAIN_BIOS_ADC_H_
#define MAIN_BIOS_ADC_H_
//#include <stdint.h>



#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

//the order of fuction use is:
//first ADC1init
//second ADC1CalibrationChannel
//third ADC1read

esp_err_t ADC1init(void);
esp_err_t ADC1deInit(void);
esp_err_t ADC1calibrationChannel(adc_channel_t channel, adc_cali_handle_t *out_handle);
esp_err_t ADC1read(adc_channel_t channel, uint16_t *wAdc);

#endif /* MAIN_BIOS_ADC_H_ */
