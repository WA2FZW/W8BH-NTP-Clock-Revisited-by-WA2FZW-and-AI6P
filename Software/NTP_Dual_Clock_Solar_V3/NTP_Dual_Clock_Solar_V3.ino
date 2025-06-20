/*
 *	Title:		NTP Dual Clock
 *	Author:		Bruce E. Hall, w8bh.net (Original Version)
 *	Date:		13 Feb 2021
 *	Hardware:	ESP32 (tested on WROOM & WROVER) or ESP8266, ILI9341 TFT display
 *	Software:	Arduino IDE 1.8.13 with Expressif ESP32 and/or ESP8266 tool package 
 *				TFT_eSPI Library version 2.5.43.
 *				ezTime Library
 *	Legal:		Copyright (c) 2021 Bruce E. Hall.
 *				Open Source under the terms of the MIT License. 
 *
 * Description:	Dual UTC/Local NTP Clock with TFT display. Time is refreshed via
 *				NTP every 30 minutes. Optional time output to serial port.
 *				Status indicator for time freshness & WiFi strength.
 *
 *				Before using, please update WIFI_SSID and WIFI_PWD
 *				with your personal WiFi credentials (now in 'UserSettings.h').
 *				Also, modify TZ_RULE with your own Posix timezone string (some
 *				choices in 'UserSettings.h').
 *
 *				see w8bh.net for a detailled, step-by-step tutorial
 *
 *	History:	11/27/20	Initial GitHub commit
 *				11/28/20	Added code to handle dropped WiFi connection
 *				11/30/20	ShowTimeDate() mod by John Price (WA2FZW)
 *				12/01/20	ShowAMPM() added by John Price (WA2FZW)
 *				02/05/21	Added support for ESP8266 modules
 *				02/07/21	Added day-above-month option
 *				02/10/21	Added date-leading-zero option
 *
 *				04/13/25	Added solar weather data to display - rkincaid/CalQRP Club.
 *							Thank you N0NBH and hamqsl.com for providing this service.
 *
 *				05/06/25	Modified by AI6P & WA2FZW to enable full secure connection
 *							to the 'hamqsl' server and made the code work on both
 *							the ESP32 and ESP8266 processors.
 *
 *				05/11/25	Complete overhaul by WA2FZW and Ai6P. Added the ability
 *							to show a user defined sequence of items from the solar
 *							data XML file in the UTC time header block.
 *
 *							Eliminated some unused functions and moved some code
 *							around that was not in the best places.
 *
 *							Allow the user to define more than one WiFi network
 *							to try connecting to.
 */


/*
 *	Libraries and other header files needed for either processor:
 */

#include <TFT_eSPI.h>			// https://github.com/Bodmer/TFT_eSPI
#include <ezTime.h>				// https://github.com/ropg/ezTime
#include <WiFiClientSecure.h>	// Actually different versions for the two processors
#include "UserSettings.h"		// User customizable settings
#include "Certificate.h"		// The hamqsl SSL certificate


/*
 *	Note, it is critical that the 'User_Setups' in the 'TFT_eSPI' library are
 *	configured properly for the GPIO pin assignments you are using. Those
 *	assignments are different for the CalQRP ESP8266 PCB and my ESP32 PCB.
 *	The details for how to accomplish this are included in the 'TFT_eSPI'
 *	documentation (and/or in the header files themselves).
 */

/*
 *	The ESP32 and ESP8266 need different libraries to establish the secure
 *	connection to the server:
 */

#if defined(ESP32)

	#include <HTTPClient.h>
	#include <WiFi.h>

#elif defined(ESP8266)

	#include <ESP8266HTTPClient.h>
	#include <ESP8266WiFi.h>
	X509List cert ( HQSL_Root_Cert );	// Make certificate a list for the API

#endif

#define NTP_SERVER "pool.ntp.org"						// Where we get the time information
#define SW_URL "https://www.hamqsl.com/solarxml.php"	// hamqsl provides the solar data


/*
 *	Not exactly what these do:
 */

#define DEBUGLEVEL	ERROR					// NONE, ERROR, INFO, or DEBUG
#define TIME_FORMAT	COOKIE					// COOKIE, ISO8601, RFC822, RFC850, RFC3339, RSS


/*
 *	'ELEMENTS' is a clever macro that gives you the number of items in an
 *	array regardless of the size/type of elements in that array.
 */

#define ELEMENTS(x) ( sizeof ( x ) / sizeof ( x[0] ))


/*
 *	If we don't get timely updates from the NTP time server, the WiFi indicator
 *	at the top right of the display changes colors; when all is well, it is green.
 *	The numbers are in seconds:
 */

#define SYNC_MARGINAL	 3600					// Orange status if no sync for 1 hour
#define SYNC_LOST		86400					// Red status if no sync for 1 day


/*
 *	The following 'typedef' is used in building the list of functions that
 *	will display the different data items in the UTC header block. All the
 *	display functions have to be of the form 'void fcn ()'. The elements of
 *	the list are built by looking at settings in the 'UserSettings.h' file
 *	by the 'BuildDataItemList' function.
 */

