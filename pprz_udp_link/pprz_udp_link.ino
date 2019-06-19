#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include "wifi_config.h"
#ifdef ENABLE_OTA_UPDATE
#include <ArduinoOTA.h>
#endif

#include <Wire.h>
#include <VL53L0X.h> // POLULU library

VL53L0X sensor;

/* Struct with available modes */
enum wifi_modes {
  WifiModeClient,
  WifiModeAccessPoint
} wifi_mode = WIFI_MODE;


/* Fancy blinking while data is transmitted if LED populated */
#define LED_PIN 13

#define PPRZ_STX 0x99

#ifdef PPRZLINK_1_GEC
#error PPRZLINK v1 encryped link is not supported, consider using standard WiFi encryption
#endif // PPRZLINK 1.0 encrypted

#ifdef PPRZLINK_2_GEC
#error PPRZLINK v2 encryped link is not supported, consider using standard WiFi encryption
#endif // PPRZLINK 2.0 encrypted

#ifndef PPRZLINK_1
#define PPRZLINK_2
#endif

#if defined PPRZLINK_1 || defined PPRZLINK_2

/* PPRZ message parser states */
enum normal_parser_states {
  SearchingPPRZ_STX,
  ParsingLength,
  ParsingSenderId,  //Sidenote: This is called the Source in case of v2
#ifdef PPRZLINK_2
  ParsingDestination,
  ParsingClassIdAndComponentId,
#endif
  ParsingMsgId,
  ParsingMsgPayload,
  CheckingCRCA,
  CheckingCRCB
};

struct normal_parser_t {
  enum normal_parser_states state;
  unsigned char length;
  int counter;
  unsigned char sender_id; //Note that Source is the official PPRZLink v2 name
#ifdef PPRZLINK_2
  unsigned char destination;
  unsigned char class_id; //HiNibble 4 bits
  unsigned char component_id; //LowNibble 4 bits
#endif
  unsigned char msg_id;
  unsigned char payload[256];
  unsigned char crc_a;
  unsigned char crc_b;
};
#else
#error One must define PPRZLINK_1 or PPRZLINK_2 and that is not the case
#endif

struct normal_parser_t parser;

char packetBuffer[256]; //buffer to hold incoming packet
char outBuffer[256];    //buffer to hold outgoing data
uint8_t out_idx = 0;
uint8_t serial_connect_info = 1; //If 1 then spit out serial print wifi connection info for debugging purposes

WiFiUDP udp;

IPAddress myIP;

void setup() {
  Serial.begin(SERIAL_BAUD_RATE);

  Wire.begin();
  sensor.init();
  sensor.setTimeout(500);
  
  wifi_mode = WIFI_MODE;
  delay(1000); //A 1s delay, sorry but breathing room to make sure all is set en done
  /* Configure LED */
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  if (serial_connect_info) {
    Serial.println();
    Serial.print("Connnecting to ");
    Serial.println(ssid);
  }
  if (wifi_mode == WifiModeClient) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      if (serial_connect_info) {
        Serial.print(".");
      }
      /* Toggle LED */
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    myIP = WiFi.localIP();
  }
  else {
    /* AccessPoint mode */

    Serial.print("$");
    
    WiFi.softAP(ssid, password);
    myIP = WiFi.softAPIP();
    /* Reconfigure own broadcast IP */
    IPAddress AP_broadcastIP(192,168,4,255);
    broadcastIP = AP_broadcastIP;
  }
  
  if (serial_connect_info) {
    Serial.println(myIP);
  }

  udp.begin(localPort);

#ifdef ENABLE_OTA_UPDATE
  /* OTA Configuration */

  // Port defaults to 8266, change this if you want to heve it your way...
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("esp-module");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

#endif
  /* Connected, LED ON */
  digitalWrite(LED_PIN, HIGH);

  /* Start measuring range */
  sensor.startContinuous();
}

