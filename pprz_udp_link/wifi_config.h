/* Configure the network you want to connect to. */
#define WIFI_MODE WifiModeClient;  // Either WifiModeClient or WifiModeAccessPoint
const char *ssid = "e540";
const char *password = "LHlqaSgu"; // In AccessPoint Mode, must be AT LEAST 8 characters long.

/* The broadcast IP, can be obtained with the ifconfig command in Linux */
IPAddress broadcastIP(192,168,255,255); // Only required in WifiModeClient

/* Port config; 4243 and 4242 are default for pprz udp */
unsigned int localPort = 4243; // port to listen on
unsigned int txPort    = 4242; // port to transmit data on


/* Over The Air (OTA) update command */
/*
python ~/.arduino15/packages/esp8266/hardware/esp8266/2.1.0/tools/espota.py -i esp-module.local -r -f /home/bart/git/esp8266_udp_firmware/pprz_udp_link/pprz_udp_link.cpp.generic.bin
*/
