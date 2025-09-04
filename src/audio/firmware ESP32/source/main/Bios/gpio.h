/*
 *	Progetto: 82037601
 *	nome    : gpio.h
 *	descr	: definizione dei GPIO
 *
 */

#include "driver/gpio.h"

#ifndef MAIN_BIOS_GPIO_H_
#define MAIN_BIOS_GPIO_H_

// see SPI manual - note: SPI half duplex mode does not support using DMA with both MOSI and MISO phases.
#define DMA_CHANNEL      	0   // CH2 used by HSPI/SPI2
#define PIN_NUM_CLK    		GPIO_NUM_6    // or IO14 - used by HSPI/SPI2
#define PIN_NUM_MISO    	GPIO_NUM_7    // or IO13 - used by HSPI/SPI2
#define PIN_NUM_MOSI    	GPIO_NUM_8    // or IO12 - used by HSPI/SPI2
#define PIN_NUM_HOLD    	GPIO_NUM_9    // or IO4 - used by HSPI/SPI2
#define PIN_NUM_WP      	GPIO_NUM_10   // or IO2 - used by HSPI/SPI2
#define PIN_NUM_SCS     	GPIO_NUM_11   // or IO15 - used by HSPI/SPI2


// GPIO PINS
#define ADC_VBAT_GPIO		GPIO_NUM_36
// #define _GPIO	   	    GPIO_NUM_39
#define MISO0_GPIO			GPIO_NUM_34
#define DRDY_GPIO   		GPIO_NUM_35
#define CSADC_GPIO   		GPIO_NUM_32
#define SYNC_GPIO   		GPIO_NUM_33
#define MOSI0_GPIO 			GPIO_NUM_25
#define I2S_SCK_GPIO 		GPIO_NUM_26
#define I2S_SDA_GPIO		GPIO_NUM_27
#define SCK0_GPIO  			GPIO_NUM_14
// #define _GPIO			GPIO_NUM_12
// #define _GPIO			GPIO_NUM_13
// #define _GPIO   			GPIO_NUM_15
// #define _GPIO			GPIO_NUM_2
#define FLASH_GPIO		   	GPIO_NUM_0
// #define _GPIO		    GPIO_NUM_4
#define MISO1_GPIO       	GPIO_NUM_5
#define SCK1_GPIO       	GPIO_NUM_18
#define MOSI1_GPIO 			GPIO_NUM_19
#define CSSD_GPIO	 		GPIO_NUM_21
#define RXD0_GPIO 			GPIO_NUM_3
#define TXD0_GPIO 			GPIO_NUM_1
#define LED2_GPIO 			GPIO_NUM_22
#define LED1_GPIO	 		GPIO_NUM_23


#endif /* MAIN_BIOS_GPIO_H_ */