typedef void (*function) ();

function dataItems[DATA_ITEMS];			// List of pointers to display functions
int16_t dataIndex = 0;					// Index into 'dataItems' array

TFT_eSPI tft = TFT_eSPI();				// Create the display object
Timezone local;							// Local timezone variable

time_t t,  oldT;						// Current & displayed UTC time
time_t lt, oldLt;						// Current & displayed local time

bool useLocalTime = false;				// Temp flag used for display updates

String xmlData = "";					// Holds the XML data from 'hamqsl.com'		


/*
 *	The 'setup' function builds the list of solar data items to be displayed,
 *	initializes the display, serial monitor and a few other things.
 */

void setup ()
{
	BuildDataItemList ();					// Build list of solar data to display

	tft.init ();							// Initialize TFT screen object
	tft.setRotation ( SCREEN_ORIENTATION );	// Landscape screen orientation

	ShowSplash ();							// Shows the credits
	delay ( 5000 );							// Time to read it (5 seconds)
	StartupScreen ();						// Mostly blank for connection statuses

	Serial.begin ( BAUDRATE );				// Start the serial port
	delay ( 1000 );							// Allow time to initialize

	setDebug ( DEBUGLEVEL );				// Enable NTP debug level
	setServer ( NTP_SERVER );				// Set NTP server URL

	ShowConnectionProgress ();				// Connect to the WiFi and NTP server

	local.setPosix ( TZ_RULE );				// Set local time zone by rule
	configTime ( 0, 0, NTP_SERVER );		// Needed for https - why? (also timezone doesn't matter here)

	NewDualScreen ();						// Show title & labels
}											// End of 'setup'


/*
 *	The 'loop' function runs forever. It gets periodic time updates from the
 *	NTP time server and whenever the time changes, it updates the display.
 */

void loop ()
{
	events ();								// Get periodic NTP updates

	t = now ();								// Get latest UTC time

	if ( t != oldT )						// Did it change (new second)?
	{
		UpdateDisplay ();					// Update clock every second
		ShowNextData ();					// Show selected solar data
		oldT = t;							// Displayed time is current time
	}
}


/*
 *	Setup Functions; the following functions are primarily used in the clock
 *	start up, although a couple of them are also used during normal operation.
 *
 *	'BuildDataItemList' looks at the 'SHOW_xxx' definitions in the 'UserSettings.h'
 *	file and for those that are non-zero, adds pointers to the functions to display
 *	them in the proper slots in the 'dataItems' array.
 *
 *	Instructions for selecting which items to display are in the 'UserSettings.h'
 *	file. It's kind of brute force, but it works!
 */

void BuildDataItemList ()
{
	if ( DATA_ITEMS == 0 )						// User doesn't want any solar data
		return;

	if ( SHOW_SFI )								// Display SFI, 'A' and 'K'?
		dataItems[SHOW_SFI - 1] = &ShowSFI;		// Add it to the list

	if ( SHOW_GMF )								// Display GMF?
		dataItems[SHOW_GMF - 1] = &ShowGMF;		// Add that to the list

	if ( SHOW_S2N )								// Display signal to noise?
		dataItems[SHOW_S2N - 1] = &ShowS2N;		// Add that to the list

	if ( SHOW_AUR )								// Display Aurora level?
		dataItems[SHOW_AUR - 1] = &ShowAUR;		// Add that to the list

	if ( SHOW_SSN )								// Display Aurora level?
		dataItems[SHOW_SSN - 1] = &ShowSSN;		// Add that to the list
}


/*
 *	'ShowSplash' displays the program title and the author cedits. 
 */

void ShowSplash ( void )
{
	tft.fillScreen ( TFT_BLACK );						// Start with empty screen
	
	tft.setTextDatum ( TC_DATUM );						// Top center text position datum

	tft.setFreeFont  ( &FreeSerifBoldItalic18pt7b );	// Title font
	tft.setTextColor ( TFT_MAGENTA );					// and color
	tft.drawString	 ( "W8BH NTP Clock", 160, 20 );		// Paint the title

	tft.setFreeFont  ( &FreeSansBold9pt7b );			// Font for the credits
	
	tft.drawString	 ("Version 3.0", 160, 60);			// Show the version
	
	tft.setTextDatum ( TL_DATUM );						// Back to default top left
	tft.setTextColor ( TFT_WHITE );						// Back to white

	tft.drawString ( "Originally by: W8BH", 30, 100 );			// Display
	tft.drawString ( "Modified by: WA2FZW & AI6P", 30, 130 );	// the credits
	tft.drawString ( "Solar data from: N0NBH", 30, 160 );

	tft.setFreeFont  ( NULL );							// Select default font
}


/*
 *	'StartupScreen' paints a mostly empty screen with just the 'TITLE' at the
 *	top. The blank area will be filled with the WiFi connection status and NTP
 *	connection status.
 */

