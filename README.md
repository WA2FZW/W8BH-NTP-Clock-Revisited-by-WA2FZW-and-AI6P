# W8BH-NTP-Clock-Revisited-bu-WA2FZW-and-AI6P

Several years ago, Bruce Hall (W8BH) designed an ESP32 based digital clock that kept time by querying an NTP (Network Time Protocol) server. His original design as I recall was either simply a breadboard or hand wired implementation. I designed a circuit board for it; Bruce has also designed one since then. Bruce’s notes and the Gerber files for my original PCB and the original software can be found at W8BH.net.

Recently, the California QRP Club did a version of Bruce’s clock that runs on an ESP8266. Robert Kincaid (AI6P) modified Bruce’s ESP8266 version of the software to add the capability to download solar data from the ‘hamqsl.com’ website run by Paul Herrman (N0NBH). 

Unfortunately, the approach Robert used to access the data was incompatible with the method required for the ESP32 processor due to differences in the software libraries available for each of the processors.

I spent a couple of weeks working with Robert to figure out how to make the software run on either processor.
Once that problem was solved, I continued to do some cleanup on the software and added the capability for the user to define a number of items from the solar data that could be displayed in a rotating sequence on the clock at a rate chosen by the user;  the user can add other items if desired.

We also added the capability for the user to define multiple WiFi networks, so that if a connection cannot be established on one, we will try others.

As there were a couple of things I didn’t like about the ESP32 PCB that I had originally designed and Bruce’s ESP8266 design, I have designed new hardware for both. 

There are actually three different hardware versions. This document will describe both the new hardware and software.
In addition to this document, the Gerber files for all three versions of the clock and the software is available on Github. The TFT_eSPI setup files are also available there.

Update July 7, 2025:

    Working with Glenn (VK3PE) we discovered that not all Cheap Yellow Displays are the same. I updated
    the 'ESP32_Cheap_Yellow_Display.h' file to include options that resolved the differences we found
    in the different ones.
