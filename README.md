# About

With this firmware, you can use an ESP8266 WiFi module for telemetry icw a UAS that runs Paparazzi autopilot code. To connect there are two option. The frist is that the module can work as a client that connects to a Hotspot. For example, you can have the ESP module connect to the a same router as your computer. The second option is to set it up as a Router itself, then just connect your PC to this module, just select SSID you gave it.

# Building

To create the module just buy a regular ESP09 module, available from many suppliers.

<img src="pictures/ESP_09_READY.jpg"  alt="ESP09 Assembly" height="200" width="200"/>
<img src="pictures/Finished_patched_ESP_09_a_ESP8266_WiFiModule.jpg"  alt="ESP09 Patched" height="200" width="200"/>

## Pinouts
<img src="pictures/ESP8266_09_bottom.png"  alt="ESP09 Bottom" height="200" width="200"/>
<img src="pictures/ESP8266_09_top.png"  alt="ESP09 Top" height="200" width="200"/>

## Pico-ESP
There is also an very light and minimum sized Pico-ESP, developed at the MAVLab TUDelft. This module runs the very same software.
<img src="pictures/pico-esp.jpg" alt="Pico ESP" height="200" width="200"/>

# Tools 

Installationthe following: 

- Download the latest stable Arduino IDE from the [Arduino website](http://www.arduino.cc/en/main/software). We used the Linux 64 version with good success.
- Enter ```http://arduino.esp8266.com/stable/package_esp8266com_index.json``` into *Additional Board Manager URLs* field. You can add multiple URLs, separating them with commas. You can find it under menu  item File -> Preferences -> Settings
- Open Boards Manager from Tools -> Board menu and install *esp8266* platform.

# Configure
Configuring the ESP

- Within the Arduino IDE, select your ESP8266 board from Tools -> Board menu.
- Open `pprz_udp_link.ino`. There should also be a tab with `wifi_config.h`.
- In `wifi_config.h`, configure the settings of the hotspot you want to connect to.
- Also change the broadcastIP to the broadcast IP in your network. In Linux, you should be able to discover this by executing `ifconfig`. Look for `Bcast:*.*.*.*` for the wireless network interface (something like wlan*).

# Flashing
- Upload the firmware to your ESP8266 by pressing the upload button.

# Airframe
In your airframe file, put the following configuration within the main AP`<firmware>` section:

```
<subsystem name="telemetry" type="transparent">
  <configure name="MODEM_BAUD" value="B115200"/>
  <configure name="MODEM_PORT" value="UART1"/>
</subsystem>
```
If you are using a different serial port on you autopilot board, change `UART1` accordingly.

# Connecting

WARNING: The module runs on **3.3V**, don't put 5V on it, 5V will destroy you board

To connect the module to your AP board:

- Connect VCC and GND, RX to TX, and TX to RX of the ESP09 board 

| ESP8266 | Autopilot |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| RX | TX |
| TX | RX |

# Testing

- Connect to you SSID of the module or let it connect to your router
- Start Paparazzi Center
- Select Tools -> Data Link
- Stop link
- Replace the command line options with "~/paparazzi/sw/ground_segment/tmtc/link **-udp**".
- Press [REDO] to restart link with the new parameters
- Wait a second...
- Enjoy telemetry over Wifi with Paparazzi

TIP: Data Messages can be simply viewed also with the.. message tool, see the Paparazzi Center tools menu.

# Documentation

Well, yes it is a opensource wiki and project. If you feel something is lacking, please add it, TIA

# Suggestions for improvement

Instead of flashing the module with a fixed SSID and password, it would be nice to have this configurable from the autopilot. Upon startup, the ESP8266 could send out requests for the SSID and password. A module within PPRZ could react by providing this in a PPRZ message format.

# Links

For more information also take a look at

- Documentation on the PICO-ESP can be found: [ on this PicoESP wiki](https://github.com/paparazzi/esp8266_udp_firmware/wiki).
- This [ESP8266 SDK project on github](https://github.com/esp8266/Arduino)
- Information about the ESP8266 chip and arduino tools: [esp8266 wiki](https://github.com/esp8266/esp8266-wiki/wiki)