void StartupScreen()
{
	tft.fillScreen ( TFT_BLACK );							// Start with empty screen
	tft.fillRoundRect ( 0, 0, 319, 32, 10, LABEL_BGCOLOR );	// Title block
	tft.drawRoundRect ( 0, 0, 319, 239, 10, TFT_WHITE );	// Draw screen edge
	tft.setTextColor ( LABEL_FGCOLOR, LABEL_BGCOLOR );		// Set label colors
	tft.drawCentreString ( TITLE, 160, 6, 4 );				// Show the 'TITLE'
	tft.setTextColor ( LABEL_FGCOLOR, TFT_BLACK );			// Set text color
}	


/*
 *	'ShowConnectionProgress' was completely overhauled in Version 3.0. We now
 *	allow the user to define a list of WiFi networks that will be tried in order
 *	until a connection is established.
 *
 *	We try each one 10 times and if we don't get a connection, we move onto the
 *	next one. If we don't get a connection on any of them, we start over with 
 *	the first one. Instructions on how to define the networks can be found in
 *	the 'UserSettings.h' file.
 */

void ShowConnectionProgress() 
{
	int16_t tries = 0;								// How many connection tries
	int16_t index = 0;								// Index to list of networks
	int16_t count = 0;								// Loop counter

	int16_t networks = ELEMENTS ( ssid_pwd );		// Number of  networks in the list

	bool connected = false;							// Turns true when we connect

	StartupScreen ();

	tft.setFreeFont  ( &FreeSansBold9pt7b );		// Easier to read than the default


/*
 *	First, we need to make sure there is at least one WiFi network in the list.
 *	If not we flash an error message on the screen forever!
 */

	while ( networks == 0 )							// No networks in the list!
	{
		tft.setTextColor ( TFT_RED, TFT_BLACK );
		tft.drawString ( "NO WIFI NETWORKS DEFINED!", 25, 100 );
		delay ( 1000 );

		tft.setTextColor ( TFT_WHITE, TFT_BLACK );
		tft.drawString ( "NO WIFI NETWORKS DEFINED!", 25, 100 );
		delay ( 1000 );
	}


/*
 *	OK, there is at least one network in the list so let's try to conect.
 */

	while ( true )									// Cycle through the list until
	{												// we get a good connection
		WiFi.begin ( ssid_pwd[index].ssid,
					 ssid_pwd[index].pwd );			// Start next one in the list
		tft.drawString ( "Connecting to:", 5, 50 );		// Show we are trying
		tft.fillRect ( 5, 70, 310, 30, TFT_BLACK );		// Erase any previous one
		tft.drawString ( ssid_pwd[index].ssid, 5, 70 );	// Show which one we're trying
		
		for ( count = 0; count < 10; count++ )			// Try each one 10 times
		{
			if ( WiFi.status() != WL_CONNECTED )		// While waiting for connection
			{   
				tft.drawString ( "    ", 249, 70 );		// Erase previous counter
				tft.drawNumber ( tries + 1, 275, 70 );	// and display the current count!
				tries++;								// Increment try counter
				delay ( 1000 );							// Wait a second
			}

			else										// We got a connection!
			{
				tft.fillRect ( 5, 70, 310, 30, TFT_BLACK );		// Erase 2nd line
				tft.drawString ( "Connected to: ", 5, 70 );		// Connected to LAN now
				tft.drawString ( ssid_pwd[index].ssid, 5, 90 );	// so show network name
				connected = true;
			}
		}

		if ( connected )							// Connected so we can stop trying
			break;									// Break out of the 'for' loop

		index = ( index + 1 ) % networks;			// Move onto the next network

		if ( networks != 1 )						// If more than one in the list
			tries = 0;								// reset the tries counter
	}

	tries = 0;										// Now counter for NTP tries

	tft.drawString ( "Waiting for NTP", 5, 130 );	// Now get NTP info

	while ( timeStatus() != timeSet )				// Wait until time retrieved
	{              
		events();									// Allow ezTime to work
		tft.drawString ( "    ", 229, 100 );		// Erase previous counter
		tft.drawNumber ( tries + 1, 230, 130 );		// Show we are trying
		tries++;									// Increment try counter
		delay ( 1000 );								// Wait a second
	}

	tft.drawString ( "NTP Time Received", 5, 150 );	// Show we got the time
	delay ( 2000 );									// Time to read the screen
	tft.setFreeFont  ( NULL );						// Reset to default font
}


/*
 *	'NewDualScreen' paints all the fixed stuff on the display; time heading
 *	blocks, time labels, 'TITLE', etc.
 */

void NewDualScreen ()										// Displays the fixed parts
{
	tft.fillScreen ( TFT_BLACK );							// Start with empty screen
	tft.fillRoundRect ( 0, 0, 319, 33, 10, LABEL_BGCOLOR );	// Title bar for local time
	tft.fillRoundRect (0, 126, 319, 34, 10, LABEL_BGCOLOR );// Title bar for UTC
	tft.setTextColor ( LABEL_FGCOLOR, LABEL_BGCOLOR );		// Set label colors
	tft.drawCentreString ( TITLE, 160, 6, 4 );				// Show title at top
	tft.drawRoundRect ( 0, 0, 319, 110, 10, TFT_WHITE );	// Draw edge around local time
	tft.drawRoundRect ( 0, 126, 319, 110, 10, TFT_WHITE );	// Draw edge around UTC
}															// End of NewDualScreen


