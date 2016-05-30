#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "wifi_config.h"

#define PPRZ_STX 0x99
#define LED_PIN 13

/* PPRZ message parser states */
enum normal_parser_states {
  SearchingPPRZ_STX,
  ParsingLength,
  ParsingSenderId,
  ParsingMsgId,
  ParsingMsgPayload,
  CheckingCRCA,
  CheckingCRCB
};

struct normal_parser_t {
  enum normal_parser_states state;
  unsigned char length;
  int counter;
  unsigned char sender_id;
  unsigned char msg_id;
  unsigned char payload[100];
  unsigned char crc_a;
  unsigned char crc_b;
};

struct normal_parser_t parser;

char packetBuffer[255]; //buffer to hold incoming packet
char outBuffer[255];    //buffer to hold outgoing data
uint8_t out_idx = 0;

WiFiUDP udp;

IPAddress myIP;

void setup() {
  delay(1000);
  /* Configure LED */
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  Serial.begin(115200);
  //Serial.println();
  //Serial.print("Connnecting to ");
  //Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
    /* Toggle LED */
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
  myIP = WiFi.localIP();
  //Serial.println(myIP);

  udp.begin(localPort);
  /* Connected, LED ON */
  digitalWrite(LED_PIN, HIGH);
}

void loop() {
  /* Check for UDP data from host */
  int packetSize = udp.parsePacket();
  int len = 0;
  if(packetSize) { /* data received on udp line*/
    // read the packet into packetBufffer
    len = udp.read(packetBuffer, 255);
    Serial.write(packetBuffer, len);
  }

  /* Check for Serial data from drone */
  /* Put all serial in_bytes in a buffer */
  while(Serial.available() > 0) {
    digitalWrite(LED_PIN, LOW);
    unsigned char inbyte = Serial.read();
    if (parse_single_byte(inbyte)) { // if complete message detected
      udp.beginPacketMulticast(broadcastIP, txPort, myIP);
      udp.write(outBuffer, out_idx);
      udp.endPacket();
    }
  }
  delay(10);
  digitalWrite(LED_PIN, HIGH);
}

/*
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

    Returns 0 if not ready, return 1 if complete message was detected
*/
uint8_t parse_single_byte(unsigned char in_byte)
{
  switch (parser.state) {

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

    case ParsingLength:
      parser.length = in_byte;
      parser.crc_a += in_byte;
      parser.crc_b += parser.crc_a;
      parser.counter++;
      parser.state = ParsingSenderId;
      break;

    case ParsingSenderId:
      parser.sender_id = in_byte;
      parser.crc_a += in_byte;
      parser.crc_b += parser.crc_a;
      parser.counter++;
      parser.state = ParsingMsgId;
      break;

    case ParsingMsgId:
      parser.msg_id = in_byte;
      parser.crc_a += in_byte;
      parser.crc_b += parser.crc_a;
      parser.counter++;
      if (parser.length == 6) {
        parser.state = CheckingCRCA;
      } else {
        parser.state = ParsingMsgPayload;
      }
      break;

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
        /*printf("MSG ID: %d \t"
               "SENDER_ID: %d\t"
               "LEN: %d\t"
               "SETTING: %d\n",
               parser.msg_id,
               parser.sender_id,
               parser.length,
               parser.payload[0]);*/
        //printf("Request confirmed\n");

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
