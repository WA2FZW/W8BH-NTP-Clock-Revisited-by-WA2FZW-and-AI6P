/*
 *	ESP32_Cheap_Yellow_Display.h - Modified by WA2FZW 07/07/2025
 *
 *		This is a stripped down user setup file for the TFT_eSPI library
 *		for the 'Cheap Yellow Display' available at Amazon and other places.
 *
 *		I've stripped out all the stuff not needed for the Cheap Yellow Display.
 *
 *		Working with Glenn (VK3PE), we discovered that all Cheap Yellow Displays
 *		are not the same. Glenn had some that had 1/4 of the screen rotated 90
 *		degrees from the correct orientation and some on which the colors were
 *		inverted.
 *
 *		If your display has the problem mentioned above, try using the 'ILI9341_2_DRIVER'
 *		instead of the 'ILI9341_DRIVER'.
 */

	#define ILI9341_DRIVER				// The display driver chip is the ILI9341
//	#define ILI9341_2_DRIVER


/*
 *	If the colors are inverted, put the following line in the initialization section
 *	of your code:
 *
 *		tft.invertDisplay (1);
 *
 *	If that doesn't fix the problem, you can try uncommenting one of the following
 *	definitions:
 */

//  #define TFT_RGB_ORDER TFT_RGB		// Color order Red-Green-Blue
//	#define TFT_RGB_ORDER TFT_BGR		// Color order Blue-Green-Red



/*
 *	The Cheap Yellow Display has the capability to adjust the backlight brightness.
 *	This capability is not used in the NTP clock software, but if the following
 *	two symbols are not defined, the backlight will be turned off at startup:
 */

	#define TFT_BL   21					// LED back-light control pin
	#define TFT_BACKLIGHT_ON HIGH		// Level to turn ON back-light (HIGH or LOW)


/*
 *	These are the ESP32 GPIO pins used to communicate with the display:
 */

	#define TFT_MISO 12					// For reading data from the display (not used)
	#define TFT_MOSI 13					// Data going to the display
	#define TFT_SCLK 14					// Data clocking
	#define TFT_CS   15					// Chip select control pin
	#define TFT_DC    2					// Data Command control pin
	#define TFT_RST  -1					// The display RESET is connected to ESP32 board RST


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

/*
 *	The ESP32 has 2 free SPI ports i.e. VSPI and HSPI, the VSPI is the default.
 *	If the VSPI port is in use and pins are not accessible (e.g. TTGO T-Beam)
 *	then uncomment the following line:
 */

	#define USE_HSPI_PORT