/*
 * 	Display functions. The following functions update various fields on the
 * 	clock (except the solar data related ones which are in a separate section).
 *
 *	'UpdateDisplay' gets called once a second from the main loop and calls
 *	all the other funtions that update everything on the display.
 */

void UpdateDisplay ()
{
	lt = local.now ();								// Get local time
	useLocalTime = true;							// Use local timezone
	ShowTimeDate ( lt, oldLt,
				LOCAL_FORMAT_12HR, 10, 46 );		// Show new local time

	useLocalTime = false;							// Now use UTC
	ShowTimeDate ( t, oldT,
				UTC_FORMAT_12HR, 10, 172 );			// Show new UTC time

	ShowClockStatus();								// And clock status
	oldLt = lt;										// and current local time


/*
 *	Get solar data first time or every hour at a random time shortly after
 *	the half hour which is when most of the data is updated.
 */

	GetSolarData();								// Get new info
}												// End of 'UpdateDisplay'


/*
 *	'ShowClockStatus' checks the WiFi connection and changes the color of the
 *	status circle in the local time header when the connection is lost for
 *	some time.
 *
 *	Modified by WA2FZW in Version 3.0:
 *
 *		In previous versions, the WiFi signal strength was shown as the
 *		negative of the RSSI (Received Signal Strength Indicator). This
 *		was misleading as the number was always positive and one would
 *		think a larger number would be better, when in fact, the higher
 *		the number, the weaker the signal.
 *
 *		It now displays the actual signal strength in dBm.
 *
 *	Note at startup it may take a few seconds for the status to be displayed
 *	as it is only checked every 10 seconds.
 */

void ShowClockStatus ()
{
	const int16_t x = 257, y = 3, w = 59, h = 27;	// Position and size of the rectangle
	int16_t	fontSz = 2;								// Font size
	int16_t color;									// Color of the rectangle
	int16_t	wifiSignal;								// Integer signal strength
	String rssi ="";								// ASCII signal strength

	if ( second () % 10 )				// If the remainder of seconds / 10 not zero
		return;							// do nothing; i.e. only execute every 10 seconds

	if ( WiFi.status() != WL_CONNECTED )			// If WiFi connection lost
	{
		tft.setFreeFont  ( &FreeSansBold9pt7b );	// Easier to read than the default
		WiFi.disconnect ();							// and drop current connection
		StartupScreen ();							// Erase most of the screen

		for ( int8_t i = 0; i < 5; i++ )			// Flash the error message
		{
			tft.setTextColor ( TFT_RED, TFT_BLACK );
			tft.drawString ( "LOST WIFI CONNECTION!", 45, 100 );
			delay ( 1000 );

			tft.setTextColor ( TFT_WHITE, TFT_BLACK );
			tft.drawString ( "LOST WIFI CONNECTION!", 45, 100 );
			delay ( 1000 );
		}
		
		ESP.restart ();									// Just start all over!
	}

	int16_t syncAge = now () - lastNtpUpdateTime ();	// how long has it been since last sync?

	if ( syncAge < SYNC_MARGINAL )					// GREEN: time is good & in sync
		color = TFT_GREEN;

	else if ( syncAge < SYNC_LOST )					// ORANGE: sync is 1-24 hours old
		color = TFT_ORANGE;

	else color = TFT_RED;							// RED: time is stale, over 24 hrs old

	tft.fillRoundRect ( x, y, w, h, 6, color );		// Show WiFi status as a color
	tft.setTextColor ( TFT_BLACK, color );

	wifiSignal = WiFi.RSSI ();						// Read signal strength
	
	if ( wifiSignal < -99 )							// Limit to 2 digit value
		wifiSignal = -99;							// for horizontal space limits

	rssi = wifiSignal;								// Assemble ASCII answer
	rssi += " dBm";

	tft.drawString ( rssi, x+6, y+6, fontSz );		// Display it

}													// End of 'ShowClockStatus'


/*
 *	Modified by John Price (WA2FZW)
 *
 *		In the original code, this was an empty function. I added code to display either
 *		an 'A' or 'P' to the right of the local time. Later changed it to show 'AM'
 *		or 'PM'.
 */

void ShowAMPM ( int16_t hr, int16_t x, int16_t y )
{
	char ampm;									// Will be either 'A' or 'P'

	if ( hr <= 11 )								// If the hour is 11 or less
		ampm = 'A';								// It's morning
	else										// Otherwise,
		ampm = 'P';								// it must be afternoon (DOH!)

	tft.drawChar ( ampm, x + 2, y - 12, 4 );	// Show 'A' or 'P'
	tft.drawChar ( 'M', x, y + 12, 4 );			// And 'M'
}												// End of 'ShowAMPM'


