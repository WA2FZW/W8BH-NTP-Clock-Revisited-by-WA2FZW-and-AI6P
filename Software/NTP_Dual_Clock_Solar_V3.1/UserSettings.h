#ifndef	_USER_SETTINGS_H_					// Prevent double include
#define	_USER_SETTINGS_H_


/*
 *	'ELEMENTS' is a clever macro that gives you the number of items in an
 *	array regardless of the size/type of elements in that array.
 */

#define ELEMENTS(x) ( sizeof ( x ) / sizeof ( x[0] ))


/*
 *	The 'wifiData' structure holds an SSID and password for a particular
 *	WiFi network. We will allow an array of these so the user can try multiple
 *	networks until we find one that works! Don't mess with this!
 */

struct wifiData {
	String ssid;
	String pwd; };


/*
 *	In Version 3.0, we added the ability to define multiple WiFi networks. Here
 *	you can define as many as you like. On startup, the program will cycle through
 *	them in order until it gets a good connection. YOu MUST have at least the
 *	first one defined, and that should be the one most commonly used.
 *
 *	Edit the SSID and PWD strings in each to provide valid SSIDs and passwords,
 *	and remove the '//' from any you want to use. You can add as many as you like
 *	by just following the models.
 */

wifiData ssid_pwd [] =
{
	"SSID_1", "PWD_1",
//	"SSID_2", "PWD_2",
//	"SSID_3", "PWD_3",
//	"SSID_4", "PWD_4",
//	"SSID_5", "PWD_5",
};


/*
 *	Time Zone rules in "Posix timezone string" format. For an explanation and a list of
 *	the strings appropriate for various locations, see the following web pages:
 *
 *		https://support.cyberdata.net/portal/en/kb/articles/010d63c0cfce3676151e1f2d5442e311
 *		https://developer.ibm.com/articles/au-aix-posix/
 *
 *	The ones listed here are for the US and the folks who have worked on this software:
 *	
 *		"EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00"		// US Eastern time
 *		"CST6CDT,M3.2.0/2:00:00,M11.1.0/2:00:00"		// US Central time
 *		"MST7MDT,M3.2.0/2:00:00,M11.1.0/2:00:00"		// US Mountain time
 *		"MST7"											// Arizona time 
 *		"PST8PDT,M3.2.0/2:00:00,M11.1.0/2:00:00"  		// USA Pacific Time
 *		"AEST-10AEDT,M10.1.0/2:00:00,M4.1.0/2:00:00" 	// Aus Eastern time
 *		"GMT0BST,M3.5.0/1:00:00,M10.5.0/2:00:00"		// UK time
 *
 *	The list is currently set up to alternate between US Eastern time and
 *	Australian Eastern time. You can change the entries by pasting one of the
 *	above strings (including the quote marks) into the list. Note that all
 *	entries except the last one also require a comma at the end.
 *
 *	There must be at least one entry in the list or you will get an error message
 *	at startup.
 *
 *	the definition of TZ_INTERVAL can be changed to a value between 1 and 30, but
 *	should be a number that divides evenly into 60. It defines how long each time
 *	will be displayed before moving onto the next one. It is currently set for 5
 *	seconds.
 */

char timeZones[][50] = {							// List of timezones to be displayed
	"EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00",
	"AEST-10AEDT,M10.1.0/2:00:00,M4.1.0/2:00:00" };

uint8_t tzCount = ELEMENTS ( timeZones );			// How many in the list

#define TZ_INTERVAL 5								// How long each time is displayed


/*
 *	The 'TITLE' is displayed at the top of the screen. You can make it
 *	whatever you like as long as it fits. Coming attraction - eventually
 *	you will have other choices that can be displayed at the top of the
 *	screen.
 */

#define TITLE "NTP CLOCK"				// Anything you like (if it fits)

#define BAUDRATE 115200					// Serial monitor output baudrate

#define SCREEN_ORIENTATION	3			// Screen portrait mode:  use 1 or 3
										// depending on how you mounted the display


