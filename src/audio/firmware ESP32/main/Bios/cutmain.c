/* Progetto:  82037601
   nome    :  CUTMAIN.C
   descriz :  Bios per Scheda 72037600 PINT iPoll

	vers. 0.1  Prima versione
	xx/06/24   Luca Boggioni, Nicola Galli.

	sBiosCode[9] contiene il codice della scheda per cui il Bios è stato implementato
	sBiosVer[5]  contiene la versione del Bios

	Assegnazione porte al Microprocessore:
	LUK rivedere una volta fatto lo schema definitivo

	Pin			Pin#	Dir		Descrizione		     							Label				Reset		Pull	Int. Edge

	SENSOR_VP 	4	 	Ian		Pin lettura tensione batteria					ADC_VBAT			-
	SENSOR_VN 	5	 	-		Non utilizzato									-					-
	IO34 		6	 	Ihw		MISO per SPI 0  								MISO0				-
	IO35 		7	 	I		Segnale data ready da ADC esterno 				DRDY				-
	IO32 		8	 	O		Chip select SPI per ADC esterno 				CSADC				1
	IO33 		9	 	O		Sync/Reset ADC esterno	 				   		SYNC				1
	IO25 		10	 	Ohw		MOSI per SPI 0									MOSI0				0
	IO26 		11	 	Ohw		Clock I2Sbus			 				   		I2S_SCK				1
	IO27 		12	 	Bhw		Dato I2Sbus0									I2S_SDA				1
	IO14 		13	 	Ohw		SCK per SPI	0									SCK0				0
	IO12 		14		-		Non utilizzato									-					-
	IO13 		16	 	-		Non utilizzato									-					-
	IO15 		23	 	-		Non utilizzato									-					-
	IO2 		24	 	-		Non utilizzato									-					-
	IO0 		25	 	I		Flash Mode input								FLASH#				-
	IO4 		26	 	-		Non utilizzato									-					-
	IO5 		29	 	Ihw		MISO per SPI 1  								MISO1				-
	IO18 		30	 	Ohw		SCK per SPI	1									SCK1				0
	IO19 		31	 	Ohw		MOSI per SPI 1									MOSI1				0
	IO21 		33	 	O		Chip select SPI per SDCARD      				CSSD				1
	RXD0 		34	 	Ihw		Rx Debug Uart									RXD0				-
	TXD0 		35	 	Ohw		Tx Debug Uart 									TXD0				-
	IO22 		36	 	O		Led 2					 				   		LED2				0
	IO23 		37	 	O		Led 1					 				   		LED1				0

	Tempi:
	fclock		: 80MHz
	ticktimer	: 10msec

	Nota strutture: per default sono allineate a un byte (packed=1)											*/


#include "global.h"


uint8_t BootCode[9];
uint8_t BootVer[5];
const char BiosCode[9] = {"82037601"};		// Codice Bios
const char BiosVer[5]  = {"v1.0"};			// Versione del Bios


