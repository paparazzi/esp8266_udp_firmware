/* Configure the network you want to connect to. The ESP8266 acts as a client */
const char *ssid = "yourssid";
const char *password = "yourpassword";

/* The broadcast IP, can be obtained with the ifconfig command in Linux */
IPAddress broadcastIP(10,42,0,255);

/* Port config; 4243 and 4242 are default for pprz udp */
unsigned int localPort = 4243; // port to listen on
unsigned int txPort    = 4242; // port to transmit data on