void loop() {
  
#ifdef ENABLE_OTA_UPDATE
  /* Check for OTA */
  ArduinoOTA.handle();
#endif

  /* Every fifth cycle, send sonar data */
  static uint8_t sonar_cnt = 0;
  if (sonar_cnt == 5) {
    sonar_cnt = 0;
    send_sonar_data();
  }
  ++sonar_cnt;

  /* Check for UDP data from host */
  int packetSize = udp.parsePacket();
  int len = 0;
  if(packetSize > 0) { /* data received on udp line*/
    // read the packet into packetBufffer
    len = udp.read(packetBuffer, 255);
    Serial.write(packetBuffer, len);
  }

  /* Check for Serial data from UA */
  /* Put all serial in_bytes in a buffer */
  while(Serial.available() > 0) {
    //digitalWrite(LED_PIN, LOW);
    unsigned char inbyte = Serial.read();
    if (parse_single_byte(inbyte)) { // if complete message detected
      udp.beginPacketMulticast(broadcastIP, txPort, myIP);
      udp.write(outBuffer, out_idx);
      udp.endPacket();
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
  }
  
  //delay(10);
  //digitalWrite(LED_PIN, HIGH);

}

/* 
   PPRZLINK v1
   PPRZ-message: ABCxxxxxxxDE
    A PPRZ_STX (0x99)
    B LENGTH (A->E)
    C PPRZ_DATA
      0 SENDER_ID
      1 MSG_ID
      2 MSG_PAYLOAD
      . DATA (messages.xml)
    D PPRZ_CHECKSUM_A (sum[B->C])
    E PPRZ_CHECKSUM_B (sum[ck_a])

   PPRZLINK v2
   PPRZ-message: ABCxxxxxxxDE
    A PPRZ_STX (0x99)
    B LENGTH (A->E)
    C PPRZ_DATA
      0 SOURCE (~sender_ID)
      1 DESTINATION (can be a broadcast ID)
      2 CLASS/COMPONENT
        bits 0-3: 16 class ID available
        bits 4-7: 16 component ID available
      3 MSG_ID
      4 MSG_PAYLOAD
      . DATA (messages.xml)
    D PPRZ_CHECKSUM_A (sum[B->C])
    E PPRZ_CHECKSUM_B (sum[ck_a])
    
    PPRZ Link GEC not implemented, one can considder WPA2x or implemting the parder oneself
    
    Returns 0 if not ready, return 1 if complete message was detected
*/

uint8_t class_componenent_raw;
uint8_t parse_single_byte(unsigned char in_byte)
{

  switch (parser.state) {

/* Same for PPRZLINK v1 and v2 */
    case SearchingPPRZ_STX:
      out_idx = 0;
      if (in_byte == PPRZ_STX) {
        //printf("Got PPRZ_STX\n");
        parser.crc_a = 0;
        parser.crc_b = 0;
        parser.counter = 1;
        parser.state = ParsingLength;
      }
      break;

/* Same for PPRZLINK v1 and v2 */
    case ParsingLength:
      parser.length = in_byte;
      parser.crc_a += in_byte;
      parser.crc_b += parser.crc_a;
      parser.counter++;
      parser.state = ParsingSenderId;
      break;


/* Same for PPRZLINK v1 and v2, however naming is of Source like old SenderId */
    case ParsingSenderId:
      parser.sender_id = in_byte;
      parser.crc_a += in_byte;
      parser.crc_b += parser.crc_a;
      parser.counter++;
      #if defined PPRZLINK_1
      parser.state = ParsingMsgId;
      #endif
      #if defined PPRZLINK_2
      parser.state = ParsingDestination;
      #endif
      break;


#if defined PPRZLINK_2
    case ParsingDestination:
      parser.destination = in_byte;
      parser.crc_a += in_byte;
      parser.crc_b += parser.crc_a;
      parser.counter++;
      parser.state = ParsingClassIdAndComponentId;
      break;
  
    case ParsingClassIdAndComponentId:
      class_componenent_raw = in_byte;
      parser.class_id = ((byte)in_byte & 0xf0) >> 4; //HiNibble 1st4 bits
      parser.component_id = (byte)in_byte & 0x0f; //LoNibble last 1st4 bits
      parser.crc_a += in_byte;
      parser.crc_b += parser.crc_a;
      parser.counter++;
      parser.state = ParsingMsgId;
      break;
#endif  

/* Same for PPRZLINK v1 and v2 */
    case ParsingMsgId:
      parser.msg_id = in_byte;
      parser.crc_a += in_byte;
      parser.crc_b += parser.crc_a;
      parser.counter++;
      #if defined PPRZLINK_1
      if (parser.length == 6) 
      #endif
      #if defined PPRZLINK_2
      if (parser.length == 8)
      #endif            
      { 
        parser.state = CheckingCRCA;
      } else {
        parser.state = ParsingMsgPayload;
      }
      break;

/* Same for PPRZLINK v1 and v2 */
    case ParsingMsgPayload:
      parser.payload[parser.counter-4] = in_byte;
      parser.crc_a += in_byte;
      parser.crc_b += parser.crc_a;
      parser.counter++;
      if (parser.counter == parser.length - 2) {
        parser.state = CheckingCRCA;
      }
      break;

    case CheckingCRCA:
      //printf("CRCA: %d vs %d\n", in_byte, parser.crc_a);
      if (in_byte == parser.crc_a) {
        parser.state = CheckingCRCB;
      }
      else {
        parser.state = SearchingPPRZ_STX;
      }
      break;

    case CheckingCRCB:
      //printf("CRCB: %d vs %d\n", in_byte, parser.crc_b);
      if (in_byte == parser.crc_b) {
        #if defined PPRZLINK_1
        /*printf("MSG ID: %d \t"
               "SENDER_ID: %d\t"
               "LEN: %d\t"
               "SETTING: %d\n",
               parser.msg_id,
               parser.sender_id,
               parser.length,
               parser.payload[0]);*/
        //printf("Request confirmed\n");
        #endif
        
        #if defined PPRZLINK_2
        /*printf("MSG ID: %d \t"
               "SOURCE: %d\t"
               "DESTINATION: %d\t"
               "LEN: %d\t"
               "SETTING: %d\n",
               parser.msg_id,
               parser.sender_id,
               parser.destination,
               parser.class_id,
               parser.component_id,
               parser.length,
               parser.payload[0]);*/
        //printf("Request confirmed\n");
        #endif
        
        /* Check what to do next if the command was received */
        outBuffer[out_idx++] = in_byte; // final byte
        parser.state = SearchingPPRZ_STX;
        return 1;
      }
      parser.state = SearchingPPRZ_STX;
      break;

    default:
      /* Should never get here */
      break;
  }
  
  outBuffer[out_idx++] = in_byte;
  return 0;
}

uint8_t sonar_crc_a;
uint8_t sonar_crc_b;

uint8_t sonar_byte(uint8_t in_byte) {
  sonar_crc_a += in_byte;
  sonar_crc_b += sonar_crc_a;
  return in_byte;
}

/** Read and send magneto message
 *  This is hacked into the HTIL_INFRARED messages
 *  which is almost perfect in data types.
 * 
 * PPRZ-message: ABCxxxxxxxDE
    A PPRZ_STX (0x99)
    B LENGTH (A->E)
    C PPRZ_DATA
      0 SENDER_ID
      1 MSG_ID
      2 MSG_PAYLOAD
      . DATA (messages.xml)
    D PPRZ_CHECKSUM_A (sum[B->C])
    E PPRZ_CHECKSUM_B (sum[ck_a])
 */
#define SONAR_MSG_LENGTH 6+6+2
void send_sonar_data(){
  uint8_t sonar_msg[SONAR_MSG_LENGTH]; // length
  sonar_crc_a = 0;
  sonar_crc_b = 0;

  sonar_msg[0] = PPRZ_STX;
  sonar_msg[1] = sonar_byte(SONAR_MSG_LENGTH); // MSG LENGTH
  sonar_msg[2] = sonar_byte(parser.sender_id); // SENDER ID
  sonar_msg[3] = sonar_byte(parser.destination);
  sonar_msg[4] = sonar_byte(class_componenent_raw);
  sonar_msg[5] = sonar_byte(236); // MSG SONAR

  uint16_t millimeters = sensor.readRangeContinuousMillimeters();
  //uint16_t millimeters = 1000;
  uint8_t msb = (millimeters >> 8);
  uint8_t lsb = (millimeters & 0xFF);

  sonar_msg[6] = sonar_byte(msb);         // MSB
  sonar_msg[7] = sonar_byte(lsb);         // LSB
  sonar_msg[8] = sonar_byte(0);
  sonar_msg[9] = sonar_byte(0);
  sonar_msg[10] = sonar_byte(0);
  sonar_msg[11] = sonar_byte(0);

  sonar_msg[12] = sonar_crc_a;
  sonar_msg[13] = sonar_crc_b;

  Serial.write(sonar_msg, SONAR_MSG_LENGTH);

}