static void io_after_reset(void)
{
    // Configure Pins
	gpio_reset_pin(ADC_VBAT_GPIO);
    gpio_set_direction(ADC_VBAT_GPIO,   GPIO_MODE_INPUT);

	gpio_reset_pin(MISO0_GPIO);
    gpio_set_direction(MISO0_GPIO,   GPIO_MODE_INPUT);

	gpio_reset_pin(DRDY_GPIO);
    gpio_set_direction(DRDY_GPIO,   GPIO_MODE_INPUT);

    gpio_reset_pin(CSADC_GPIO);
	gpio_set_direction(CSADC_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(CSADC_GPIO, 1);

    gpio_reset_pin(SYNC_GPIO);
	gpio_set_direction(SYNC_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(SYNC_GPIO, 1);

    gpio_reset_pin(MOSI0_GPIO);
	gpio_set_direction(MOSI0_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(MOSI0_GPIO, 0);

    gpio_reset_pin(I2S_SCK_GPIO);
	gpio_set_direction(I2S_SCK_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(I2S_SCK_GPIO, 1);

	gpio_reset_pin(I2S_SDA_GPIO);
	gpio_set_direction(I2S_SDA_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(I2S_SDA_GPIO, 1);

    gpio_reset_pin(SCK0_GPIO);
	gpio_set_direction(SCK0_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(SCK0_GPIO, 0);

	gpio_reset_pin(FLASH_GPIO);
    gpio_set_direction(FLASH_GPIO,  GPIO_MODE_OUTPUT);
	gpio_set_level(FLASH_GPIO, 1);

    gpio_reset_pin(MISO1_GPIO);
	gpio_set_direction(MISO1_GPIO, GPIO_MODE_INPUT);

	gpio_reset_pin(SCK1_GPIO);
	gpio_set_direction(SCK1_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(SCK1_GPIO, 0);

	gpio_reset_pin(MOSI1_GPIO);
	gpio_set_direction(MOSI1_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(MOSI1_GPIO, 0);

	gpio_reset_pin(CSSD_GPIO);
	gpio_set_direction(CSSD_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(CSSD_GPIO, 1);

	gpio_reset_pin(RXD0_GPIO);
    gpio_set_direction(RXD0_GPIO,   GPIO_MODE_INPUT);

    gpio_reset_pin(TXD0_GPIO);
	gpio_set_direction(TXD0_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(TXD0_GPIO, 1);

    gpio_reset_pin(LED2_GPIO);
	gpio_set_direction(LED2_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(LED2_GPIO, 0);

    gpio_reset_pin(LED1_GPIO);
	gpio_set_direction(LED1_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(LED1_GPIO, 0);
}


TimerHandle_t tmr;
int blink_id = 1;

void tickTimerCallback( TimerHandle_t pxTimer )
{
	TIMERcallback();
}

float buffer[]={1189,731,570,100,-352,-349,-734,-1213,-1608,-1956,-2398,-2587,-2222,-1485,-726,41,723,913,1123,975,652,409,-50,-719,-1087,-1589,-1922,-2258,-2679,-2491,-2040,-1174,-650,55,383,783,1115,1180,936,465,179,-200,-592,-1189,-1428,-2029,-2077,-2196,-1867,-1582,-1156,-262,276,648,1175,1243,900,674,129,-280,-537,-944,-1358,-1926,-2343,-2259,-2343,-1971,-1270,-7,849,1037,1360,1282,1036,546,50,-301,-398,-1107,-1760,-1762,-2031,-2224,-2056,-1587,-975,-317,232,836,1091,1093,1023,907,33,-188,-275,-911,-1305,-1727,-2170,-2438,-2578,-2015,-1084,-392,358,996,1184,1293,906,1001,545,117,-488,-1151,-1639,-2127,-2386,-2544,-2397,-1983,-1085,-388,-28,517,1150,1103,1045,745,338,-254,-285,-568,-1101,-1663,-2174,-2235,-2187,-2222,-1522,-492,335,499,476,727,835,822,446,30,-243,-1119,-1215,-1447,-2219,-2709,-2799,-2252,-1451,-873,-286,706,1442,1541,1357,1066,668,-14,-624,-908,-1351,-1638,-2205,-2487,-2279,-2061,-1972,-1097,-9,536,821,1112,1211,973,529,132,-93,-578,-1176,-1503,-1961,-2326,-2368,-2045,-1453,-1009,5,760,920,1123,1090,1135,629,-68,-477,-561,-1056,-1662,-2110,-2471,-2363,-2409,-1713,-703,-283,255,943,1072,1113,1269,610,127,-324,-580,-782,-1368,-1794,-1960,-2200,-2288,-2035,-1431,-696,52,370,1012,1224,843,539,71,-351,-745,-1250,-1575,-1970,-2324,-2262,-1784,-1724,-1087,-583,-146,688,1391,1166,916,440,294,152,-496,-1017,-1551,-1623,-2248,-2533,-2319,-1776,-1411,-484,75,732,669,884,1059,762,204,-458,-712,-1094,-1298,-1829,-2322,-2590,-2538,-2330,-1359,-517,28,945,1037,1006,673,509,548,24,-672,-995,-1354,-1635,-2318,-2727,-2652,-2056,-1632,-729,224,714,1179,1488,1389,663,367,286,-308,-724,-1126,-1504,-2073,-2234,-2129,-2143,-1458,-811,-77,648,1098,1180,973,837,566,-232,-586,-1009,-1323,-1609,-2146,-2414,-2400,-1923,-1423,-603,5,306,976,1295,1074,834,372,66,-403,-546,-686,-1203,-2076,-2595,-2641,-2315,-1670,-1072,-172,648,1322,1357,1103,800,704,102,-408,-583,-779,-1289,-1594,-2119,-2308,-2265,-1562,-815,-187,310,816,874,880,967,629,382,-214,-206,-532,-1273,-1554,-2087,-2308,-1891,-1419,-918,-162,579,644,1114,1091,909,912,451,-242,-662,-1232,-1516,-1803,-2197,-2237,-2259,-1980,-1294,-724,212,891,1232,1281,1405,1068,377,-221,-314,-628,-1238,-1814,-2089,-2233,-2226,-1855,-1162,-424,279,744,1180,1028,1161,1257,632,75,-171,-531,-1155,-1630,-2097,-2482,-2301,-1997,-1447,-410,517,821,1110,1015,937,904,612,88,-562,-1420,-1210,-1785,-2361,-2384,-2303,-1963,-1462,-792,371,868,1094,1006,1463,962,392,324,-203,-1040,-1442,-1666,-2179,-2321,-2106,-2191,-1587,-1060,-92,492,730,717,783,485,596,163,-144,-493,-1110,-1636,-2271,-2650,-2521,-2138,-1362,-316,258,371,926,1165,1328,1180,449,206};

void app_main(void)
{
	esp_err_t err;
	uint16_t numBytesWrite;
	char data[16] = {0};

	io_after_reset();

	UART0init(115200);
    
    tmr = xTimerCreate("Tick Timer", pdMS_TO_TICKS(10), pdTRUE, ( void * )blink_id, &tickTimerCallback);
    if( xTimerStart(tmr, 0 ) != pdPASS ) {
     printf("Tick Timer start error");
    }

 // LUK
 /*   
 	gpio_reset_pin(FLASH_GPIO);
	gpio_set_direction(FLASH_GPIO, GPIO_MODE_INPUT);

	Test_MICROPHONE_due();
	
	ADS131M0xinit(CSADC_GPIO, DRDY_GPIO, SYNC_GPIO) ;
	while(gpio_get_level(FLASH_GPIO));

	WAVWRITERinit();

	WAVWRITERcreateFile("test.wav", 8000);
	WAVWRITERwriteFile(buffer, 1024);
	WAVWRITERcloseFile();

	while(1);
 */// END LUK
    USRmain();
}


