/*
 *	Progetto: 82037601
 *	nome    : cutmain.h
 *	descr	: Bios per Nuvap Minicustom 72035300
 *
 */

#ifndef CUTMAIN_H_
#define CUTMAIN_H_


#define CSADC_L  gpio_set_level(CSADC_GPIO, 0)
#define CSADC_H  gpio_set_level(CSADC_GPIO, 1)

#define SYNC_L   gpio_set_level(SYNC_GPIO, 0)
#define SYNC_H   gpio_set_level(SYNC_GPIO, 1)

#define CSSD_L   gpio_set_level(CSSD_GPIO, 0)
#define CSSD_H   gpio_set_level(CSSD_GPIO, 1)

#define LED2_L   gpio_set_level(LED2_GPIO, 0)
#define LED2_H   gpio_set_level(LED2_GPIO, 1)

#define LED1_L   gpio_set_level(LED1_GPIO, 0)
#define LED1_H   gpio_set_level(LED1_GPIO, 1)

void cut_main(void);
void app_main(void);

#endif /* CUTMAIN_H_ */
