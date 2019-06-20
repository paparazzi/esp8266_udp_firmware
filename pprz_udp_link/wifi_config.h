/* Here you can configure how or to what the network you want to connect to.
 * Also the Serial Datarate and which PPRZ Link version you wan to use */

//#define WIFI_MODE WifiModeClient; // Uncomment this line and reflash the module to connect to a router
/* TIP: The broadcast IP, can be obtained with the ifconfig command in Linux */
IPAddress broadcastIP(192,168,1,255);// Only of use in WifiModeClient
//----------------------------------------
#ifndef WIFI_MODE
#define WIFI_MODE WifiModeAccessPoint;
const char *ssid = "Trashcan"; //Change to whatever you fancy...
const char *password = "1234567890"; // In AccessPoint Mode, must be AT LEAST 8 characters long
#else
const char *ssid = "OpenWRT"; //The SSID must match that of your routers SSID when using WifiModeClient
const char *password = "admin";
#endif

#define SERIAL_BAUD_RATE 115200
//#define SERIAL_BAUD_RATE 921600 //The need for speed...

/* If one want to use PPRZLink v1 uncomnent this line below */
//#define PPRZLINK_1

/* Port config; 4243 and 4242 are default for pprz udp */
unsigned int localPort = 4243; // port to listen on
unsigned int txPort    = 4242; // port to transmit data on

/* To enable Over The Air (OTA) update, uncommet the line below */
//#define ENABLE_OTA_UPDATE //WIP: when commented out, no over the air update possible

/* To update your firmware Over The Air (OTA), then use this update command on
 * your local machine:
 *
    python ~/.arduino15/packages/esp8266/hardware/esp8266/2.1.0/tools/espota.py -i esp-module.local -r -f /home/$USER/git/esp8266_udp_firmware/pprz_udp_link/pprz_udp_link.cpp.generic.bin

 if you have a newer Esspressif IFD replace 2.1.0 with something else. Note that your bin file can reside in some other directory too
*/

// For the range sensor

// Uncomment this line to use long range mode. This
// increases the sensitivity of the sensor and extends its
// potential range, but increases the likelihood of getting
// an inaccurate reading because of reflections from objects
// other than the intended target. It works best in dark
// conditions.

//#define LONG_RANGE


// Uncomment ONE of these two lines to get
// - higher speed at the cost of lower accuracy OR
// - higher accuracy at the cost of lower speed

//#define HIGH_SPEED
//#define HIGH_ACCURACY