void ShowTime ( time_t t, bool hr12, int16_t x, int16_t y )
{
	const int16_t fontSz = 7;						// Font size
	tft.setTextColor ( TIMECOLOR, TFT_BLACK );		// Set time color

	int16_t h = hour ( t );						// Get hours, minutes, and seconds
	int16_t m = minute ( t );
	int16_t s = second ( t );

	if ( hr12 )										// If using 12hr time format,
	{
		if ( DISPLAY_AMPM )							// If showing AM/PM
			ShowAMPM ( h, x + 220, y + 14 );		// Show it
 
		if ( h == 0 )								// 00:00 becomes 12:00
			h = 12;

		if ( h > 12 )								// 13:00 becomes 01:00
			h -= 12;
	}

	if ( h < 10 )										// Is hour a single digit?
	{
		if (( !hr12 ) || ( HOUR_LEADING_ZERO ))			// 24hr format: always use leading 0
			x += tft.drawChar ( '0', x, y, fontSz );	// Show leading zero for hours

		else
		{
			tft.setTextColor ( TFT_BLACK, TFT_BLACK );	// Black on black text
			x += tft.drawChar ( '8', x, y, fontSz );	// Will erase the old digit
			tft.setTextColor ( TIMECOLOR, TFT_BLACK );	// Reset proper color
		}
	}

	x += tft.drawNumber ( h, x, y, fontSz );			// Show hours
	x += tft.drawChar ( ':', x, y, fontSz );			// Show ':'

	if ( m < 10)										// Single digit minutes?
		x += tft.drawChar ( '0', x, y, fontSz );		// Always a leading zero for minutes

	x += tft.drawNumber ( m, x, y, fontSz );			// Show minutes
	x += tft.drawChar ( ':', x, y, fontSz );			// Another ':'

	if ( s < 10 )										// Single digit seconds?
		x += tft.drawChar ( '0', x, y, fontSz );		// Always a leading zero for seconds

	x += tft.drawNumber ( s, x, y, fontSz );			// Show seconds
}														// End of ShowTIme


void ShowDate ( time_t t, int16_t x, int16_t y )
{
	const int16_t fontSz = 4;							// Font size
	const int16_t yspacing = 30;						// Vertical spacing
	const char* months[] = { "JAN", "FEB", "MAR",		// Should be obvious!
							 "APR", "MAY", "JUN",
							 "JUL", "AUG", "SEP",
							 "OCT", "NOV", "DEC" };

	int16_t i = 0;										// ???
	int16_t m = month ( t ) - 1;						// Index to above array
	int16_t d = day ( t );								// Just a number

	tft.setTextColor ( DATECOLOR, TFT_BLACK );			// Set proper colors
	tft.fillRect ( x, y, 50, 60, TFT_BLACK );			// Erase previous date

	if ( DATE_ABOVE_MONTH )								// Show date on top?
	{
		if (( DATE_LEADING_ZERO ) && ( d < 10 ))		// Do we need a leading zero?
			i = tft.drawNumber ( 0, x, y, fontSz );		// Draw leading zero

		tft.drawNumber ( d, x + i, y, fontSz );			// Draw date
		y += yspacing;									// Y position for month
		tft.drawString ( months[m], x, y, fontSz );		// Draw month
	}

	else												// Month goes on top		
	{
		tft.drawString ( months[m], x, y, fontSz );		// Draw month
		y += yspacing;									// Vertical space for day

		if (( DATE_LEADING_ZERO ) && ( d < 10 ))		// Do we need a leading zero?
			x += tft.drawNumber ( 0, x, y, fontSz );	// Yep, draw it

		tft.drawNumber ( d, x, y, fontSz );				// Draw date
	}
}														// End of 'ShowDate'


void ShowTimeZone ( int16_t x, int16_t y )
{
	const int16_t fontSz = 4;							// Font size

	tft.setTextColor ( LABEL_FGCOLOR, LABEL_BGCOLOR );	// Set text colors

	if ( !useLocalTime )
		tft.drawString ( "UTC", x, y + 3, fontSz );		// UTC time
	else
		tft.drawString ( local.getTimezoneName(),
								x, y + 2, fontSz);		// Show local time zone
}														// End of 'ShowTimeZone'


void ShowTimeDate ( time_t t, time_t oldT, bool hr12, int16_t x, int16_t y )
{
	ShowTime ( t, hr12, x, y );							// Display time HH:MM:SS

	if (( !oldT ) || ( hour ( t ) != hour ( oldT )))	// Did hour change?
		ShowTimeZone ( x, y - 42 );						// Yes, update time zone

	if (( !oldT ) || ( day ( t ) != day ( oldT )))		// Did date change?
		ShowDate ( t, x + 250, y );						// Yes, update it
}														// End of 'ShowTimeDate'

										
/*
 *	Currently gives the option to show UTC or local time; why not both?
 */

