/* Configure the network you want to connect to. */
#define WIFI_MODE WifiModeAccessPoint;  // Either WifiModeClient or WifiModeAccessPoint
//const char *ssid = "OpenWrt";
//const char *ssid = "STEREO1";
//const char *ssid = "CX10_NR1";
const char *ssid = "Pats";
//const char *password = ""; // In AccessPoint Mode, must be AT LEAST 8 characters long.
//const char *password = "1234567890"; // In AccessPoint Mode, must be AT LEAST 8 characters long.
const char *password = "BamBifBoem"; // In AccessPoint Mode, must be AT LEAST 8 characters long.

#define SERIAL_BAUD_RATE 115200
//#define SERIAL_BAUD_RATE 921600

/* The broadcast IP, can be obtained with the ifconfig command in Linux */
IPAddress broadcastIP(192,168,1,255); // Only required in WifiModeClient

/* Port config; 4243 and 4242 are default for pprz udp */
unsigned int localPort = 4243; // port to listen on
unsigned int txPort    = 4242; // port to transmit data on


/* Over The Air (OTA) update command */
/*
python ~/.arduino15/packages/esp8266/hardware/esp8266/2.1.0/tools/espota.py -i esp-module.local -r -f /home/bart/git/esp8266_udp_firmware/pprz_udp_link/pprz_udp_link.cpp.generic.bin
*/
