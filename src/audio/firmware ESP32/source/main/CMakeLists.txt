idf_component_register(SRCS 
                    "Bios/adc.c"
                    "Bios/cutmain.c"
                    "Bios/spi0.c"    
                    "Bios/spi1.c"
                    "Bios/timer.c"                    
                    "Bios/uart0.c"
                    "Bios/utils.c"
                    
                    "Drivers/ADS131M0x.c"
                    "Drivers/driver_utils.c"
                    "Drivers/sdcard.c"
                    "Drivers/wifi.c"
                    "Drivers/wav_file/WAVFileWriter.c"
                                    
					"Test/test.c"
					"Test/usr_main.c"
                    "Test/usr_utils.c"    
                    
                    "VirtualLcd/virtuallcd.c"                        
             
					INCLUDE_DIRS "." "Bios" "Drivers" "Drivers/wav_file" "Test" "VirtualLcd")