void PrintTime ()											// Print time to serial port
{
	if ( !PRINTED_TIME )									// Option 0: dont print
		return;

	if ( PRINTED_TIME == 1 )								// Option 1: print UTC time
		Serial.println ( dateTime ( TIME_FORMAT ));

	else
		Serial.println ( local.dateTime ( TIME_FORMAT ));	// Option 2: print local time
}															// End of 'PrintTime'


/*
 *	The following functions are all part of the process of getting the solar
 *	data from hamqsl.com and displaying it on the clock/
 *
 *	'GetSolarData' was added by Robert (AI6P) to grab solar conditions as XML from:
 *	https://www.hamqsl.com/solarxml.php
 *
 *	Doing some testing, it looks like the 'AUR', 'BZ' and 'SSN' values are
 *	updated hourly at about the top of the hour and the 'SFI', 'A', 'K',
 *	'GMF' and 'S2N' values only update every three hours at about the half hour.
 *	I don't know about other data items that are not currently being (optionally)
 *	reported.
 *
 *	Checking with Paul (N0NBH), he informs me that quering the website every 15
 *	minutes would not be a problem, but since it looks like twice an hour would
 *	be adequate, so that's what we will do.
 *
 *	So, we're going to poll the solar data twice per hour shortly after the top
 *	of the hour and shortly after the half hour. We're going to do that slightly
 *	randomly using a modification of the algorithm provided by Robert (AI6P).
 *
 *	The connection is made with HTTPS encryption using a genuine SSL certificate
 *	(see Certificate.h). *IF* hamqsl updates their HTTPS certificate you made
 *	need to update it in the 'Certificate.h' header file.
 *
 *	Notice that the process is slightly different depending on whether the ESP32 or
 *	ESP8266 is being used.
 *
 *	If the connection fails, we will retry it every 5 minutes. Once connected, the
 *	normal polling times will resume.
 */

void GetSolarData()
{
static	int16_t pollMin = 2;					// Or 32 plus up to 4 minutes
static	int16_t pollSec = 0;					// The second at which we'll update

static	int32_t	failTime;						// Time of last failed attempt
static	bool	retry;							// Need to retry after 10 minutes

	if ( xmlData == "" )						// First time or a failure retry
	{											// if no previous data
		pollSec = second ( local.now ());		// We'll poll on this second
		pollMin = 2 + ( pollSec % 5 );			// on a semi random minute
		failTime = 0;							// No failures
		retry = false;							// so no need to retry
	}


/*
 *	If 'retry' is true and more than 5 minutes (300,000 milliseconds), reset the
 *	'failTime' and 'retry' variables and and make 'xmlData' a null string which
 *	will make it look like this is the first time we got here.
 */

	if ( retry && (( millis () - failTime ) > 300000 ))
	{
		failTime = 0;							// Assume no failure
		retry = false;							// so no need to retry
		xmlData = "";							// Erase the data
	}


/*
 *	If it's time to poll 'hamqsl.com' or the first time here or after
 *	a failed attempt, do it.
 *
 *	'pollMin' has been set to somewhere between 2 and 6. In order to poll shortly
 *	after the top of the hour and half hour, see see if the remainder of the
 *	current minute and 'pollMin' divided by 30 are equal; if so, time to poll.
 */

	if (((( minute(t) % 30 ) == ( pollMin % 30 ))
					&& second(t) == pollSec ) || xmlData == "" )
    {
    	Serial.print ( "Connecting to website: " );
    	PrintTime ();
   
		WiFiClientSecure client;					// Set up web connection
		HTTPClient https;

		#if defined ( ESP32 )						// Different for ESP32
			client.setCACert ( HQSL_Root_Cert );	

		#elif defined ( ESP8266 )					// and ESP8266
			client.setTrustAnchors ( &cert );

		#endif

		bool connected = https.begin ( client, SW_URL );	// Open the URL connection

		int16_t httpResponseCode = https.GET ();			// Get the response code


/*
 *	Not sure trying 5 times in rapid succession makes a lot of sense.
 */

		for ( int16_t i = 0; i < 5; ++i )					// Try up to 5 times
		{
			if ( httpResponseCode > 0 )						// If we got a response try to use it
			{
				xmlData = https.getString ();				// Get the XML data
//				Serial.println ( xmlData );					// For debugging

				Serial.print ( "\nSolar data updated: " );
				PrintTime ();

				break;										// OK we're good, don't retry
			}


/*
 *	If we couldn't connect to the web page, we record the failure time and set
 *	the need to 'retry' flag.
 *
 *	We put the string "Missing' into the 'xmlData'. That accomplishes two things.
 *	Because 'xmlData' is not a null string, we won't retry every time the function
 *	is called (which is everytime the time changes). 
 *
 *	Since there is no valid solar data in 'xmlData' all the displayed info will
 *	show '??' indicating we couldn't get the data.
 */

			else											// Connection failed
			{
				Serial.print ( "HTTP Error code: ");		// Print the response code on the
				Serial.println ( httpResponseCode );		// console if something goes wrong
				failTime = millis ();						// Record time of failure
				retry = true;								// and set the 'retry' flag
				xmlData = "Missing";						// No valid data
			}

			delay ( 100 );									// 0.1 second
		}													// End of loop

		https.end ();										// Free resources
	}
}															// End of 'GetSolarData'


