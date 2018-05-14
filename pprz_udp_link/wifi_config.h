/* Configure the network you want to connect to. */

#define WIFI_MODE WifiModeAccessPoint;  // Either WifiModeClient or WifiModeAccessPoint
//#define WIFI_MODE WWifiModeClient;    // Enable to connect to a router
//#define WIFI_MODE WWifiModeMesh;      // NIY

//const char *ssid = "OpenWRT"; //Your router SSID when using WifiModeClient
const char *ssid = "MyFlyingThing"; //Change to whatever you fancy...
//const char *password = ""; // In AccessPoint Mode, must be AT LEAST 8 characters long.
const char *password = "1234567890"; // In AccessPoint Mode, must be AT LEAST 8 characters long.

#define SERIAL_BAUD_RATE 115200
//#define SERIAL_BAUD_RATE 921600 //The need for speed...

/* If one want to use PPRZLink v1 uncomnent this line below */
//#define PPRZLINK_1

/* The broadcast IP, can be obtained with the ifconfig command in Linux */
IPAddress broadcastIP(192,168,1,255); // Only required in WifiModeClient

/* Port config; 4243 and 4242 are default for pprz udp */
unsigned int localPort = 4243; // port to listen on
unsigned int txPort    = 4242; // port to transmit data on

/* To enable Over The Air (OTA) update, uncommet the line below */
#define ENABLE_OTA_UPDATE //WIP: when commented out, no over the air update possible

/* To update your firmware Over The Air (OTA), then use this update command on
 * your local machine:
 *
    python ~/.arduino15/packages/esp8266/hardware/esp8266/2.1.0/tools/espota.py -i esp-module.local -r -f /home/$USER/git/esp8266_udp_firmware/pprz_udp_link/pprz_udp_link.cpp.generic.bin

 if you have a newer Esspressif IFD replace 2.1.0 with something else. Note that your bin file can reside in some other directory too
*/
