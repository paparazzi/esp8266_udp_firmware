#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include "wifi_config.h"

/* Magnetometer defines */
#define HMC5883           0x1E     // 7-bit R/W I2C address for The HMC5883
#define MODE_REGISTER     0x02     // HMC8553's Mode Register address
#define X_MSB_REGISTER    0x03     // HMC8553's Data Output X-MSB Register
#define CONT_MEASURE_MODE 0x00     // Continuous-measurement Mode

/* Struct with available modes */
enum wifi_modes {
  WifiModeClient,
  WifiModeAccessPoint
} wifi_mode = WifiModeAccessPoint;

#define PPRZ_STX 0x99
#define LED_PIN BUILTIN_LED // change back to 13

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
uint8_t serial_connect_info = 1; // Serial print wifi connection info

WiFiUDP udp;

IPAddress myIP;

void setup() {
  wifi_mode = WIFI_MODE;
  delay(1000);
  /* Configure LED */
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  Serial.begin(115200);
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
    WiFi.softAP(ssid, password);
    myIP = WiFi.softAPIP();
    /* Reconfigure broadcast IP */
    IPAddress AP_broadcastIP(192,168,4,255);
    broadcastIP = AP_broadcastIP;
  }
  
  if (serial_connect_info) {
    Serial.println(myIP);
  }

  udp.begin(localPort);


  /* OTA Configuration */
  /*
  // Port defaults to 8266
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
  */

  /* Connected, LED ON */
  digitalWrite(LED_PIN, HIGH);

  /* Magnetometer setup */
  Wire.begin(0,2);
  Wire.beginTransmission(HMC5883);
  Wire.write(MODE_REGISTER);       // Set the HMC8553's address pointer to the Mode Register
  Wire.write(CONT_MEASURE_MODE);   // Set the Mode Register to Continuous-measurement Mode
  Wire.endTransmission();
}

void loop() {
  /* Every 5th cycle, read magneto */
  static uint8_t mag_cnt = 0;
  if (mag_cnt == 5) {
    mag_cnt = 0;
    send_magneto_data();
  }
  ++mag_cnt;
  
  /* Check for OTA */
  //ArduinoOTA.handle();

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
  
  delay(2);
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


uint8_t mag_crc_a;
uint8_t mag_crc_b;
uint8_t mag_byte(uint8_t in_byte) {
  mag_crc_a += in_byte;
  mag_crc_b += mag_crc_a;
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
void send_magneto_data(){
  uint8_t mag_msg[13];
  mag_crc_a = 0;
  mag_crc_b = 0;
  
  mag_msg[0] = PPRZ_STX;
  mag_msg[1] = mag_byte(13);
  mag_msg[2] = mag_byte(0);
  mag_msg[3] = mag_byte(7); // MSG HITL_INFRARED

  Wire.beginTransmission(HMC5883);
  Wire.write(X_MSB_REGISTER);      // Set address pointer to the 1st data-output register
  Wire.endTransmission();

  // Read in the 6 bytes of data and convert it to 3 integers representing x, y and z
  Wire.requestFrom(HMC5883, 6);
  if (Wire.available() == 6){
    mag_msg[4] = mag_byte(Wire.read());         // X-MSB
    mag_msg[5] = mag_byte(Wire.read());         // X-LSB
    mag_msg[6] = mag_byte(Wire.read());         // Y-MSB
    mag_msg[7] = mag_byte(Wire.read());         // Y-LSB
    mag_msg[8] = mag_byte(Wire.read());         // Z-MSB
    mag_msg[9] = mag_byte(Wire.read());         // Z-LSB

    mag_msg[10] = mag_byte(0); // unused byte of HITL_INFRARED (ac_id)
    mag_msg[11] = mag_crc_a;
    mag_msg[12] = mag_crc_b;

    Serial.write(mag_msg, 13);
  }
}