/*
 *	'GetXmlData' is a poor man's xml tag extraction function added by Robert (AI6P)
 *	rather than pulling in an entire XML library when it wasn't really that necessary.
 *	It's pretty straightforward. Just use 'indexof' to find the tag locations and pull
 *	out the value with substring.
 *
 *	Note that this only works on the items in the XML data that are in the form:
 *
 *		<sunspots>51</sunspots>
 *
 *	There is data in the XML data that takes different forms.
 */

String GetXmlData ( String xml, String tag )
{
	int16_t i = xml.indexOf ( "<" + tag + ">" );			// Where the beginning tag is
	int16_t j = xml.indexOf ( "</" + tag + ">" );			// and the ending tag

	String val = "";										// Data value

	if ( i > 0 && j > i && j < xml.length ())				// Sanity check
	{
		val = xml.substring (i + tag.length() + 2, j );		// Get the data between the tags
		val.trim ();										// Eliminate whitespace
		return val;
	}

	else
		return "??";					// If we didn't find anything then use this
}										// End of 'GetXmlData'


/*
 *	'ShowNextData' cycles through the list of pointers to the functions that
 *	display the selected items from the 'xmlData' received from 'hamqsl.com'
 *	every 'CYCLE_TIME' seconds.
 *
 *	Instructions on how to establish the list can be found in the 'UserSettings.h'
 *	header file.
 */

void ShowNextData ()
{
	if (( DATA_ITEMS == 0 )						// If nothing to display
				|| ( CYCLE_TIME == 0 ))			// or illegal time setting
		return;									// do nothing

	if (( second ( t ) % CYCLE_TIME ) == 0 )	// Only change every 'CYCLE_TIME' seconds
	{
		dataItems[dataIndex++]();				// Display something
		if ( dataIndex >= DATA_ITEMS )			// Don't exceed maximum number
			dataIndex = 0;						// Reset list index
	}
}												// End of 'ShowNextData'


/*
 *	The following functions display the various items that the user has decided
 *	to show in the heading block for the UTC time.
 *
 *	'ShowSFI' displays the solar flux ('SFI'), and the 'A' and 'K' indicies.
 *
 *	The numbers are color coded to match the colors used in the bar graphs on
 *	the NOAA solar data site, which are different that the colors that Paul
 *	uses on the 'hamqsl.com' website.
 */

void ShowSFI ()
{
	String sflux = GetXmlData ( xmlData, "solarflux" );	// Get the solar flux
	String kindx = GetXmlData ( xmlData, "kindex" );	// Get the K index
	String aindx = GetXmlData ( xmlData, "aindex" );	// Get the A index

	int16_t	sfiInt = sflux.toInt ();				// Need numbers
	int16_t	aInt   = aindx.toInt ();
	int16_t	kInt   = kindx.toInt ();

	String headings = "SFI:           A:           K:   ";	// Header for the data


/*
 *	First, we handle the solar flux index. It will be shown as normal (< 200) or
 *	high (200 or higher). Why did I pick those levels? I don't do much HF operating,
 *	so I don't really know the effect of a high SFI on conditions, but on 6 meters,
 *	when the SFI is greater than 200, there is a good chance of trans-equatorial
 *	(TEP) or F2 propagation.
 *
 *	The breakpoints can be changed in the 'UserSettings.h' file.
 */

	ClearSolarData ();									// Erase previous data

	tft.setTextColor ( LABEL_FGCOLOR, LABEL_BGCOLOR );	// Set label colors
	tft.drawString ( headings, 80, 133, 4 );			// Paint SFI headers

	tft.setTextColor ( COLOR_NORMAL	, LABEL_BGCOLOR );	// Assume normal reading

	if ( sfiInt >= MEDIUM_SFI )								// 175 or greater
		tft.setTextColor ( COLOR_MEDIUM, LABEL_BGCOLOR );	// Make number yellow

	if ( sfiInt >= HIGH_SFI )								// 200 or greater
		tft.setTextColor ( COLOR_HIGH, LABEL_BGCOLOR );		// Make number red

	tft.drawString ( sflux, 125, 134, 4 );					// Paint the number


/*
 *	The NOAA breakpoints for the 'A' index are 20 and 30:
 */

	tft.setTextColor ( COLOR_NORMAL	, LABEL_BGCOLOR );		// Assume normal reading

	if ( aInt >= MEDIUM_A )									// 'A' 20 or higher?
		tft.setTextColor ( COLOR_MEDIUM, LABEL_BGCOLOR );	// Medium level

	if ( aInt >= HIGH_A )									// 30 or higher?
		tft.setTextColor ( COLOR_HIGH, LABEL_BGCOLOR );		// Highest level

	tft.drawString ( aindx, 205, 134, 4 );					// Show 'A'


/*
 *	The NOAA breakpoints for the 'K' index are 4 and 5:
 */

	tft.setTextColor ( COLOR_NORMAL, LABEL_BGCOLOR );		// Assume normal reading

	if ( kInt >= MEDIUM_K )									// 'K' 4 or higher?
		tft.setTextColor ( COLOR_MEDIUM, LABEL_BGCOLOR );	// Medium level

	if ( kInt >= HIGH_K )									// 5 or higher?
		tft.setTextColor ( COLOR_HIGH, LABEL_BGCOLOR );		// Highest level

	tft.drawString ( kindx, 284, 134, 4 );

	tft.setTextColor ( LABEL_FGCOLOR, LABEL_BGCOLOR );		// Set normal label colors
}															// End of 'ShowSFI'


