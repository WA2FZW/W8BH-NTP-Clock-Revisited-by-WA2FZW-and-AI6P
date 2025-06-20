/*
 *	ESP8266_Generic_Setup.h.h - Modified by WA2FZW 06/11/2025
 *
 *		This is a stripped down user setup file for the TFT_eSPI library
 *		for the ESP8266. It has been tested with the ESP8266 NodeMCU module
 *		and the ESP8266 D1 Mini module.
 *
 *		I've stripped out all the stuff not really needed for clarity.
 */


	#define ILI9341_DRIVER				// The display driver chip is the ILI9341

	
/*
 *	The ESP8266s are weird! The GPIO pins designations on the boards are
 *	labeled 'D0', 'D1', etc., however those are not the actual GPIO pin
 *	numbers! The TFT_eSPI library expects the 'Dn' designations.
 *
 *	Using the PCBs I designed for both modules, the display RESET is connected
 *	to the processor's RST pin. The display RESET line could also be connected
 *	to a GPIO pin.
 */

//	#define TFT_MISO  PIN_D6				// Actually GPIO 12 (Not used)
	#define TFT_MOSI  PIN_D7				// Actually GPIO 13
	#define TFT_SCLK  PIN_D5				// Actually GPIO 14
	#define TFT_CS    PIN_D8				// Actually GPIO 15
	#define TFT_DC    PIN_D3				// Actually GPIO  0
//	#define TFT_RST   PIN_D4  				// Actually GPIO  2
	#define TFT_RST  -1						// Display reset connected to processor RST



/*
 *	You can comment out the #defines below with // to stop specific fonts from
 *	being loaded, but the The ESP32 has plenty of memory so commenting out fonts
 *	is not really necessary. If all fonts are loaded the extra FLASH space required
 *	is about 17Kbytes. If you need to save FLASH space only enable the fonts you need!
 */

	#define LOAD_GLCD		// Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
	#define LOAD_FONT2		// Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
	#define LOAD_FONT4		// Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
	#define LOAD_FONT6		// Large 48 pixel font, needs ~2666 bytes in FLASH, only
							// characters 1234567890:-.apm
	#define LOAD_FONT7		// 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only
							// characters 1234567890:-.
	#define LOAD_FONT8		// Large 75 pixel font needs ~3256 bytes in FLASH, only
							// characters 1234567890:-.
	#define LOAD_GFXFF		// FreeFonts. Include access to the 48 Adafruit_GFX free fonts
							// FF1 to FF48 and custom fonts


/*
 *	Comment out the #define below to stop the SPIFFS filing system and smooth font
 *	code being loaded this will save ~20kbytes of FLASH
 */

	#define SMOOTH_FONT


/*
 *	I've found that the 40MHz SPI speed works fine, but you can try other speeds:
 */

//	#define SPI_FREQUENCY	 1000000
//	#define SPI_FREQUENCY	 5000000
//	#define SPI_FREQUENCY	10000000
//	#define SPI_FREQUENCY	20000000
//	#define SPI_FREQUENCY	27000000
	#define SPI_FREQUENCY	40000000
//	#define SPI_FREQUENCY	80000000


/*
 *	The NTP clock doesn't read the display or use the touch screen capabilities,
 *	but I'll leave these defined:
 */

	#define SPI_READ_FREQUENCY	20000000
	#define SPI_TOUCH_FREQUENCY  2500000
