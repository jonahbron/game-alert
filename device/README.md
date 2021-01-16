# Game Alert Device

The device software is written as an Arduino sketch.  When uninitialized, it will create a "game-alert-device" network so that a user can connect and provide the Wi-Fi credentials.  Once provided, it will connect to the local network.

To develop, the esp8266 board must be installed.

https://github.com/esp8266/Arduino#installing-with-boards-manager

You may also need to install the driver for the CH340 USB chip, which the hardware uses to connect to a host computer.  Be sure to use a fully connected USB cable as well.

https://learn.sparkfun.com/tutorials/how-to-install-ch340-drivers/discuss

TODO deep sleep mode is not possible without an additional jumper: https://www.instructables.com/Enable-DeepSleep-on-an-ESP8266-01/