/*
 *	'ShowGMF' displays the Geomagnetic Field conditions. For now, there are
 *	no color codes; I may change that.
 */

void ShowGMF ()
{
	String headings = "GMF:  ";							// Header

	String gmf = GetXmlData ( xmlData, "geomagfield" );

	ClearSolarData ();									// Erase previous data

	tft.setTextColor ( LABEL_FGCOLOR, LABEL_BGCOLOR );	// Set label colors
	tft.drawString ( headings, 80, 133, 4 );			// Paint GMF Header

	tft.setTextColor ( COLOR_NORMAL, LABEL_BGCOLOR );	// Assume normal reading

	tft.drawString ( gmf, 150, 133, 4 );				// Paint the value

	tft.setTextColor ( LABEL_FGCOLOR, LABEL_BGCOLOR );	// Normal label colors
}														// End of 'ShowGMF'


/*
 *	'ShowS2N' displays the dignal to noise level apparantely in 'S' units. Again
 *	I have not added color coding here, but might.
 */

void ShowS2N ()											// Signal to noise level
{
	String headings = "S2N:  ";							// Header for signal to noise

	String s2n = GetXmlData ( xmlData, "signalnoise" );

	ClearSolarData ();									// Erase previous data

	tft.setTextColor ( LABEL_FGCOLOR, LABEL_BGCOLOR );	// Set label colors
	tft.drawString ( headings, 80, 133, 4 );			// Paint S2N Header

	tft.setTextColor ( COLOR_NORMAL	, LABEL_BGCOLOR );	// Assume normal reading

	tft.drawString ( s2n, 150, 133, 4 );				// Paint the value

	tft.setTextColor ( LABEL_FGCOLOR, LABEL_BGCOLOR );	// Normal label colors
}														// End of 'ShowS2N'


/*
 *	'ShowAUR' displays the Aurora level and the 'BZ'. Again I have not added
 *	color coding here, but might will once I have a better understanding of
 *	what the values mean.
 */

void ShowAUR ()											// Aurora level
{
	String headings = "AUR:          BZ:";				// Header for Aurora & BZ

	String aur = GetXmlData ( xmlData, "aurora" );
	String bz  = GetXmlData ( xmlData, "magneticfield" );

	ClearSolarData ();									// Erase previous data

	tft.setTextColor ( LABEL_FGCOLOR, LABEL_BGCOLOR );	// Set label colors
	tft.drawString ( headings, 80, 133, 4 );			// Paint AUR Header

	tft.setTextColor ( COLOR_NORMAL	, LABEL_BGCOLOR );	// Assume normal reading

	tft.drawString ( aur, 150, 133, 4 );				// Paint the AUR value
	tft.drawString ( bz,  235, 133, 4 );				// and the BZ value

	tft.setTextColor ( LABEL_FGCOLOR, LABEL_BGCOLOR );	// Normal label colors
}														// End of 'ShowAUR'


/*
 *	'ShowSSN' displays the current number of sunspots. Again I have not added color
 *	coding here.
 */

void ShowSSN ()											// Sunspot count
{
	String	headings = "SSN:  ";						// Header for sunspot count

	String ssn = GetXmlData ( xmlData, "sunspots" );

	ClearSolarData ();									// Erase previous data

	tft.setTextColor ( LABEL_FGCOLOR, LABEL_BGCOLOR );	// Set label colors
	tft.drawString ( headings, 80, 133, 4 );			// Paint SSN Header

	tft.setTextColor ( COLOR_NORMAL	, LABEL_BGCOLOR );	// Assume normal reading

	tft.drawString ( ssn, 150, 133, 4 );				// Paint the value

	tft.setTextColor ( LABEL_FGCOLOR, LABEL_BGCOLOR );	// Normal label colors
}														// End of 'ShowSSN'


/*
 *	Simple function to erase any previous solar data that was in the UTC
 *	header block. Assumes that the solar data always starts at x = 80.
 */

void ClearSolarData ()
{
	tft.fillRoundRect (80, 126, 319, 32, 10, LABEL_BGCOLOR );	// Title bar for UTC
	tft.drawRoundRect ( 0, 126, 319, 110, 10, TFT_WHITE );		// Draw edge around UTC
}