/*
 *	These definitions control how the time and date data is displayed. What
 *	each means should be self-explanatory,
 */

#define LOCAL_FORMAT_12HR	true		// Local time format 12hr "11:34" vs 24hr "23:34"
#define UTC_FORMAT_12HR		false		// UTC time format 12 hr "11:34" vs 24hr "23:34"
#define DISPLAY_AMPM		true		// if true, show 'AM' or 'PM'
#define HOUR_LEADING_ZERO	false		// "01:00" vs " 1:00"
#define DATE_LEADING_ZERO	true		// "Feb 07" vs. "Feb 7"
#define DATE_ABOVE_MONTH 	false		// "12 Feb" vs. "Feb 12"

#define PRINTED_TIME		1			// 0 = NONE, 1 = UTC, or 2 = LOCAL


/*
 *	The normal colors of the foregrounds and backgrounds of the stuff
 *	displayed on the screen:
 */

#define TIMECOLOR		TFT_CYAN			// Color of 7-segment time display
#define DATECOLOR		TFT_YELLOW			// Color of displayed month & day
#define LABEL_FGCOLOR	TFT_WHITE			// Color of label text
#define LABEL_BGCOLOR	TFT_BLUE			// Color of label background


/*
 *	More colors; these are used to indicate that various data elements have
 *	exceeded predefined limits. For example, When the 'K' index is 4 or higher,
 *	NOAA shows yellow bars on its graph. When 'K' exceeds 5, NOAA shows red
 *	bars. We will color code the numbers the same!
 *
 *	NOAA website is at: https://www.swpc.noaa.gov/products/station-k-and-indices
 */

#define	COLOR_NORMAL	TFT_GREEN			// Below any threshold
#define	COLOR_MEDIUM	TFT_YELLOW			// Medium level
#define	COLOR_HIGH		TFT_RED				// Maximum


/*
 *	These define the breakpoints for the things that are color coded. Those
 *	for the 'A' and 'K' values correspond to the NOAA breakpoints. The SFI
 *	breakpoints are arbitrary.
 */

#define	MEDIUM_K		  4					// Above 4 it's yellow
#define	HIGH_K			  5					// Above 5, red

#define	MEDIUM_A		 20					// Above 20 it's yellow
#define	HIGH_A			 30					// Above 30, red

#define	MEDIUM_SFI		175					// Above 175 it's yellow
#define	HIGH_SFI		200					// Above 200, red


/*
 *	What bits of data do we want to display? Here you can decide!
 *
 *	It's a bit tricky, so pay attention. For any of the data items that you
 *	do not want to show, assign a value of '0'. For those that you want to
 *	show assign values of '1', '2', etc. in the order that you want them
 *	displayed in. Don't skip any numbers!
 *
 *	Set the value of 'DATA_ITEMS' to the highest value that you assigned.
 *
 *	If you don't want to display any of the items, set them all to '0' and
 *	set the value of 'DATA_ITEMS' to '0'. Failure to do this correctly will
 *	result in the program crashing!
 *
 *	If you want to display things not already in the list, you'll need to add
 *	a symbol and a function to display the additional items; the 'BuildDataItemList'
 *	function will also need to be modified.
 */

#define	SHOW_SFI	1						// Displays SFI, 'A' and 'K'
#define	SHOW_GMF	2						// Displays geomagetic field activity
#define	SHOW_S2N	3						// Display signal to noise
#define	SHOW_AUR	4						// Display Aurora level
#define	SHOW_SSN	5						// Display sunspot count

#define	DATA_ITEMS	5						// How many we are displaying


/*
 *	'CYCLE_TIME' defines how long each of the displayed solar data items will remain
 *	on the screen. The number has to be something that evenly divides into 60 or
 *	one item will get a shorter time. Good choices are 2, 3, 4, 5, 6 and 10. If
 *	you set it to zero, nothing will get displayed.
 */

#define	CYCLE_TIME	2						// Seconds to show each solar data item

#endif
